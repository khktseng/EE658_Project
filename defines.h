/* useful macros and definitions */

#ifndef DEFINES_H
#define DEFINES_H

#define GETBIT(num, i) (((num) & (1 << i)) >> i)
#define MASK(num, mask) ((num) & (mask))
#define BIT0(num) ((num) & 0b01)
#define BIT1(num) ((num) & 0b10)

#define NUMFUNCS 17
#define MAXLINE 100               /* Input buffer size */
#define MAXNAME 31               /* File name size */
#define N_DROP	5	  	//Drop faults after detected this many times

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))



#endif