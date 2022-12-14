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
        vector<cktNode*> upstreamNodes;
        vector<cktNode*> downstreamNodes;
        int level;
        bool linked;

        //mutable through public functions
        LOGIC value;
        bool stuckAt;
        LOGIC stuckAtValue;
        
        LOGIC eval(LOGIC a, LOGIC b);
        bool implyFromOutput(LOGIC xSet);
        bool implyFromInputs(LOGIC xSet);

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

        cktNode* getUnassignedInput(bool controllable);
        bool needAllInputs(LOGIC v);
        LOGIC getLikely(cktNode* input, LOGIC v);

        bool evaluate();    //true if value changed
        bool imply();       //true if inputs can be determined
        void reset();

};
typedef map<int, cktNode*> cktMap;
typedef vector<cktNode*> cktList;
typedef queue<cktNode*> cktQ;

#include "cktNode.cpp"
#endif