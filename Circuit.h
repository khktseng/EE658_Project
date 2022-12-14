#ifndef CIRCUIT_H
#define CIRCUIT_H

#include "includes.h"
#include "structures.h"
#include "cktNode.h"
#include "defines.h"
#include "Logic.h"
#include "Fault.h"

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
        void simulate(cktQ *toEvaluate, cktQ *notEvaluated, int level);
        faultList rflCheckpoint();
        faultMap deductiveFaultSim(faultList* fl, inputList* inputs);
        inputMap randomTestGen();

    public:
        Circuit(string filename);
        int getMaxLevels(){return maxLevel;};
        vector<cktNode*> getPINodeList(){return PInodes;};
        vector<cktNode*> getPONodeList(){return POnodes;};
        LOGIC getNodeLogic(int nodeID);
        faultList generateFaults(bool reduced);
        void addFault(Fault* fault);
        void simulate(map<int, LOGIC> *input);
        void reset();
        double faultCoverage(faultList* detectedFaults);
        inputList randomTestsGen(int numTests);
        

};

#include "Circuit.cpp"
#endif