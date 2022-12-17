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

inline ostream &operator<<(ostream &str, Fault* fault) {
    str << "<F@ " << fault->getNode()->getNodeID() << ", sav=" << fault->getSAV()<< ">";
    return str;
}



typedef vector<Fault*> faultList;
typedef set<Fault*> faultSet;
typedef map<inputMap*, faultList*> faultMap;
typedef queue<Fault*> faultQ;

#include "Fault.cpp"
#endif