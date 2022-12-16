/* Circuit class
*/

#include "Circuit.h"

Circuit::Circuit(string filename) {
    ifstream cktFile(filename.c_str());

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

        placeholder();

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


faultList Circuit::rflCheckpoint() {
    faultList reducedFaultList;

    for (int i = 0; i < PInodes.size(); i++) {
        reducedFaultList.push_back(new Fault(PInodes[i], 0));
        reducedFaultList.push_back(new Fault(PInodes[i], 1));
    }

    for (int i = 0; i < FBnodes.size(); i++) {
        reducedFaultList.push_back(new Fault(FBnodes[i], 0));
        reducedFaultList.push_back(new Fault(FBnodes[i], 1));
    }

    return reducedFaultList;
}


faultList Circuit::generateFaults(bool reduced) {
    if (reduced) { return rflCheckpoint();}

    faultList faults;
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        faults.push_back(new Fault(it->second, 0));
        faults.push_back(new Fault(it->second, 1));
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


double Circuit::faultCoverage(faultList* detectedFaults) {
    double totalFaults = generateFaults(true).size();
    return detectedFaults->size() / totalFaults;
}


inputMap* Circuit::randomTestGen() {
    int numPIs = PInodes.size();
    ulong randomBits = rand() % (int(pow(2, numPIs)));
    inputMap* test = new inputMap();

    for (int i = 0; i < numPIs; i++) {
        (*test)[PInodes[i]->getNodeID()] = (LOGIC)(randomBits & 0b1);
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


faultMap* Circuit::deductiveFaultSim(faultList* fl, inputList* ins) {
    faultMap* detectedFaults = new faultMap();
    bool faultDetected;

    Fault* currFault;
    inputMap* currInput;

    for (int i = 0; i < fl->size(); i++) {
        faultDetected = false;
        this->reset();
        currFault = fl->at(i);
        this->addFault(currFault);

        for (int j = 0; j < ins->size() && !faultDetected; j++) {
            resetPO();
            currInput = ins->at(j);
            this->simulate(currInput);

            if (faultAtPO()) {
                if (!(detectedFaults->count(currInput))) {
                    detectedFaults->insert(pair<inputMap*,faultList*>(currInput, new faultList()));
                }
                detectedFaults->at(currInput)->push_back(currFault);
                faultDetected = true;
                //cout << "Fault " << currFault << " detected by " << currInput << "\n";
            }
        }

        /*
        if (!faultDetected) {
             cout << "fault " << currFault << " not detected\n";
        }*/
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
            testVector->insert(pair<int, LOGIC>(PInodes[i]->getNodeID(), PInodes[i]->getTrueValue()));
        }
        return testVector;
    } else {
        cout << "PODEM Failed : " << fault << "\n\n";
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
    piBacktrace.node->setValue(X);
    piBacktrace.node->tested = true;

    if (podemOut) {
        //cout << "Backtrace failed\n";
    }
    return podemOut;
}

cktNode Circuit::getNode(int nodeID)
    {return *nodes[nodeID];}


void Circuit::atpg() {
    int numRandom = PInodes.size() * 4;

    cout << "Generating random faults...\n";
    faultList reducedFaults = generateFaults(true);
    inputList randomInputs = randomTestsGen(numRandom);

    faultMap * randomFD = deductiveFaultSim(&reducedFaults, &randomInputs);
    inputMap * firstInput = randomInputs[0];

    faultList * randomF = new faultList();

    for (faultMap::iterator it = randomFD->begin(); it != randomFD->end(); ++it) {
        randomF->insert(randomF->end(), randomFD->at(it->first)->begin(), randomFD->at(it->first)->end());
    }

    cout << faultCoverage(randomF) << "\n";
}

