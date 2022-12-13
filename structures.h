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
   unsigned int logic3[3]; 	//  for 5-state logic; 0, 1, X, D, Dbar
   unsigned int fmask_AND; 	//  Fault Mask, AND
   unsigned int fmask_OR;	//  Fault Mask, OR
} NSTRUC;


typedef struct fault_struc{
	int ref;  	// line number(May be different from indx 
	bool stuckAt; 	// Stuck at 0 or 1
	vector<int> faultFound; // -1 = not found; else, is index of pattern
} FSTRUC;


#endif