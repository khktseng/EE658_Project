#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "includes.h"
#include "structures.h"
#include "cktNode.h"
#include "defines.h"

class Circuit {
    private:
        cktMap nodes;
        map<int, vector<cktNode*>> levNodes;
        int maxLevel;
        vector<cktNode*> PInodes;
        vector<cktNode*> POnodes;
        int numNodes;

        void linkNodes();
        void levelize(cktNode *currNode, int curr_level);
        void levelize();
        void verifyLink();
        void simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level);

    public:
        Circuit(string filename);
        int getMaxLevels(){return maxLevel;};
        vector<cktNode*> getPINodeList(){return PInodes;};
        vector<cktNode*> getPONodeList(){return POnodes;};
        void simulate(uint input);
        void reset();

};

#include "Circuit.cpp"
#endif