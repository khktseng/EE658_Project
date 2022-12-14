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


void Circuit::linkNodes(){
    //link all upstream nodes for each node first
    for(cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        it->second->link(&nodes);
    }
}


void Circuit::verifyLink() {
    for(cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        cktNode *checkNode = it->second;
        assert(checkNode->getUpstreamList().size() == checkNode->getNumFanIns());
        assert(checkNode->getDownstreamList().size() == checkNode->getNumFanOuts());
    }
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

    cktQ xList;
    simulate(&toEvaluate, &notEvaluated, 1, &xList);
    assert(xList.empty());
    initialized = true;
}


void Circuit::simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level, cktQ* dFrontier) {
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
        } else {
            bool dFront = false;
            for (int i = 0; i < currNode->getUpstreamList().size(); i++) {
                if (currNode->getUpstreamList()[i]->getValue() == D
                    || currNode->getUpstreamList()[i]->getValue() == DB) {
                        dFront = true;
                }
            }
            if (dFront) {
                dFrontier->push(currNode);
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
        if (nodeID > nodes.size()) {
            throw nodeID;
        }
    } catch (int n) {
        cout << "BadValueException: node " << n << " is out of range\n";
    }

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
    double totalFaults = nodes.size() * 2;
    return detectedFaults->size() / ((double)nodes.size() * 2.0);
}


inputMap Circuit::randomTestGen() {
    int numPIs = PInodes.size();
    ulong randomBits = rand() % (int(pow(2, numPIs)));
    inputMap test;

    for (int i = 0; i < numPIs; i++) {
        test[PInodes[i]->getNodeID()] = (LOGIC)(randomBits & 0b1);
        randomBits = randomBits >> 1;
    }

    return test;
}


inputList Circuit::randomTestsGen(int numTest) {
    inputList tests;
    inputMap test;
    for (int i = 0; i < numTest; i++) {
        test = randomTestGen();
        tests.push_back(&test);
    }
    return tests;
}


faultMap Circuit::deductiveFaultSim(faultList* fl, inputList* ins) {
    faultList faults = *fl;
    inputList inputs = *ins;
    faultMap detectedFaults;

    for (int i = 0; i < fl->size(); i++) {
        this->addFault(faults[i]);
        for (int j = 0; j < inputs.size(); i++) {
            this->simulate(inputs[j]);

            for (int k = 0; k < POnodes.size(); k++) {
                LOGIC currOutput = POnodes[k]->getValue();
                if (currOutput == D || currOutput == DB) {
                    detectedFaults[inputs[j]]->push_back(faults[i]);
                    break;
                }
            }
        }
        this->reset();
    }

    return detectedFaults;
}


void Circuit::backwardsImplication(cktNode* root) {
    if(!root->imply()) {return;}

    for (int i = 0; i < root->getUpstreamList().size(); i++) {
        backwardsImplication(root->getUpstreamList()[i]);
    }
}


OBJECTIVE Circuit::objective(cktQ* dFrontier) {
    OBJECTIVE obj;

    if (dFrontier->empty()) {
        return {NULL, X};
    }

    cktNode* dGate;
    cktNode* dIn;

    do {
        dGate = dFrontier->front();
        dFrontier->pop();
        dIn = dGate->getUnassignedInput(true);
    } while (dIn == NULL && !dFrontier->empty());
    
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


inputMap Circuit::PODEM(Fault* fault) {
    this->reset();
    this->addFault(fault);

    cktQ toEvaluate;
    cktQ notEvaluated;
    cktQ dFrontier;
    cktNode* dNode = fault->getNode();

    for (int i = 0; i < dNode->getDownstreamList().size(); i++) {
        toEvaluate.push(dNode->getDownstreamList()[i]);
    }
    this->simulate(&toEvaluate, &notEvaluated, dNode->getLevel(), &dFrontier);
    this->backwardsImplication(dNode);

    return podem(fault, &dFrontier);
}


inputMap Circuit::podem(Fault* fault, cktQ* dFrontier) {
    inputMap testPattern;
    if (faultAtPO()) {
        for (int i = 0; i < PInodes.size(); i++) {
            testPattern[PInodes[i]->getNodeID()] = PInodes[i]->getTrueValue();
        }
        return testPattern;
    }
    bool xPathCheck = false;
    for (int i = 0; i < POnodes.size(); i++) {
        if (POnodes[i]->getValue() == X){
            xPathCheck = true;
        }
    }
    if (!xPathCheck) {
        return testPattern;
    }

    OBJECTIVE obj = objective(dFrontier);
    if (obj.node == NULL || obj.targetValue == X) {return testPattern;}
    OBJECTIVE piBacktrace = backtrace(obj);

    cktQ toEvaluate;
    cktQ notEvaluated;
    piBacktrace.node->setValue(piBacktrace.targetValue);
    //cout << piBacktrace.node->getNodeID() << " " << piBacktrace.targetValue << "\n";
    for (int i = 0; i < piBacktrace.node->getDownstreamList().size(); i++) {
        toEvaluate.push(piBacktrace.node->getDownstreamList()[i]);
    }
    this->simulate(&toEvaluate, &notEvaluated, piBacktrace.node->getLevel(), dFrontier);
    testPattern = podem(fault, dFrontier);
    if (!testPattern.empty()) {
        return testPattern;
    }

    assert(toEvaluate.empty() && notEvaluated.empty());
    //assert(updatedDFront.empty());

    piBacktrace.node->setValue(~piBacktrace.targetValue);
    for (int i = 0; i < piBacktrace.node->getDownstreamList().size(); i++) {
        toEvaluate.push(piBacktrace.node->getDownstreamList()[i]);
    }
    this->simulate(&toEvaluate, &notEvaluated, 1, dFrontier);
    testPattern = podem(fault, dFrontier);
    piBacktrace.node->setValue(X);
    piBacktrace.node->tested = true;
    return testPattern;
}



