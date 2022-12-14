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

//  gate types, Column 3 of circuit format	
enum e_logicType {
	zero = 0, 
	one = 1, 
	X = 2, 
	D = 3, 
	Dbar= 4, 
}; 

//  Logic Tables ////////////////////////
//  Col and Row Indexes:  0, 1, X, D, D'
const enum e_logicType AND_LOGIC5[5][5] = {
	{zero, 	zero, 	zero, 	zero, 	zero},
	{zero, 	one, 	X, 		D, 		Dbar},
	{zero, 	X,		X,		X,		X},
	{zero,	D,		X,		D,		zero},
	{zero,	Dbar,	X,		zero,	Dbar},
};
const enum e_logicType OR_LOGIC5[5][5] = {
	{zero, 	one, 	X, 		D, 		Dbar},
	{one, 	one, 	one, 	one, 	one},
	{X, 	one,	X,		X,		X},
	{D,		one,	X,		D,		one},
	{Dbar,	one,	X,		one,	Dbar},
};
const enum e_logicType XOR_LOGIC5[5][5] = {
	{zero, 	one, 	X, 		D, 		Dbar},
	{one, 	zero, 	X, 		Dbar, 	D},
	{X, 	X,		X,		X,		X},
	{D,		Dbar,	X,		zero,	one},
	{Dbar,	D,		X,		one,	zero},
};
const enum e_logicType NOT_LOGIC5[5] = 
	{one, 	zero, 	X, 		Dbar, 	D};

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


#endif