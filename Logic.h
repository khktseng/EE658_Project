/* enum for logic and overloaded operators
*/
#ifndef LOGIC_H
#define LOGIC_H

#include "defines.h"

// logic enum
typedef enum e_logic : unsigned long{
    ZERO = 0,
    ONE,
    D,
    DB,
    X,
} LOGIC;

inline LOGIC operator^(LOGIC const &a, LOGIC const &b) {
    if (a == X || b == X) {
        return X;
    }
    return (LOGIC)((int)a^b);
}

inline LOGIC operator|(LOGIC const &a, LOGIC const &b) {
    if (a == ONE || b == ONE || 
        (a == D && b == DB) ||
        (a == DB && b == D)) {return ONE;}
    if (a == X || b == X) {return X;}
    return (LOGIC)((int)a|b);
}

inline LOGIC operator~(LOGIC const &a) {
    if (a == X) return X;
    return (LOGIC)(a ^ 0b01);
}

inline LOGIC operator&(LOGIC const &a, LOGIC const &b) {
    if (a == ZERO || b == ZERO) {return ZERO;}
    if (a == X || b == X) {return X;}
    if (a == ONE) {return b;}
    if (b == ONE) {return a;}
    return ZERO;
}

inline ostream &operator<<(ostream &str, LOGIC l) {
    switch(l) {
        case ZERO:
            str << "0";
            break;
        case ONE:
            str << "1";
            break;
        case D:
            str << "D";
            break;
        case DB:
            str << "DB";
            break;
        case X:
            str << "X";
            break;
    }

    return str;
}

typedef map<int, LOGIC> inputMap;
typedef vector<inputMap*> inputList;
typedef queue<inputMap*> inputQ;

inline ostream &operator<<(ostream &str, inputMap* ins) {
    str << "<";
    for (inputMap::iterator it = ins->begin(); it != ins->end(); ++it) {
        str << it->first << "=" << it->second << ", ";
    }
    str << "\b\b";
    str << ">";

    return str;
}

#endif
