/* header for Fault class*/

#ifndef FAULT_H
#define FAULT_H

#include "includes.h"
#include "structures.h"
#include "Logic.h"
#include "cktNode.h"

class Fault {
    private:
        cktNode* node_a;
        int stuckAtValue;

    public:
        Fault(cktNode* na, int sav);
        cktNode* getNode() {return node_a;};
        int getSAV() {return stuckAtValue;};
};

typedef vector<Fault*> faultList;
typedef map<inputMap*, faultList*> faultMap;

#include "Fault.cpp"
#endif