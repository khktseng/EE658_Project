#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "includes.h"
#include "structures.h"
#include "cktNode.h"
#include "defines.h"
#include "Fault.h"

typedef struct objective_s{
    cktNode* node;
    LOGIC targetValue;
} OBJECTIVE;

class Circuit {
    private:
        cktMap nodes;
        map<int, cktList> levNodes;
        int maxLevel;
        cktList PInodes;
        cktList POnodes;
        cktList FBnodes;
        int numNodes;
        bool initialized;

        void linkNodes();
        void levelize(cktNode *currNode, int curr_level);
        void levelize();
        void verifyLink();
        void simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level, cktList* dFrontier);
        faultList rflCheckpoint();
        
        inputMap* randomTestGen();
        bool podem(Fault* fault, cktList* dFrontier);
        OBJECTIVE objective(cktList* dFrontier);
        OBJECTIVE backtrace(OBJECTIVE kv);
        void backwardsImplication(cktNode* root);
        bool faultAtPO();
        cktNode getNode(int nodeID);
        bool xPathCheck(cktNode* node);
        bool xPathCheck(cktList* dFrontier);
        void placeholder();
        void resetPO();

    public:
        Circuit(string filename);

        void        addFault(Fault* fault);
        void        atpg();
        Fault*      createFault(int nodeID, int sav);
        faultMap*   deductiveFaultSim(faultList* fl, inputList* inputs);
        double      faultCoverage(faultList* detectedFaults);
        faultList   generateFaults(bool reduced);
        LOGIC       getNodeLogic(int nodeID);
        vector<int> getNodeIDs();
        int         getMaxLevels(){return maxLevel;};
        vector<cktNode*> getPINodeList(){return PInodes;};
        vector<cktNode*> getPONodeList(){return POnodes;};

        inputList   randomTestsGen(int numTests);
        inputMap*   PODEM(Fault* fault);
        void        printPO();
        void        reset();
        void        simulate(map<int, LOGIC> *input);     
};

#include "Circuit.cpp"
#endif