/* Fault Class*/

#include "Fault.h"

Fault::Fault(cktNode* na, int sav) {
    this->node_a = na;
    this->stuckAtValue = sav;
}