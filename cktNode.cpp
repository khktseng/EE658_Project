/* cktNode class*/

#include "cktNode.h"

cktList cktNode::getUpstreamList(){
    return upstreamNodes;
}
cktList cktNode::getDownstreamList(){
    return downstreamNodes;
}
int cktNode::getNodeID() {
    return nodeID;
}
int cktNode::getLineNum() {
    return lineNum;
}
gateT cktNode::getGateType() {
    return gateType;
}
nodeT cktNode::getNodeType() {
    return nodeType;
}
int cktNode::getLevel() {
    return level;
};
int cktNode::getNumFanIns() {
    return numFanIns;
}
int cktNode::getNumFanOuts() {
    return numFanOuts;
}

bool cktNode::faultMatch() {
    return (stuckAtValue == D && value != ZERO) ||
            (stuckAtValue == DB && value != ONE);
}

LOGIC cktNode::getValue() {
    if (stuckAt && faultMatch()) {
        return stuckAtValue; 
    }
    return value;
};
LOGIC cktNode::getTrueValue() {
    return value;
}

void cktNode::setValue(LOGIC v) {
    value = v;
};

// returns true if v can be implied without all inputs set
// returns false if v cannot
bool cktNode::needAllInputs(LOGIC v) {
    switch (gateType) {
        case NOT:
        case BRCH:
            return true;
            break;
        case XOR:
        case XNOR:
            return false;
            break;
        case NAND:
        case OR:
            return v == ONE;
            break;
        case AND:
        case NOR:
            return v == ZERO;
            break;
        default:
            return false;
    }
}

LOGIC cktNode::getLikely(cktNode* input, LOGIC v) {
    switch (gateType) {
        case NOT:
        case NAND:
        case NOR:
            return ~v;
            break;
        case BRCH:
        case AND:
        case OR:
            return v;
            break;
        default:
            break;
    }
    LOGIC likely = ZERO;
    for (int i = 0; i < upstreamNodes.size(); i++) {
        if (upstreamNodes[i]->isJustified()) {
            likely = likely ^ upstreamNodes[i]->getValue();
            if (gateType == XNOR) {
                likely = ~likely;
            }
        }
    }

    likely = likely ^ v;
    return likely;
}


cktNode* cktNode::getUnassignedInput(bool controllable) {
    cktNode* retNode = NULL;
    int controllability;

    if (controllable){
        controllability = this->level;
    } else {
        controllability = 0;
    }

    for (int i = 0; i < upstreamNodes.size(); i++) {
        if (upstreamNodes[i]->getValue() == X) {
            if (retNode == NULL ||
                 controllable && upstreamNodes[i]->level < controllability ||
                !controllable && upstreamNodes[i]->level > controllability) {

                retNode = upstreamNodes[i];
                controllability = upstreamNodes[i]->level;
            }
        }
    }

    return retNode;
}


cktNode::cktNode(int id, int ln, gateT gt, nodeT nt, int fis, int fos, vector<int> usIDs) {
    nodeID = id;
    lineNum = ln;
    gateType = gt;
    nodeType = nt;
    numFanIns = fis;
    numFanOuts = fos;
    upstreamIDs = usIDs;
    downstreamIDs = *(new vector<int>());
    level = -1;
    value = X;
    tested = false;

    stuckAt = false;
    linked = false;
}


void cktNode::link(cktMap *nodes) {
    if (linked) {
        return;
    }
    if (upstreamNodes.size() == 0 && upstreamIDs.size() != 0) {
        for(int i = 0; i < upstreamIDs.size(); i++) {
            upstreamNodes.push_back((*nodes)[upstreamIDs[i]]);
            (*nodes)[upstreamIDs[i]]->downstreamNodes.push_back(this);
            (*nodes)[upstreamIDs[i]]->downstreamIDs.push_back(this->nodeID);
        }
    }

    linked = true;
}


void cktNode::setFault(int sav) {
    stuckAt = true;
    tested = true;
    if (sav) {
        stuckAtValue = DB;
        value = ZERO;
    } else {
        stuckAtValue = D;
        value = ONE;
    }
}

// Evaluate the two logics based on gateType
LOGIC cktNode::eval(vector<LOGIC> args) {
    if (gateType == NOT) {return ~args[0];}
    if (gateType == BRCH || IPT) {return args[0];}

    LOGIC newVal = X;
    queue<LOGIC> notX = notXList();
    bool hasXIn = !(notX.size() == upstreamNodes.size());
    if (!notX.empty()) {
        newVal = notX.front();
        notX.pop();
    }

    while (!notX.empty()) {
        LOGIC popped = notX.front();
        notX.pop();
        switch (gateType) {
            case XNOR:
            case XOR:
                newVal = newVal ^ popped;
                break;
            case OR:
            case NOR:
                newVal = newVal | popped;
                break;
            case AND:
            case NAND:
                newVal = newVal & popped;
                break;
            default:
                cout << gateType << " evaluate error \n";
                assert(upstreamNodes.size() == 1);
                return X;
                break;
        }
    }

    if (hasXIn) {
        switch (gateType) {
            case XOR:
            case XNOR:  
                newVal = newVal ^ X; 
                break;
            case OR:
            case NOR:   
                newVal = newVal | X; 
                break;
            case AND:
            case NAND:  
                newVal = newVal & X; 
                break;
            default:
                cout << "x in error\n";
                cout << gateType << "\n";
                return X;
                break;
        }
    }

    if (gateType == XNOR || gateType == NOR || gateType == NAND) {
        return ~newVal;
    }
    return newVal;
}


LOGIC cktNode::eval(cktList args) {
    vector<LOGIC> argList;

    for (int i = 0; i < upstreamNodes.size(); i++) {
        argList.push_back(upstreamNodes[i]->getValue());
    }

    return eval(argList);
}

LOGIC cktNode::eval(cktList args, LOGIC ci) {
    vector<LOGIC> argList;
    argList.push_back(ci);
    for (int i = 0; i < upstreamNodes.size(); i++) {
        argList.push_back(upstreamNodes[i]->getValue());
    }

    return eval(argList);
}

LOGIC cktNode::eval(LOGIC a, LOGIC b) {
    vector<LOGIC> argList = {a, b};
    return eval(argList);
}


queue<LOGIC> cktNode::notXList() {
    queue<LOGIC> notX;
    for (int i = 0; i < upstreamNodes.size(); i++) {
        if (upstreamNodes[i]->getValue() != X) {
            notX.push(upstreamNodes[i]->getValue());
        }
    }
    return notX;
}


bool cktNode::evaluate() {
    assert(nodeType != PI);
    LOGIC newValue = eval(this->upstreamNodes);

    if (newValue != this->value) {
        this->value = newValue;
        return true;
    }
    return false;
}


bool cktNode::implyFromOutput(LOGIC xSet) {
    bool xFlag = false;
    for (int i = 0; i < upstreamNodes.size(); i++) {
        if (upstreamNodes[i]->getValue() == X) {
            xFlag = true;
            upstreamNodes[i]->setValue(xSet);
        }
    }
    return xFlag;
}

bool cktNode::implyFromInputs(LOGIC xSet, bool NGate) {
    if (NGate) {
        xSet = ~xSet;
    }
    cktList xList;
    for (int i = 0; i < upstreamNodes.size(); i++) {
        if ((upstreamNodes[i]->getValue() == xSet)) {
            return false;
        }
        if (upstreamNodes[i]->getValue() == X) {
            xList.push_back(upstreamNodes[i]);
        }
    }
    if (xList.size() == 1) {
        xList[0]->setValue(xSet);
        return true;
    }
    return false;
}


bool cktNode::imply(){
    if (nodeType == PI) {
        return false;
    }

    LOGIC ci = X;
    cktList xList;
    switch (gateType) {
        case BRCH:
            if (upstreamNodes[0]->getValue() == X) {
                upstreamNodes[0]->setValue(this->getTrueValue());
                return true;
            } else {return false;}
            break;
        case NOT:
            if (upstreamNodes[0]->getValue() == X) {
                upstreamNodes[0]->setValue(this->getTrueValue());
                return true;
            } else {return false;}
            break;
        case OR:
            if (this->getTrueValue() == ZERO) {
                return implyFromOutput(ZERO);
            } else { // this.truevalue is ONE
                return implyFromInputs(ONE, false);
            }
            break;
        case NOR:
            if (this->getTrueValue() == ONE) {
                return implyFromOutput(ZERO);
            } else { // this.truevalue is ZERO 
                return implyFromInputs(ONE, true);
            }
            break;
        case AND:
            if (this->getTrueValue() == ONE) {
                return implyFromOutput(ONE);
            } else { // this.truevalue is ZERO
                return implyFromInputs(ZERO, false);
            }
            break;
        case NAND:
            if (this->getTrueValue() == ZERO) {
                return implyFromOutput(ONE);
            } else {
                return implyFromInputs(ZERO, true);
            }
        case XOR:
            for (int i = 0; i < upstreamNodes.size(); i++) {
                if (upstreamNodes[i]->getValue() != X) {
                    if (ci == X) {ci = upstreamNodes[i]->getValue();}
                    else {
                        ci = eval(getUpstreamList(), ci);
                    }
                } else {
                    xList.push_back(upstreamNodes[i]);
                }
            }
            if (xList.size() == 1) {
                xList[0]->setValue(eval(getUpstreamList(), ci));
                return true;
            }
            return false;
            break;
        case XNOR:
            for (int i = 0; i < upstreamNodes.size(); i++) {
                if (upstreamNodes[i]->getValue() != X) {
                    if (ci == X) {ci = upstreamNodes[i]->getValue();}
                    else {ci = eval(ci, upstreamNodes[i]->getValue());}
                } else {
                    xList.push_back(upstreamNodes[i]);
                }
            }
            if (xList.size() == 1) {
                xList[0]->setValue(eval(~ci, this->value));
                return true;
            }
            return false;
            break;
        default:
            cout << "cktNode imply error\n";
            cout << gateType << "\n";
            return false;
            break;
    }
}

bool cktNode::isJustified() {
    return value != X;
}

void cktNode::reset() {
    value = X;
    stuckAt = false;
    tested = false;
}

void cktNode::resetValue() {
    value = X;
}

bool cktNode::isStuckAt() {
    return stuckAt;
}