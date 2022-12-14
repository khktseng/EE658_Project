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
} LOGIC;

inline LOGIC operator^(LOGIC const &a, LOGIC const &b) {
    return (LOGIC)((int)a^b);
}

inline LOGIC operator|(LOGIC const &a, LOGIC const &b) {
    if (a == ONE || b == ONE || 
        (a == D && b == DB) ||
        (a == DB && b == D)) {return ONE;}
    return (LOGIC)((int)a|b);
}

inline LOGIC operator~(LOGIC const &a) {
    return (LOGIC)(a ^ 0b01);
}

inline LOGIC operator&(LOGIC const &a, LOGIC const &b) {
    if ((LOGIC*)a == (LOGIC*)b) {return a;}
    if (a == ZERO || b == ZERO) {return ZERO;}
    return (LOGIC)(BIT1((int)a^b) + BIT0((int)a&b));
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
    }

    return str;
}

typedef map<int, LOGIC> inputMap;
typedef vector<inputMap*> inputList;

#endif