/* cktNode class*/

#include "cktNode.h"

cktList cktNode::getUpstreamList(){return upstreamNodes;}
cktList cktNode::getDownstreamList(){return downstreamNodes;}

cktNode::cktNode(int id, int ln, gateT gt, nodeT nt, int fis, int fos, vector<int> usIDs) {
    nodeID = id;
    lineNum = ln;
    gateType = gt;
    nodeType = nt;
    numFanIns = fis;
    numFanOuts = fos;
    upstreamIDs = usIDs;
    level = -1;

    stuckAt = false;
    initialized= false;
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
        stuckAtValue = D;
    } else {
        stuckAtValue = DB;
    }
}

bool cktNode::evaluate() {
    assert(nodeType != PI);
    LOGIC newValue;
    
    if (gateType == NOT) newValue = ~upstreamNodes[0]->getValue();
    else newValue = upstreamNodes[0]->getValue();

    for (int i = 1; i < upstreamNodes.size(); i++) {
        switch (gateType){
            case XOR:
                newValue = newValue ^ upstreamNodes[i]->getValue();
                break;
            case OR:
                newValue = newValue | upstreamNodes[i]->getValue();
                break;
            case AND:
                newValue = newValue & upstreamNodes[i]->getValue();
                break;
            case XNOR:
                newValue = ~(newValue ^ upstreamNodes[i]->getValue());
                break;
            case NOR:
                newValue = ~(newValue | upstreamNodes[i]->getValue());
                break;
            case NAND:
                newValue = ~(newValue & upstreamNodes[i]->getValue());
                break;
            default:
                cout << "evaluate error\n";
                return false;
                break;
        }
    }

    if (!initialized || newValue != this->value) {
        this->value = newValue;
        return true;
    }
    return false;
}

void cktNode::reset() {
    initialized = false;
    stuckAt = false;
}
