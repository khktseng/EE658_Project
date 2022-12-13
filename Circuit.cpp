/* Circuit class
*/

#include "Circuit.h"

Circuit::Circuit(string filename) {
    ifstream cktFile(filename, ios::in);

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
    initialized = false;
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

    simulate(&toEvaluate, &notEvaluated, 1);
    initialized = true;
}


void Circuit::simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level) {
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
        }
    }

    simulate(notEvaluated, toEvaluate, level + 1);
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




