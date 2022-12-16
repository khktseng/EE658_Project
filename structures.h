/* structs used in all files*/
#ifndef STRUCTS_H
#define STRUCTS_H

#include "includes.h"
#include "Logic.h"

/* Node Type, column 1 of circuit format */
typedef enum e_nodeType {
	GATE = 0, 
	PI = 1, 
	FB = 2, 
	PO = 3,
} nodeT;

//  gate types, Column 3 of circuit format	
typedef enum e_gateType {
	IPT = 0, 
	BRCH = 1, 
	XOR = 2, 
	OR = 3, 
	NOR = 4, 
	NOT = 5, 
	NAND = 6, 
	AND = 7,
	XNOR = 8,
} gateT;
inline ostream &operator<<(ostream &str, gateT gate) {
	switch (gate) {
		case IPT: str << "IPT"; 	break;
		case BRCH: str << "BRCH";	break;
		case XOR: str << "XOR";		break;
		case OR: str << "OR";		break;
		case NOR: str << "NOR";		break;
		case NAND: str <<"NAND";	break;
		case AND: str << "AND";		break;
		case XNOR: str << "XNOR";	break;
		case NOT: str << "NOT";		break;
		default: str << "GATET_X";	break;
	}
	return str;
}
//  gate types, Column 3 of circuit format	
enum e_logicType {
	zero = 0, 
	one = 1, 
	x = 2, 
	d = 3, 
	dbar= 4, 
}; 

//  Logic Tables ////////////////////////
//  Col and Row Indexes:  0, 1, x, d, d'
const enum e_logicType AND_LOGIC5[5][5] = {
	{zero, 	zero, 	zero, 	zero, 	zero},
	{zero, 	one, 	x, 		d, 		dbar},
	{zero, 	x,		x,		x,		x},
	{zero,	d,		x,		d,		zero},
	{zero,	dbar,	x,		zero,	dbar},
};
const enum e_logicType OR_LOGIC5[5][5] = {
	{zero, 	one, 	x, 		d, 		dbar},
	{one, 	one, 	one, 	one, 	one},
	{x, 	one,	x,		x,		x},
	{d,		one,	x,		d,		one},
	{dbar,	one,	x,		one,	dbar},
};
const enum e_logicType XOR_LOGIC5[5][5] = {
	{zero, 	one, 	x, 		d, 		dbar},
	{one, 	zero, 	x, 		dbar, 	d},
	{x, 	x,		x,		x,		x},
	{d,		dbar,	x,		zero,	one},
	{dbar,	d,		x,		one,	zero},
};
const enum e_logicType NOT_LOGIC5[5] = 
	{one, 	zero, 	x, 		dbar, 	d};

typedef struct n_struc {
   int indx;             /* node index(from 0 to NumOfLine - 1 */
   int ref;              /* line number(May be different from indx */
   enum e_gateType gateType;         /* gate type */
   enum e_nodeType nodeType;         /* node type */
   int fin;              /* number of fanins */
   int fout;             /* number of fanouts */
   vector<int> upNodes;
   vector<int> downNodes;
   int level;                 /* level of the gate output */
   bool logic;
   unsigned int logic3[3]; 	//  For PFS and PLS, 3-logic 0, 1, X
   enum e_logicType logic5; // 5-value logic Dalg and maybe PODEM
   unsigned int fmask_AND; 	//  Fault Mask, AND
   unsigned int fmask_OR;	//  Fault Mask, OR
} NSTRUC;


typedef struct fault_struc{
	int ref;  	// line number(May be different from indx 
	bool stuckAt; 	// Stuck at 0 or 1
	vector<int> faultFound; // -1 = not found; else, is index of pattern
} FSTRUC;

const int bitWidth = 8*sizeof(int);

#endif