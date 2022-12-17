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
        string cktName;
        cktMap nodes;
        map<int, cktList> levNodes;
        int maxLevel;
        cktList PInodes;
        cktList POnodes;
        cktList FBnodes;
        int numNodes;
        int numGates;
        bool initialized;
        char cstringName[MAXLINE];

        void linkNodes();
        void levelize(cktNode *currNode, int curr_level);
        void levelize();
        void verifyLink();
        void simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level, cktList* dFrontier);
        faultSet rflCheckpoint();
        
        inputMap* randomTestGen();
        bool podem(Fault* fault, cktList* dFrontier);
        OBJECTIVE objective(cktList* dFrontier);
        OBJECTIVE backtrace(OBJECTIVE kv);
        void backwardsImplication(cktNode* root);
        bool faultAtPO();
        
        bool xPathCheck(cktNode* node);
        bool xPathCheck(cktList* dFrontier);
        void placeholder();
        void resetPO();

    public:
        Circuit(char* filename);

        cktNode     getNode(int nodeID);
        void        addFault(Fault* fault);
        double      atpg(inputList* testVectors);
        double      atpg_det(inputList* testVectors);
        Fault*      createFault(int nodeID, int sav);
        faultMap*   deductiveFaultSim(faultSet* fl, inputList* inputs);
        double      faultCoverage(faultSet* detectedFaults);
        faultSet    generateFaults(bool reduced);
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

        inline cktMap getNodes() {return nodes;};     
        inline int getNumPI() {return PInodes.size();};
        inline int getNumPO() {return POnodes.size();};
        inline int getNumNodes() {return nodes.size();};
        inline int getNumGates() {return numGates;};
        string getCktName();
        inputMap * createInputVector(vector<int> nodeIDs, vector<int> inputs);
        vector<int> getPOs();
        
};

#include "Circuit.cpp"
#endif