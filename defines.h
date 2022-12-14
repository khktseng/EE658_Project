/* useful macros and definitions */

#ifndef DEFINES_H
#define DEFINES_H

#define GETBIT(num, i) (((num) & (1 << i)) >> i)
#define MASK(num, mask) ((num) & (mask))
#define BIT0(num) ((num) & 0b01)
#define BIT1(num) ((num) & 0b10)

#endif