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

            lineNum++;
        }

        linkNodes();
        verifyLink(); //assert
        levelize();
    }

    this->numNodes = lineNum ;
    this->maxLevel = 0;
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


void Circuit::simulate(uint input) {
    cktQ *toEvaluate;
    cktQ *notEvaluated;

    for (int i = 0; i < PInodes.size(); i++) {
        LOGIC inBit = (LOGIC)GETBIT(input, i);
        if(inBit != (int)PInodes[i]->getValue()) {
            PInodes[i]->setValue(inBit);
            for (int j = 0; j < PInodes[i]->getDownstreamList().size(); j++){
                toEvaluate->push(PInodes[i]->getDownstreamList()[j]);
            }
        }
    }

    simulate(toEvaluate, notEvaluated, 1);
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

void Circuit::reset() {
    for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        cktNode *currNode = it->second;
        currNode->reset();
    }
}





