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

LOGIC cktNode::getValue() {
    if (stuckAt) return stuckAtValue; 
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
        }
    }

    linked = true;
}


void cktNode::setFault(int sav) {
    stuckAt = true;
    if (sav) {
        stuckAtValue = DB;
        value = ZERO;
    } else {
        stuckAtValue = D;
        value = ONE;
    }
}

// Evaluate the two logics based on gateType
LOGIC cktNode::eval(LOGIC a, LOGIC b) {
    switch (gateType){
            case XOR:
                return a ^ b;
                break;
            case OR:
                return a | b;
                break;
            case AND:
                return a & b;
                break;
            case XNOR:
                return a ^ b;
                break;
            case NOR:
                return ~(a | b);
                break;
            case NAND:
                return ~(a & b);
                break;
            default:
                cout << "evaluate error\n";
                return X;
                break;
        }
}


bool cktNode::evaluate() {
    assert(nodeType != PI);
    LOGIC newValue;
    
    if (gateType == NOT) newValue = ~upstreamNodes[0]->getValue();
    else newValue = upstreamNodes[0]->getValue();

    for (int i = 1; i < upstreamNodes.size(); i++) {
        newValue = eval(newValue, upstreamNodes[i]->getValue());
    }

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

bool cktNode::implyFromInputs(LOGIC xSet) {
    cktList xList;
    for (int i = 0; i < upstreamNodes.size(); i++) {
        if (upstreamNodes[i]->getValue() == xSet) {
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
                return implyFromInputs(ONE);
            }
            break;
        case NOR:
            if (this->getTrueValue() == ONE) {
                return implyFromOutput(ZERO);
            } else { // this.truevalue is ZERO 
                return implyFromInputs(ONE);
            }
            break;
        case AND:
            if (this->getTrueValue() == ONE) {
                return implyFromOutput(ONE);
            } else { // this.truevalue is ZERO
                return implyFromInputs(ZERO);
            }
            break;
        case NAND:
            if (this->getTrueValue() == ZERO) {
                return implyFromOutput(ONE);
            } else {
                return implyFromInputs(ZERO);
            }
        case XOR:
            for (int i = 0; i < upstreamNodes.size(); i++) {
                if (upstreamNodes[i]->getValue() != X) {
                    if (ci == X) {ci = upstreamNodes[i]->getValue();}
                    else {ci = eval(ci, upstreamNodes[i]->getValue());}
                } else {
                    xList.push_back(upstreamNodes[i]);
                }
            }
            if (xList.size() == 1) {
                xList[0]->setValue(eval(ci, this->value));
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
}
