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
        bool initialized;
        LOGIC value;
        bool stuckAt;
        LOGIC stuckAtValue;
        
    public:
        cktNode(int id, int ln, gateT gt, nodeT nt, int fis, int fos, vector<int> usIDs);
        int getNodeID() {return nodeID;};
        int getLineNum() {return lineNum;};
        gateT getGateType() {return gateType;};
        nodeT getNodeType() {return nodeType;};
        int getLevel() {return level;};
        int getNumFanIns() {return numFanIns;};
        int getNumFanOuts() {return numFanOuts;};
        LOGIC getValue() {return value;};
        LOGIC getStuckAtValue() {assert(stuckAt); return stuckAtValue;}

        vector<cktNode*> getUpstreamList();
        vector<cktNode*> getDownstreamList();

        void link(map<int, cktNode*> *nodes); // call : void link(cktMap *nodes)


        void setLevel(int l) {level = l;};
        void setValue(LOGIC v) {assert(nodeType==PI); value=v;};
        void setFault(int sav);
        void removeStuckAt() {stuckAt = false;}

        bool evaluate();    //true if value changed
        void reset();

};
typedef map<int, cktNode*> cktMap;
typedef vector<cktNode*> cktList;
typedef queue<cktNode*> cktQ;

#include "cktNode.cpp"
#endif