/* Circuit class
*/

#include "Circuit.h"

Circuit::Circuit(char* file) {
    ifstream cktFile(file);

    if (cktFile.fail()) {
        cout << "File: " << file << " does not exist.\n";
    }

    vector<cktNode*> nodeList;
    int nodeID;
    int lineNum;
    int ntype, gtype;
    nodeT nodeType;
    gateT gateType;
    int numFanIns;
    int numFanOuts;
    int usID;
    vector<int> *upstreamIDs;
    int downstreamID;
    int numGates;

    cktNode *currNode;
    string currLine;
    lineNum = 0;
    if (cktFile.good()) {
        while (getline(cktFile, currLine)) {
            istringstream ss(currLine);

            ss >> ntype >> nodeID >> gtype;
            nodeType = (nodeT) ntype;
            gateType = (gateT) gtype;

            vector<int> usIDs;

            switch (nodeType) {
                case PO:
                case GATE:
                    numGates++;
                    ss >> numFanOuts >> numFanIns;
                    while (ss >> usID) {
                        usIDs.push_back(usID);
                    }
                    break;
                case PI:
                    ss >> numFanOuts;
                    numFanIns = 0;
                    break;
                case FB:
                    numFanOuts = 1;
                    numFanIns = 1;
                    ss >> usID;
                    usIDs.push_back(usID);
                    break;
                default:
                    cout << "Error reading node type \n";
                    break;
            }

            currNode = new cktNode(nodeID, lineNum, gateType, nodeType, numFanIns, numFanOuts, usIDs);
            this->nodes[nodeID] = currNode;

            if (nodeType == PI) {this->PInodes.push_back(currNode);}
            if (nodeType == PO) {this->POnodes.push_back(currNode);}
            if (nodeType == FB) {this->FBnodes.push_back(currNode);}

            lineNum++;
        }
        linkNodes();
        verifyLink(); //assert
        levelize();
    }

    char* fileName = strdup(file);
    char *strPtr = strchr(fileName,'/');
    while (strPtr != NULL) {
        strcpy(fileName, strPtr+1);
        strPtr = strchr(fileName,'/');
    }
    strPtr = strchr(fileName, '.');
    *strPtr = '\0';
    
    this->cktName = fileName;
    this->numNodes = lineNum ;
    this->maxLevel = 0;
    this->initialized = false;
    assert(this->numNodes == this->nodes.size());
}

void Circuit::resetPO() {
    for (int i = 0; i < POnodes.size(); i++) {
        POnodes[i]->resetValue();
    }
}


void Circuit::printPO() {
    cout <<"<";
    for(int i = 0; i < POnodes.size(); i++) {
        cout << POnodes[i]->getNodeID() << "=" << POnodes[i]->getValue() << ",";
    }
    cout << ">\n";
}


void Circuit::linkNodes(){
    //link all upstream nodes for each node first
    for(cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        it->second->link(&nodes);
    }
}


void Circuit::verifyLink() {
    for(cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        cktNode *checkNode = it->second;

        if (checkNode->getUpstreamList().size() != checkNode->getNumFanIns()) {
            cout << checkNode->getNodeID() << " failed usptream \n";        
        }

        if (checkNode->getDownstreamList().size() != checkNode->getNumFanOuts()) {
            cout << checkNode->getNodeID() << " failed downstream\n";
        }


        assert(checkNode->getUpstreamList().size() == checkNode->getNumFanIns());
        assert(checkNode->getDownstreamList().size() == checkNode->getNumFanOuts());
    }
}


vector<int> Circuit::getNodeIDs(){
    vector<int> nodeIDs;
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        nodeIDs.push_back(it->first);
    }

    return nodeIDs;
}


void Circuit::levelize() {
    for (int i = 0; i < PInodes.size(); i++) {
        levelize(PInodes[i], 0);
    }

    // Map nodes by level in this.levNodes
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        cktNode *currNode = it->second;
        int level = currNode->getLevel();
        levNodes[level].push_back(currNode);
    }
}


void Circuit::levelize(cktNode* currNode, int currLevel) {
    int nodeLevel = currNode->getLevel();
    if (nodeLevel != -1) {
        if(nodeLevel >= currLevel) {
            return;
        }
    }

    currNode->setLevel(currLevel);
    if (maxLevel < currLevel) {
        maxLevel = currLevel;
    }

    for(int i = 0; i < currNode->getNumFanOuts(); i++) {
        cktList dsl = currNode->getDownstreamList();
        levelize(dsl[i], currLevel + 1);
    }
}


void Circuit::simulate(inputMap* inputmap) {
    cktQ toEvaluate;
    cktQ notEvaluated;
    inputMap input = *inputmap;

    for (inputMap::iterator it = input.begin(); it != input.end(); ++it) {
        int nodeID = it->first;
        LOGIC inValue = it->second;
        if (!initialized || inValue != nodes[nodeID]->getValue()) {
            nodes[nodeID]->setValue(inValue);
            for (int j = 0; j < nodes[nodeID]->getDownstreamList().size(); j++){
                toEvaluate.push(nodes[nodeID]->getDownstreamList()[j]);
            }
        }
    }

    cktList xList;
    simulate(&toEvaluate, &notEvaluated, 1, &xList);
    initialized = true;
}


void Circuit::simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level, cktList* dFrontier) {
    if (toEvaluate->size() == 0) {
        return;
    }
    
    while(toEvaluate->size()) {
        cktNode *currNode = toEvaluate->front();
        toEvaluate->pop();

        if (currNode->getLevel() != level) {
            notEvaluated->push(currNode);
        } else if (currNode->evaluate()) {
            for (int i = 0; i < currNode->getDownstreamList().size(); i++) {
                notEvaluated->push(currNode->getDownstreamList()[i]);
            }
        } else if (currNode->getValue() == X){
            bool dFront = false;
            for (int i = 0; i < currNode->getUpstreamList().size(); i++) {
                if (currNode->getUpstreamList()[i]->getValue() == D
                    || currNode->getUpstreamList()[i]->getValue() == DB) {
                        dFront = true;
                }
            }
            if (dFront) {
                dFrontier->push_back(currNode);
            }
        }
    }

    simulate(notEvaluated, toEvaluate, level + 1, dFrontier);
}


LOGIC Circuit::getNodeLogic(int nodeID) {
    return nodes[nodeID]->getValue();
}


void Circuit::reset() {
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        cktNode *currNode = it->second;
        currNode->reset();
    }
    initialized = false;
}


faultSet Circuit::rflCheckpoint() {
    faultSet reducedFaultList;

    for (int i = 0; i < PInodes.size(); i++) {
        reducedFaultList.insert(new Fault(PInodes[i], 0));
        reducedFaultList.insert(new Fault(PInodes[i], 1));
    }

    for (int i = 0; i < FBnodes.size(); i++) {
        reducedFaultList.insert(new Fault(FBnodes[i], 0));
        reducedFaultList.insert(new Fault(FBnodes[i], 1));
    }

    return reducedFaultList;
}


faultSet Circuit::generateFaults(bool reduced) {
    if (reduced) { return rflCheckpoint();}

    faultSet faults;
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        faults.insert(new Fault(it->second, 0));
        faults.insert(new Fault(it->second, 1));
    }
    return faults;
}


void Circuit::addFault(Fault* fault) {
    fault->getNode()->setFault(fault->getSAV());
}

Fault* Circuit::createFault(int nodeID, int sav) {
    try {
        if (sav > 1 || sav < 0) {
            throw sav;
        }
    } catch (int s) {
        cout << "BadValueException: stuck at value " << s << " must be 1 or 0\n";
    }

    return new Fault(nodes[nodeID], sav);
}


double Circuit::faultCoverage(faultSet* detectedFaults) {
    double totalFaults = generateFaults(true).size();
    return detectedFaults->size() / totalFaults;
}


inputMap* Circuit::randomTestGen() {
    int numPIs = PInodes.size();
    ulong randomBits = rand() % (int(pow(2, numPIs)));
    inputMap* test = new inputMap();

    for (int i = 0; i < numPIs; i++) {
        (*test)[PInodes[i]->getNodeID()] = (LOGIC)(randomBits & 0b001);
        randomBits = randomBits >> 1;
    }

    return test;
}


inputList Circuit::randomTestsGen(int numTest) {
    inputList tests;
    for (int i = 0; i < numTest; i++) {
        tests.push_back(randomTestGen());
    }
    return tests;
}


faultMap* Circuit::deductiveFaultSim(faultSet* fl, inputList* ins) {
    faultMap* detectedFaults = new faultMap();
    bool faultDetected;

    Fault* currFault;
    inputMap* currInput;

    for (inputList::iterator it = ins->begin(); it != ins->end(); ++it) {
        for (faultSet::iterator itt = fl->begin(); itt != fl->end();) {
            this->reset();
            currFault = *itt;
            this->addFault(currFault);
            currInput = *it;
            this->simulate(currInput);

            //cout << currFault << " " << currInput << "\n";

            if(faultAtPO()) {
                if (!detectedFaults->count(currInput)) {
                    detectedFaults->insert(pair<inputMap*,faultList*>(currInput, new faultList()));
                }
                detectedFaults->at(currInput)->push_back(currFault);
                fl->erase(itt++);
            } else {
                ++itt;
            }
        }
    }
    
    return detectedFaults;
}


void Circuit::backwardsImplication(cktNode* root) {
    if(!root->imply()) {
        cktQ toEvaluate;
        cktQ notEvaluated;
        cktList dFrontier;
        for (int i = 0; i < root->getDownstreamList().size(); i ++) {
            toEvaluate.push(root->getDownstreamList()[i]);
        }
        this->simulate(&toEvaluate, &notEvaluated, root->getLevel() + 1, &dFrontier);
        return;
    }

    for (int i = 0; i < root->getUpstreamList().size(); i++) {
        backwardsImplication(root->getUpstreamList()[i]);
    }
}


OBJECTIVE Circuit::objective(cktList* dFrontier) {
    OBJECTIVE obj;

    if (dFrontier->empty()) {
        return {NULL, X};
    }

    cktNode* dGate;
    cktNode* dIn;

    do {
        dGate = dFrontier->back();
        dFrontier->pop_back();
        dIn = dGate->getUnassignedInput(true);
    } while (dIn == NULL && !dFrontier->empty());
    dFrontier->push_back(dGate);
    
    LOGIC objV;
    switch (dGate->getGateType()) {
        case XOR:
            objV = ZERO;
            break;
        case XNOR:
            objV = ONE;
            break;
        case OR:
        case NOR:
            objV = ZERO;
            break;
        case NAND:
        case AND:
            objV = ONE;
            break;
        default:
            cout << "objective error\n";
            break;
    }

    obj = {dIn, objV};
    return obj;
}


OBJECTIVE Circuit::backtrace(OBJECTIVE kv) {
    if (kv.node->tested) {
        cout << "tested\n";
        return {NULL, X};
    }
    if (kv.node->getNodeType() == PI){
        return kv;
    }

    cktNode* nextBack = kv.node->getUnassignedInput(kv.node->needAllInputs(kv.targetValue));
    if (nextBack == NULL) {
        return kv;
    }
    LOGIC likely = kv.node->getLikely(nextBack, kv.targetValue);

    return backtrace({nextBack, likely});
}

bool Circuit::faultAtPO() {
    for (int i = 0; i < POnodes.size(); i++) {
        if (POnodes[i]->getValue() == D || POnodes[i]->getValue() == DB) {
            return true;
        }
    }
    return false;
}

bool Circuit::xPathCheck(cktList* dFrontier) {
    cktQ dFQ;
    for (int i = 0; i < dFrontier->size(); i++) {
        dFQ.push(dFrontier->at(i));
    }

    int frontierSize = dFQ.size();
    for (int i = 0; i < frontierSize; i++) {
        cktNode* currNode = dFQ.front();
        dFQ.pop();
        if (xPathCheck(currNode)) {
            //dFrontier->push(currNode);
            return true;
        }
        //dFrontier->push(currNode);
    }
    return false;
}

bool Circuit::xPathCheck(cktNode* node) {
    if (node->getValue() != X) {
        return false;
    }
    if (node->getNodeType() == PO) {
        return true;
    }
    for (int i = 0; i < node->getDownstreamList().size(); i++) {
        if (xPathCheck(node->getDownstreamList()[i])) {
            return true;
        }
    }
    return false;
}

void Circuit::placeholder() {
    int x = 10;
}

inputMap* Circuit::PODEM(Fault* fault) {
    this->reset();
    this->addFault(fault);

    LOGIC dVal = fault->getNode()->getTrueValue();
    cktQ toEvaluate;
    cktQ notEvaluated;
    cktList dFrontier;
    cktNode* dNode = fault->getNode();
    

    for (int i = 0; i < dNode->getDownstreamList().size(); i++) {
        toEvaluate.push(dNode->getDownstreamList()[i]);
    }

    if (dNode->getNodeType() == FB) {
        while (dNode->getNodeType() == FB) {
            dNode = dNode->getUpstreamList()[0];
        }
        dNode->setValue(dVal);
        for (int i = 0; i < dNode->getDownstreamList().size(); i++) {
            toEvaluate.push(dNode->getDownstreamList()[i]);
        }
    }

    this->simulate(&toEvaluate, &notEvaluated, dNode->getLevel() + 1, &dFrontier);
    this->backwardsImplication(dNode);

    if (podem(fault, &dFrontier)) {
        inputMap* testVector = new inputMap();
        for (int i = 0; i < PInodes.size(); i++) {
            if (PInodes[i]->getTrueValue() == D || PInodes[i]->getTrueValue() == DB) {
                testVector->insert(pair<int, LOGIC>(PInodes[i]->getNodeID(), X));
            } else {
                testVector->insert(pair<int, LOGIC>(PInodes[i]->getNodeID(), PInodes[i]->getTrueValue()));
            }
        }
        return testVector;
    } else {
        cout << "PODEM Failed : " << fault << "\n";
        return NULL;
    }
}

int pf(Fault* fault) {
    return fault->getNode()->getNodeID();
}

bool Circuit::podem(Fault* fault, cktList* dFrontier) {
    if (faultAtPO()) {
        //cout << "success, " << fault << " at PO\n";
        return true;
    }

    if (!xPathCheck(dFrontier)) {
        //cout << "xPath FAIL : " << fault << "\n";
        return false;
    }

    OBJECTIVE obj = objective(dFrontier);
    if (obj.node == NULL || obj.targetValue == X) {return false;}
    OBJECTIVE piBacktrace = backtrace(obj);
    
    if (piBacktrace.node == NULL) {

    }

    cktQ toEvaluate;
    cktQ notEvaluated;
    piBacktrace.node->setValue(piBacktrace.targetValue);
    //cout << piBacktrace.node->getNodeID() << " " << piBacktrace.targetValue << "\n";
    for (int i = 0; i < piBacktrace.node->getDownstreamList().size(); i++) {
        toEvaluate.push(piBacktrace.node->getDownstreamList()[i]);
    }
    this->simulate(&toEvaluate, &notEvaluated, piBacktrace.node->getLevel() + 1, dFrontier);
    if (podem(fault, dFrontier)) {
        return true;
    }

    assert(toEvaluate.empty() && notEvaluated.empty());
    //assert(updatedDFront.empty());

    piBacktrace.node->setValue(~piBacktrace.targetValue);
    for (int i = 0; i < piBacktrace.node->getDownstreamList().size(); i++) {
        toEvaluate.push(piBacktrace.node->getDownstreamList()[i]);
    }
    this->simulate(&toEvaluate, &notEvaluated, 1, dFrontier);
    bool podemOut = podem(fault, dFrontier);
    //piBacktrace.node->setValue(X);
    piBacktrace.node->tested = true;

    if (podemOut) {
        //cout << "Backtrace failed\n";
    }
    return podemOut;
}

cktNode Circuit::getNode(int nodeID)
    {return *nodes[nodeID];}


double Circuit::atpg(inputList* testVectors) {
    faultSet reducedFaults = generateFaults(true);
    faultSet detectedFaults;

    int numRandom = 0;
    double fcPrev = 0;
    double fc = 0;
    while ((fc == 0 || (fc - fcPrev > 0.01)) && numRandom < MAXRANDOM) {
        inputList randomInputs = randomTestsGen(1);
        faultMap* randomFD = deductiveFaultSim(&reducedFaults, &randomInputs);

        if (!randomFD->empty()) {
            for (faultList::iterator it = randomFD->at(randomInputs[0])->begin();
                    it != randomFD->at(randomInputs[0])->end(); ++it) {
                detectedFaults.insert(*it);
                reducedFaults.erase(*it);
            }
        }

        faultList uniqueFaults;
        for (set<Fault*>::iterator it = detectedFaults.begin(); it != detectedFaults.end(); ++it) {
            uniqueFaults.push_back(*it);
        }
        testVectors->push_back(randomInputs[0]);
        fcPrev = fc;
        fc = faultCoverage(&detectedFaults);
        numRandom++;
    }

    printf("Stopping random inputs on #%d with FC=%.3f and prevFC=%.3F\n", numRandom, fc, fcPrev);

    for (faultSet::iterator it = reducedFaults.begin(); it != reducedFaults.end(); ++it) {
        inputMap* newTV = this->PODEM(*it);
        if (newTV != NULL) {
            testVectors->push_back(newTV);
            detectedFaults.insert(*it);
        }
    }
    return faultCoverage(&detectedFaults);
}

double Circuit::atpg_det(inputList * testVectors) {
    faultSet faults = this->generateFaults(true);
    faultSet detectedFaults;
    for (faultSet::iterator it = faults.begin(); it != faults.end(); ++it) {
      inputMap* tv = this->PODEM(*it);
      if (tv != NULL) {
         testVectors->push_back(tv);
         detectedFaults.insert(*it);
      }
   }
   return faultCoverage(&detectedFaults);
}

string Circuit::getCktName() {
    return this->cktName;
}

inputMap* Circuit::createInputVector(vector<int> nodeIDs, vector<int> inputs) {
    inputMap* newInput = new inputMap();
    assert(nodeIDs.size() == inputs.size());

    for (int i = 0; i < nodeIDs.size(); i++) {
        LOGIC newI;
        switch (inputs[i]) {
            case 0:
                newI = ZERO;
                break;
            case 1:
                newI = ONE;
                break;
            default:
                newI = X;
                break;
        }
        newInput->insert(pair<int, LOGIC>(nodeIDs[i], newI));
    }
    return newInput;
}
