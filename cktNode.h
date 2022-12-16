#ifndef CKTNODE_H
#define CKTNODE_H

#include "includes.h"
#include "structures.h"
#include "defines.h"

class cktNode {
    private:
        int nodeID;
        int lineNum;
        gateT gateType;
        nodeT nodeType;
        int numFanIns;
        int numFanOuts;
        vector<int> upstreamIDs;
        vector<int> downstreamIDs;
        vector<cktNode*> upstreamNodes;
        vector<cktNode*> downstreamNodes;
        int level;
        bool linked;

        //mutable through public functions
        LOGIC value;
        bool stuckAt;
        LOGIC stuckAtValue;
        
        LOGIC eval(vector<LOGIC> args);
        LOGIC eval(vector<cktNode*> args);
        LOGIC eval(vector<cktNode*> args, LOGIC ci);
        LOGIC eval(LOGIC a, LOGIC b);
        bool implyFromOutput(LOGIC xSet);
        bool implyFromInputs(LOGIC xSet, bool NGate);
        queue<LOGIC> notXList();
        vector<LOGIC> getInputList();

    public:
        bool tested;

        cktNode(int id, int ln, gateT gt, nodeT nt, int fis, int fos, vector<int> usIDs);
        int getNodeID();
        int getLineNum();
        gateT getGateType();
        nodeT getNodeType() ;
        int getLevel();
        int getNumFanIns();
        int getNumFanOuts();
        LOGIC getValue();
        LOGIC getTrueValue();

        vector<cktNode*> getUpstreamList();
        vector<cktNode*> getDownstreamList();

        void link(map<int, cktNode*> *nodes); // call : void link(cktMap *nodes)


        void setLevel(int l) {level = l;};
        void setValue(LOGIC v);
        void setFault(int sav);
        void removeStuckAt() {stuckAt = false;}
        bool isJustified();
        bool isStuckAt();

        cktNode* getUnassignedInput(bool controllable);
        bool needAllInputs(LOGIC v);
        LOGIC getLikely(cktNode* input, LOGIC v);

        bool evaluate();    //true if value changed
        bool imply();       //true if inputs can be determined
        void reset();
        void resetValue();
        bool faultMatch();

};
typedef map<int, cktNode*> cktMap;
typedef vector<cktNode*> cktList;
typedef queue<cktNode*> cktQ;

inline ostream &operator<<(ostream &str, cktNode* node) {
    if (node->isStuckAt()){
        str << "<S ";    
    } else {
        str << "<N";
    }
    str << node->getNodeID() << "=" << node->getValue();
    str << " : " << node->getGateType();
    str << " of ";
    for (int i = 0; i < node->getUpstreamList().size(); i++) {
        str << node->getUpstreamList()[i]->getNodeID() << "=";
        str << node->getUpstreamList()[i]->getValue() << " ";
    }
    str << ">";
    return str;
}

#include "cktNode.cpp"
#endif