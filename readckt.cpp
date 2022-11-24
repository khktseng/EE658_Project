/*=======================================================================
Circuit Simulator and ATPG

  The circuit format (called "self" format) is based on outputs of
  a ISCAS 85 format translator written by Dr. Sandeep Gupta.
  The format uses only integers to represent circuit information.
  The format is as follows:

1        2        3        4           5           6 ...
------   -------  -------  ---------   --------    --------
0 GATE   outline  0 IPT    #_of_fout   #_of_fin    inlines
                  1 BRCH
                  2 XOR(currently not implemented)
                  3 OR
                  4 NOR
                  5 NOT
                  6 NAND
                  7 AND

1 PI     outline  0        #_of_fout   0

2 FB     outline  1 BRCH   inline

3 PO     outline  2 - 7    0           #_of_fin    inlines




                                    Authors: 
										Chihang Chen
										Kousthubh Dixit
										Kyle Tseng
										Kris Fausnight
                                    Date: Nov 22, 2022

=======================================================================*/


#include <cstdio>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <sstream>

using namespace std;

#define MAXLINE 100               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

enum e_state {EXEC, CKTLD};         /* Gstate values */
/* Node Type, column 1 of circuit format */
enum e_nodeType {
	GATE = 0, 
	PI = 1, 
	FB = 2, 
	PO = 3,
};    
//  gate types, Column 3 of circuit format	
enum e_gateType {
	IPT = 0, 
	BRCH = 1, 
	XOR = 2, 
	OR = 3, 
	NOR = 4, 
	NOT = 5, 
	NAND = 6, 
	AND = 7,
	XNOR = 8,
};  

struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   void (*fptr)(char *);             /* function pointer of the commands */
   enum e_state state;        /* execution state sequence */
};

typedef struct n_struc {
   unsigned indx;             /* node index(from 0 to NumOfLine - 1 */
   unsigned ref;              /* line number(May be different from indx */
   enum e_gateType gateType;         /* gate type */
   enum e_nodeType nodeType;         /* gate type */
   unsigned fin;              /* number of fanins */
   unsigned fout;             /* number of fanouts */
   vector<int> upNodes;
   vector<int> downNodes;
   int level;                 /* level of the gate output */
   int logic3[3]; 	//  for 5-state logic; 0, 1, X, D, Dbar
   int fmask_AND; 	//  Fault Mask, AND
   int fmask_OR;	//  Fault Mask, OR
} NSTRUC;                     

typedef struct fault_struc{
	unsigned ref;  	// line number(May be different from indx 
	bool stuckAt; 	// Stuck at 0 or 1
	vector<int> faultFound; // -1 = not found; else, is index of pattern
} FSTRUC;
/*----------------  Function Declaration -----------------*/
void allocate(void);
void clear(void);
const char *gname(int );
const char *nname(int );
void levelizeNodes(void);
void genNodeIndex(void);
//  Logic Simulation
void processNodeQueue(void);
void addPiNodesToQueue(void);
void addNodeToQueue(int );
void logicInit(void);
char getLogic(int , int);
bool simNode3(int);
void addOutputPattern(int);
void checkFaults(int,int, int);
//   Display printouts
void printTestPatterns(void);
void printFaultList(void);
//  Node Setup
void setPI_forPLS(int, int);
void setPI_forPFS(int);
void setFaults(int, int);
void resetFaultMasks(void);
//  File Read/Write
void readFaultList(char *);
bool readTestPatterns(char *);
void writeOutputPatterns(char *);
void writeFaultsDetected(char *);
void writeAllFaults(char *);

/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 9
void cread(char *);
void pc(char *);
void help(char *);
void quit(char *);
void lev(char *);
void logicSim(char *);
void printNode(char *);
void parallelFaultSimulation(char *);
struct cmdstruc command[NUMFUNCS] = {
	{"READ", cread, EXEC},
	{"PC", pc, CKTLD},
	{"HELP", help, EXEC},
	{"QUIT", quit, EXEC},
	{"LEV", lev, CKTLD},
	{"LOGICSIM", logicSim, CKTLD},
	{"PRINTNODE", printNode, CKTLD},
	{"PFS", parallelFaultSimulation, CKTLD},
	{"WRITEALLFAULTS", writeAllFaults, CKTLD},
};

/*----------------Global Variables-----------------------------------------*/
enum e_state Gstate = EXEC;     /* global exectution sequence */
int Nnodes;                     /* number of nodes */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int Ngates;
int Done = 0;                   /* status bit to terminate program */
char *circuitName;
char curFile[MAXNAME];			/* Name of current parsed file */
vector<NSTRUC> NodeV;
vector<int> PI_Nodes;
vector<int> PO_Nodes;
vector<int> ref2index;
vector< pair<int,int> > nodeQueue; //  First->level, second->node reference
const int bitWidth = 8*sizeof(int);
bool eventDriven = true;

//  Paralell Test Pattern Simulation, scratchpad
vector<int> PI_list;
vector<vector<char> > testPatterns;

//  Parallel Fault Simulation, scratchpad
//   Will re-use testPatterns variable
vector<FSTRUC> FaultV;

//  Varible to store PO outputs
vector<vector<char> > outputPatterns;

/*------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: shell
description:
  This is the main program of the simulator. It displays the prompt, reads
  and parses the user command, and calls the corresponding routines.
  Commands not reconized by the parser are passed along to the shell.
  The command is executed according to some pre-determined sequence.
  For example, we have to read in the circuit description file before any
  action commands.  The code uses "Gstate" to check the execution
  sequence.
  Pointers to functions are used to make function calls which makes the
  code short and clean.
-----------------------------------------------------------------------*/
int main()
{
   uint8_t com;
   char cline[MAXLINE], wstr[MAXLINE], *cp;
	
	printf("EE658 Fault/Logic Simulator, Group 14\n");
	printf("This processor bit width: %d\n", bitWidth);
	
   while(!Done) {
      printf("\nCommand>");
      fgets(cline, MAXLINE, stdin);
      if(sscanf(cline, "%s", wstr) != 1) continue;
      cp = wstr;
      while(*cp){
	*cp= Upcase(*cp);
	cp++;
      }
      cp = cline + strlen(wstr);
      //com = READ;
	  com = 0;
      while(com < NUMFUNCS && strcmp(wstr, command[com].name)) com++;
      if(com < NUMFUNCS) {
         if(command[com].state <= Gstate) (*command[com].fptr)(cp);
         else printf("Execution out of sequence!\n");
      }
      else system(cline);
   }
}
/*----------------------------------------------------*/
/*------Parallel Logic Simulation -------------------*/
/*----------------------------------------------------*/
void parallelFaultSimulation(char *cp)
{
	
	char patternFile[MAXLINE];
	char faultFile[MAXLINE];
	char outFile[MAXLINE];
	
	 //  Ensure all nodes are simulated
	 //  Necessary for paralell fault simulation
	eventDriven = false;
	
	//  Read in file names
	sscanf(cp, "%s %s %s", patternFile, faultFile, outFile);
	//Debug //////////
	printf("\nTest Patterns: %s\nFault List: %s\nOutput: %s;\n", patternFile, faultFile, outFile);
	////////////////////////////////////
	
	//  Read Fault List
	readFaultList(faultFile);
	//// --- Debug -------------
	printFaultList(); //  Debug
	// --------------------------
	
	//  Read Test Patterns; 1 or multiple
	//  Test patterns saved to:
	//     	vector<int> PI_list;
	//		vector<vector<char> > testPatterns;
	bool fileOK = readTestPatterns(patternFile);
	if(!fileOK){return;}
	
	//// --- Debug -------------
	printTestPatterns();
	// --------------------------
	
	//  Perform fault simulation
	//  Determine the # of passes to simulate all faults
	//  Can only do (32/64-1) faults
	//  first entry is always "no fault"
	int N_passes, N_faults, faults;
	int indStart, indEnd;
	N_faults = FaultV.size();
	N_passes = (N_faults+bitWidth-1)/(bitWidth-1); // Ceil function
	
	printf("\n %d fault patterns\n", testPatterns.size());
	
	for(int patt = 0;patt<testPatterns.size();++patt){

		
		for(int pass = 0;pass<N_passes;++pass){
			//  Find start and stop indexes in fault list for this pass
			indStart = pass*(bitWidth-1);
			indEnd = (pass+1)*(bitWidth-1)-1;
			indEnd = min(indEnd, (N_faults-1));
			printf("Pass %d; Fault list start: %d, end: %d\n",pass, indStart, indEnd);
			
			//  Set input pattern at PI's
			setPI_forPFS(patt);
			// Set all fault masks to known value
			resetFaultMasks();
			setFaults(indStart, indEnd);
			
			//  Add PI list to queue
			addPiNodesToQueue();
			
			// Simulate Circuit Logic
			// Process the Node queue; simulates logic of each node
			processNodeQueue();
			
			//  Check Results
			//  Records if faults are found
			checkFaults(indStart, indEnd, patt);
		}			
	}
	
	//  Done; write report
	writeFaultsDetected(outFile);
	
	
}

void setFaults(int indStart, int indEnd){
	int f;
	int ref;
	NSTRUC *np;
	FSTRUC *fp;
	int bitCounter = 1;//  Start at second bit
	for(f = indStart;f<=indEnd;f++){
		fp = &FaultV[f];
		np = &NodeV[ref2index[fp->ref]];
		if(fp->stuckAt){
			//  Stuck at 1
			np->fmask_OR = 1<<bitCounter;
		}else{
			//  Stuck at 0
			np->fmask_AND = ~(1<<bitCounter);
		}
		bitCounter++;
	}
	
	
}

//  
void checkFaults(int indStart,int indEnd, int patt){
	int f, temp;
	int nPo;
	bool detected;
	NSTRUC *np;
	FSTRUC *fp;
	int bitCounter = 1;//  Start at second bit
	for(f = indStart; f<=indEnd;++f){
		//  Cycle through each PO
		detected = false;
		for(nPo = 0;nPo<PO_Nodes.size();++nPo){
			//  Get pointer to a PO
			np = &NodeV[ref2index[PO_Nodes[nPo]]];
			//  Check if logic at bitCounter 
			//  is different than bit 0
			for(int k=0;k<3;k++){
				temp = np->logic3[k]& (1<<bitCounter);
				temp = temp>>bitCounter;
				detected = detected|((np->logic3[k]&1)!= temp);
			}
			
			if(detected){
				//printf("\nDetected fault %d",FaultV[f].ref);
				//  Mark this fault as detected
				fp = &FaultV[f];
				fp->faultFound.push_back(patt);
				//  No need to continue through remianing
				//  PO's.  Exit loop early
				break;
			}
		}//  Loop for each PO
		++bitCounter;
	}//  Loop for each fault
}
/*--------logicSim--------------------------------------------------------
input: "input file", "output file"
output: nothing
called by: shell
description:
	This function performs a parallel logic simulation on the currently loaded circuit.
	Assumption is that levelization has already been done.
	Input file ascii file, example:
	1,2,3,6,7
	1,1,0,0,1
	1,1,1,1,1
	0,0,0,0,0
	1,0,1,0,1
	0,1,0,1,0
	1,1,0,0,1
	1,0,1,1,0
	The header row is the reference # of the PI
	Each row is a different test pattern, logic corresponding 
	to the PI of the first row.
  
-----------------------------------------------------------------------*/
void logicSim(char *cp)
{
	// Perform a logic simulation
	//  Some counters and temp variables
	int j,k;
	
	// Run logic simulation in event-Driven mode
	eventDriven = true;
	
	//  Read File	
	//  Input in form "fileToRead.txt fileToWrite.txt"
	char patternFile[MAXLINE];
	char writeFile[MAXLINE];
	sscanf(cp, "%s %s", patternFile, writeFile);
	
	//Debug //////////
	printf("Read: %s, Write: %s;\n", patternFile, writeFile);
	////////////////
	
	//  Read Test Patterns; 1 or multiple
	//  Test patterns saved to:
	//     	vector<int> PI_list;
	//		vector<vector<char> > testPatterns;
	bool fileOK = readTestPatterns(patternFile);
	if(!fileOK){return;}
	
	//// --- Debug -------------
	printTestPatterns();
	// --------------------------
	
	// Set all fault masks to known value
	//  No faults are being simulated here
	resetFaultMasks();
	
	//  Clear output
	outputPatterns.clear();
	
	
	//  Determine the # of passes to simulate input
	//  Can only do 32/64 patterns per pass (Depending on word width)
	int N_passes, N_patterns, n_patterns;
	N_patterns = testPatterns.size();
	//  Get ceil of N_patterns/bitWidth;
	N_passes = (N_patterns+bitWidth-1)/bitWidth;
	int indStart, indEnd;
	int Pass, patt;
	//  Start going through the passes
	for(Pass=0;Pass<N_passes;Pass++){
		//  Find start and stop indexes in test pattern list for this pass
		indStart = Pass*bitWidth;
		indEnd = indStart+min(bitWidth, N_patterns-Pass*bitWidth) -1;
		//  Write test patterns to PI's
		setPI_forPLS(indStart, indEnd);

		//  Add PI list to queue
		addPiNodesToQueue();
		
		// Process the Node queue
		// Simulates logic of each node
		processNodeQueue();
		
		//  Add results to output array
		n_patterns = indEnd-indStart+1;
		addOutputPattern(n_patterns);
		
	}
	
	writeOutputPatterns(writeFile);
		
	//  Print "OK"
	printf("\n==> OK\n");
}

char getLogic(int nodeRef,int index){
	//  Returns an ascii character based on the logic at the specified index
	//  Logic is 3x32 or 3x64, each column is a result from a test pattern
	NSTRUC *np;
	bool log[3];
	np = &NodeV[ref2index[nodeRef]];
	int mask = 1<<index;
	for(int i = 0;i<3;i++){
		//  Isolate only the bit at index
		log[i] = (np->logic3[i] & mask);
	}
	if((log[0]==false) & (log[1]==false)){
		return '0';
	}
	if((log[0]==true) & (log[1]==true)){
		return '1';
	}
	if((log[0]==false) & (log[1]==true)){
		return 'X';
	}
	//  Unknown result, pass 'U'
	return 'U';
	
	
}

void setPI_forPFS(int patt){
	//  This function assigns the specified test patterns 
	//  to the PI's for paralell fault simulation
	//  Only 1 input pattern is used
	NSTRUC *np;
	int PI, k;
	//  Cycle through each PI and apply the test pattern

	for(int PI = 0;PI<PI_list.size();PI++){
		//  Get the current PI
		np = &NodeV[ref2index[PI_list[PI]]];

		//Get logic value of this PI for this test pattern
		char logic = testPatterns[patt][PI];
		switch (logic){
			case '0':
				//0;0;0
				np->logic3[0] = 0;
				np->logic3[1] = 0;
				np->logic3[2] = 0;
				break;
			case '1':
				// 1;1;0
				np->logic3[0] = ~0;
				np->logic3[1] = ~0;
				np->logic3[2] = 0;
				break;
			default:
				// 0;1;0
				np->logic3[0] = 0;
				np->logic3[1] = ~0;
				np->logic3[2] = 0;
		}
	}// End loop for each PI
	
}

void setPI_forPLS(int indStart, int indEnd){
	//  This function assigns the specified test patterns 
	//  to the PI's for paralell logic simulation
	//  0 to 32/64 input patterns are used
	NSTRUC *np;
	int PI, k, patt;
	//  Cycle through each PI and apply all of the test patterns
	//  simultaneously (max of 32 or 64 )
	for(int PI = 0;PI<PI_list.size();PI++){
		//  Get the current PI
		np = &NodeV[ref2index[PI_list[PI]]];
		//  Set to initial state of 'X'
		np->logic3[0] = 0;
		np->logic3[1] = ~0;// set to all 1's
		np->logic3[2] = 0;
		for(int patt =indStart;patt<=indEnd;patt++){
			//Cycle through each test pattern
			//Get logic value of this PI for this test pattern
			char logic = testPatterns[patt][PI];
			//Left<<shift adds 0 to the LSB
			np->logic3[0] = np->logic3[0]<<1;
			np->logic3[1] = np->logic3[1]<<1;
			np->logic3[2] = np->logic3[2]<<1;
			switch (logic){
				case '0':
					//  Nothing to do here; already set to 
					// 0;0;0
					break;
				case '1':
					// 1;1;0
					np->logic3[0] += 1;
					np->logic3[1] += 1;
					break;
				default:
					// 0;1;0
					np->logic3[1] += 1;
			}
		}//  End loop for each test pattern
	}// End loop for each PI
}




void addNodeToQueue(int nodeRef){

	//  First determine if this node is already in the queue
	vector< pair<int,int> > ::iterator qIter;
	for(qIter = nodeQueue.begin(); qIter!=nodeQueue.end();qIter++){
		if(qIter->second==nodeRef){
			//  Already in queue, exit
		return;
		}
	}

	
	//  Get pointer to this node
	NSTRUC *np;
	np = &NodeV[ref2index[nodeRef]];
	//  Add it to the queue
	//  First item is level, second is reference
	nodeQueue.push_back(make_pair(np->level, np->ref));
	//  Sort by the level, largest to smallest
	sort(nodeQueue.rbegin(), nodeQueue.rend());
}

void addPiNodesToQueue(void){
	nodeQueue.clear();
	for(int j=0;j<PI_list.size();j++){
		addNodeToQueue(PI_list[j]);
	}
}



void processNodeQueue(void){
	//  Function to process all of the nodes in the queue
	//  Need to determine current logic of these nodes based on the inputs
	//  Assumes levelization has already occurred.
	NSTRUC *np;
	bool logicChanged;
	while (nodeQueue.size()>0){
		//  Get pointer to the last node in the queue
		//  This will have the lowest level
		np = &NodeV[ref2index[nodeQueue.back().second]];
		nodeQueue.pop_back();//  Remove this node
		//  Perform logic simulation
		//printf("Processing Node %d\n",np->ref);
		logicChanged = simNode3(np->ref);
		//  Now add all downstream nodes to the queue if the logic changed
		//  If not event-driven, add all nodes regardless
		if ((logicChanged)||(eventDriven == false)){
			for(int k = 0;k<np->fout;k++){
				addNodeToQueue(np->downNodes[k]);
			}	
		}
	}
	
}
bool simNode3(int nodeRef){
	//  Function to determine current logic of this node
	//  based on logic of upstream nodes.
	//  Uses 3-bit logic; currently only 0, 1, X
	//  Output True:   Node logic has changed
	//  Output False:  Nodelogic has not changed
	NSTRUC *np;
	NSTRUC *npIn_A;
	NSTRUC *npIn_B;
	//  Misc Counters
	int i, k, inp;
	
	// Placeholders for logic
	int oldLogic[3], temp0, temp1;
	
	//  Get pointer to current node
	np = &NodeV[ref2index[nodeRef]];
	int N_inputs = np->upNodes.size();
	
	//  Check if this is a PI
	if((np->gateType == IPT)||(N_inputs==0)){
		//  Input; logic already set
		//  Nothing to do here except apply fault mask
		//  Fault Application
		for(k=0;k<2;k++){
			np->logic3[k] = np->logic3[k] & np->fmask_AND;
			np->logic3[k] = np->logic3[k] | np->fmask_OR;		
		}
		//  return true to ensure downstream nodes are processed.
		return true;
	}

	//  Save current logic to determine if it has changed later
	for(k=0;k<3;k++){
		oldLogic[k] = np->logic3[k];
	}
	
	
	
	//  Simulate logic ////////////////////
	//  Get first input node
	npIn_A = &NodeV[ref2index[np->upNodes[0]]];
	switch (np-> gateType){
		case IPT:
			//  Nothing to do here
			break;
		case BRCH:
			//  Set equal to the input
			//  Should only be 1 input
			for(k=0;k<3;k++){
				np->logic3[k] = npIn_A->logic3[k];
			}
			break;
		case XOR:
		case XNOR:
			//  Assume only 2 inputs possible
			npIn_B = &NodeV[ref2index[np->upNodes[1]]];
			for(k=0;k<3;k++){
				// Bitwise XOR
				np->logic3[k] = npIn_A->logic3[k] ^ npIn_B->logic3[k];
			}
			//  Fix for inversion with 'X'
			temp0 = np->logic3[0];
			temp1 = np->logic3[1];
			np->logic3[0] = temp0 & temp1;
			np->logic3[1] = temp0 | temp1;
			//  Fix for scenario inp A and B = 'X';
			temp0 = np->logic3[1];
			np->logic3[2] = ((npIn_A->logic3[0]^npIn_A->logic3[1])&
				(npIn_B->logic3[0]^npIn_B->logic3[1]))|temp0;
			break;
		case AND:
		case NAND:
			if(N_inputs==1){
				//  Special condition; must treat "AND" gate as a buffer with 1 input
				for(k=0;k<3;k++){
					np->logic3[k] = npIn_A->logic3[k];
				}
			}else{
				for (inp = 1;inp<N_inputs;inp++){
					npIn_B = &NodeV[ref2index[np->upNodes[inp]]];
					//  Bitwise &
					for(k=0;k<3;k++){
						np->logic3[k] = npIn_A->logic3[k] & npIn_B->logic3[k];
					}
					//  For more than 2 inputs, set first input equal to 
					//  previous result.
					npIn_A = np;
				}
			}
			break;
		case OR:
		case NOR:
			for (inp = 1;inp<N_inputs;inp++){
				npIn_B = &NodeV[ref2index[np->upNodes[inp]]];
				for(k=0;k<3;k++){
					//  Bitwise OR
					np->logic3[k] = npIn_A->logic3[k] | npIn_B->logic3[k];
				}
				//  For more than 2 inputs, set first input equal to 
				//  previous result.
				npIn_A = np;
			}
			break;
		case NOT:
			//  Just copy over; inversion handled in next step
			for(k=0;k<3;k++){
				np->logic3[k] = npIn_A->logic3[k];
			}
			break;
		default:
			printf("Node type %d not recognized\n",np->gateType);
	}
	
	//  Output Inversion
	if((np->gateType == XNOR)||
		(np->gateType==NAND) ||
		(np->gateType==NOR) ||
		(np->gateType==NOT))
	{
		//  Invert logic; C1 = NOT(A2), C2 = NOT(A1)
		temp0 = np->logic3[0];
		np->logic3[0] = ~np->logic3[1];
		np->logic3[1] = ~temp0;
	}
	
	//  Fault Application
	for(k=0;k<2;k++){
		np->logic3[k] = np->logic3[k] & np->fmask_AND;
		np->logic3[k] = np->logic3[k] | np->fmask_OR;		
	}
	
	//////////////////////////debug //////////////////////
	/*
	char refStr[10];
	printf("Processing Node:  %s, %d\n",gname(np->gateType), np->ref);
	sprintf(refStr,"%d", np->ref);
	printNode(refStr);
	printf("\nInputs:");
	printf("\n");
	for(k=0;k<np->upNodes.size();k++){
		sprintf(refStr,"%d", np->upNodes[k]);
		printNode(refStr);
	}
	printf("\n\n\n");
	*/
	//////////////////////////debug //////////////////////
	
	//  Function output to see if logic has changed
	for(k=0;k<3;k++){
		if(oldLogic[k] != np->logic3[k]){
			return true;
		}
	}
	return false;
	
	
}

void resetFaultMasks(void){
	vector<NSTRUC>::iterator nodeIter;
	for(nodeIter = NodeV.begin(); nodeIter!=NodeV.end();nodeIter++){
		nodeIter->fmask_AND = ~0;//  Set to all 1's
		nodeIter->fmask_OR = 0;
	}
}
void addOutputPattern(int n_patterns){
	//  Write outputs to outputPatterns
	//  For paralell processing, specify the # of patterns
	vector<char> tempOutput;
	
	//  Cycle through each pattern.
	for(int patt=n_patterns-1;patt>=0;patt--){
		tempOutput.clear();
		for(int k = 0;k<PO_Nodes.size();k++){
			//  Use the "getLogic" to convert 5-value logic to a character
			tempOutput.push_back(getLogic(PO_Nodes[k], patt));
		}
		outputPatterns.push_back(tempOutput);
	}
	
}


/*-----------------------------------------------------------------------
input: circuit description file name
output: nothing
called by: main
description:
  This routine reads in the circuit description file and set up all the
  required data structure.
-----------------------------------------------------------------------*/
void cread(char *cp)
{
	char buf[MAXLINE];
	int  i, j, k, ref = 0;
	FILE *fd;
	enum e_nodeType nodeType;
	enum e_gateType gateType;
	
	
	vector<NSTRUC>::iterator nodeIter;

	sscanf(cp, "%s", buf);
	if((fd = fopen(buf,"r")) == NULL) {
		printf("File %s does not exist!\n", buf);
		return;
	}
	
	//  Enter filename into global filename variable
	strcpy(curFile, buf);
	
	//  If another circuit is already loaded, clear it
	if(Gstate >= CKTLD) clear();
	
	//  Step through each line and parse the node	
	fseek(fd, 0L, 0);
	int index=0; //  Counter for current index
	while(fscanf(fd, "%d %d", &nodeType, &ref) != EOF) {
		//  New Node
		NSTRUC tempNode;
		tempNode.ref = ref;
		tempNode.level = -1;
		//  Unknown state, 010
		tempNode.logic3[0] = 0;
		tempNode.logic3[1] = ~0;//  Set to all 1's
		tempNode.logic3[2] = 0;
		tempNode.indx = index++;
		tempNode.nodeType = nodeType;

		//  Parse in node
		switch(nodeType) {
			case PI:

			case PO:

			case GATE:
				fscanf(fd, "%d %d %d", &tempNode.gateType, &tempNode.fout, &tempNode.fin);
				break;
				
			case FB:
				tempNode.fout = tempNode.fin = 1;
				fscanf(fd, "%d", &tempNode.gateType);
				break;

			default:
				printf("Unknown node type!\n");
				exit(-1);
        }

		for(i=0;i<tempNode.fin;i++){
			fscanf(fd, "%d", &ref);
			tempNode.upNodes.push_back( ref);
		}
		//  Add new node
		NodeV.push_back(tempNode);
    }
	//  Done processing through file
	fclose(fd);
	
	//  Generate node indexes 
	genNodeIndex();
	
	//  Count node types
	Nnodes = Npi = Npo   = Ngates =0; // Global variables
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter)
	{
		Nnodes++;
		if(nodeIter->gateType>1){
			Ngates++;
		}
		if(nodeIter->nodeType == PI){
			PI_Nodes.push_back(nodeIter->ref);
			Npi++;
		}else if (nodeIter->nodeType == PO){
			PO_Nodes.push_back(nodeIter->ref);
			Npo++;
		}
	}

	//  Create the downstream/output node list for each node	
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		for(j=0;j<nodeIter->fin;j++){
			int ref = nodeIter->upNodes[j];
			NodeV[ref2index[ref]].downNodes.push_back(nodeIter->ref);
		}
	}
	//  Set fin and fout
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		nodeIter->fin = nodeIter->upNodes.size();
		nodeIter->fout = nodeIter->downNodes.size();
	}
	
	// Set all fault masks to known value
	resetFaultMasks();
	
	//  Levelize Nodes
	levelizeNodes();
   
	//  Done; circuit is loaded
	Gstate = CKTLD;
	printf("==> OK\n");
}

void genNodeIndex(void){
	//  Regenerates reference vectors of the global NodeV vector
	//  ref2index supplies the index for a given reference #
	
	vector<NSTRUC>::iterator nodeIter;
	int i;
	
	//  Clear vectors first
	ref2index.clear();
	

	//  Find the maximum value of the reference #
	int maxRef = 0;
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		if(nodeIter->ref>maxRef){
			maxRef = nodeIter->ref;
		}
	}
	
	//  Create the ref2index array
	//  First initialize with 0;
	for(i=0;i<=maxRef;i++){
		ref2index.push_back(0);
	}
	//  Now enter the reference numbers
	i = 0;
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		ref2index[nodeIter->ref] = i++;
	}

}


/*---------------- Levelization ------------------------------------------
input: string for name of file to be written
output: void
called by: main
description:
  Levelize the nodes in the circuit.  PI is 0
-----------------------------------------------------------------------*/
int getLevel(int nodeRef){
	//  Quick function to return the level of a specified node
	
	NSTRUC *nodePtr;
	nodePtr = &NodeV[ref2index[nodeRef]];

	return nodePtr->level;
}

void levelizeNodes(void)
{
	//NSTRUC *np;
	
	vector<NSTRUC>::iterator np;
	
	//  Levelization algorithm	
	int noAction;  //  Indicator to see if any levels are applied
	while(1){
		noAction = 1;
		for(np=NodeV.begin();np!=NodeV.end();++np){
			//  Check to see if level is applied
			if(np->level<0){
				//  No level applied yet
				//  See if level can be applied
				if(np->nodeType == PI){
					// Easy; this is a primary input so assign "0"
					np->level = 0;//  New level applied!
					noAction = 0;
				}else{
					//  Check to see if all of the inputs are levelized yet
					int maxLevel = -1;
					for(int j = 0; j<np->fin; j++){
						int upLevel = getLevel(np->upNodes[j]);
						if (upLevel<0){
							//  1 input not levelized yet; no need to continue
							maxLevel = -1; //  To indicate failure to find level
							break;
						}else{
							maxLevel = (maxLevel>upLevel) ? maxLevel : upLevel;
						}
					}
					//  If valid, update level
					if(maxLevel>=0){
						//  Current level is the maximum of the input levels +1
						np->level = maxLevel+1;
						noAction = 0;//  New level applied!
					}
					
				}
			}
		}
		//  See if any new levels were applied.  
		//  If no new levels were applied, you are either done or stuck 
		if (noAction) break;
	}
	

	
}


void lev(char *cp)
{
	int i, j, k;
	//NSTRUC *np;
	vector<NSTRUC>::iterator np;
   
	//  Code to get name of circuit from file-name
	//  Example:  Converts "./circuits/c17.ckt" to "c17"
	char *strPtr;
    while(1)
    {
        strPtr= strchr(curFile,'/');
        if((strPtr!=NULL) && (strPtr<(curFile+MAXNAME-2))){
			//  Trim to everything after the '/' character
            strcpy(curFile, strPtr+1);
        }else{
			//  '/' character not found; all done
            break;
        }
    }
    strPtr = strstr(curFile,".ckt");
    if((strPtr!=NULL)&&(strPtr>curFile)){
		//  if the ".ckt" string is found,
		//  set its location to null to mark end of string
        *strPtr = 0;
    }
	
	
	levelizeNodes();
	
	//  Write File
	FILE *fptr;
	char buf[MAXLINE];
	sscanf(cp, "%s", buf);
	if((fptr = fopen(buf,"w")) == NULL) {
		printf("File %s cannot be written!\n", buf);
		return;
	}else{
		printf("==> Writing file: %s\n",buf);
	}
	//  Begin writing file
	fprintf(fptr,"%s\n", curFile);
	fprintf(fptr,"#PI: %d\n#PO: %d\n",Npi, Npo);
	fprintf(fptr,"#Nodes: %d\n#Gates: %d\n",Nnodes, Ngates);
	for(np=NodeV.begin();np!=NodeV.end();np++){		
		fprintf(fptr,"%d %d\n",np->ref, np->level);
	}
	//  Done writing file
	fclose(fptr);
	
	//  Print OK to console
	printf("==> OK\n");
	
}
/*=============================================*/
//  Read/Write File Functions
/*============================================*/
void readFaultList(char *faultFile){
	//  Read the Fault List
	//  Format Example:
	//   1@0
	//   2@1
	//   21@0
	//   ...
	FILE *fd;
	if((fd = fopen(faultFile,"r")) == NULL) {
		printf("\nFile %s cannot be opened\n", faultFile);
		return;
	}
	FaultV.clear();
	FSTRUC tempFault;
	tempFault.faultFound.clear();
	while(fscanf(fd, "%d@%d", &tempFault.ref, &tempFault.stuckAt) != EOF){
		FaultV.push_back(tempFault);
	}
	fclose(fd);
	
}
bool readTestPatterns(char *patternFile){
	//  Read test patternf ile
	//  First line is list of PI's
	//  Subsequent lines are logic corresponding to those PI's.
	fstream fptrIn;
	string lineStr;
	fptrIn.open(patternFile, ios::in);
	char foo;
	int temp;
	vector<char> tempPattern;
	
	//  Open file
	if(!fptrIn.is_open()){
		printf("File %s cannot be read!\n", patternFile);
		return false;
	}
	PI_list.clear();//  Clear list of PI's
	testPatterns.clear();//  Clear loaded test patterns
	bool firstLine = true;
	while(getline(fptrIn, lineStr)){
		if(firstLine){
			// First line is list of PI's
			stringstream ss(lineStr);
			while(ss>>temp){ //  Convert ascii to integer
				//  Add this PI reference to the list
				PI_list.push_back(temp);
				ss>>foo;//  remove comma
			}
			firstLine = false;
		}else{
			// Subsequent lines are test patterns
			tempPattern.clear();// pattern instance
			int i = 0;
			while(i<lineStr.size()){
				tempPattern.push_back(lineStr[i]);
				i=i+2;//  skip the comma
			}
			//  Add new pattern to vector of patterns
			testPatterns.push_back(tempPattern);			
		}
	}
	fptrIn.close();
	
	
	
	//  Data Quality Check /////////////////////////////////////////
	//  Check that inputs are valid
	for(int j=0;j<PI_list.size();j++){
		//  Check that PI reference does not exceed length of node vector
		if(PI_list[j]>(ref2index.size()-1)){
			printf("\nWarning, input file %s contains inputs exceeding range\n", patternFile);
			return false;
		}
		//  Check that node is actually a PI
		if(NodeV[ref2index[PI_list[j]]].nodeType != PI){
			printf("\nWarning, input file %s contains inputs that are not PI\n", patternFile);
			return false;
		}
	}
	for(int j=0;j<testPatterns.size();j++){
		//  Check that each test pattern size is equal to the PI list size
		if(testPatterns[j].size()!=PI_list.size()){
			printf("\nWarning, input file %s test pattern size does not match PI size",patternFile);
			return false;
		}
	}
	/////////////////////////////////////////////////////////
	
	return true;
}

void writeOutputPatterns(char *fileName){
	FILE *fptrOut;
	if((fptrOut = fopen(fileName,"w")) == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
	}else{
		printf("==> Writing file of PO outputs: %s\n",fileName);
	}
	
	//  Write header
	//  First line is list of PO's
	for(int k = 0;k<PO_Nodes.size();k++){
		fprintf(fptrOut,"%d",PO_Nodes[k]);
		//  If this is the last entry, line return else comma
		if(k<(PO_Nodes.size()-1)){
			fprintf(fptrOut,","); 
		}else{
			fprintf(fptrOut,"\n");
		}
	}
	
	//  Write each output pattern
	for(int patt=0;patt<outputPatterns.size();++patt){
		for(int k = 0;k<outputPatterns[patt].size();k++){
			//  Write each value
			fprintf(fptrOut,"%c",outputPatterns[patt][k]);
			//  Print a , or a line-return if this is last entry
			if(k<(PO_Nodes.size()-1)){
				fprintf(fptrOut,",");
			}else{
				fprintf(fptrOut,"\n");
			}
		}
	}	
	
	//  Done writing file
	fclose(fptrOut);
	
}

void writeFaultsDetected(char *fileName){
	//  Write output from pfs
	FILE *fptrOut;
	FSTRUC *fp;
	
	if((fptrOut = fopen(fileName,"w")) == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
	}else{
		printf("==> Writing file of PO outputs: %s\n",fileName);
	}
	
	//  Cycle through fault list and mark if they are found
	for(int i=0;i<FaultV.size();++i){
		fp = &FaultV[i];
		if(fp->faultFound.size()>0){
			fprintf(fptrOut,"%d@%d\n",fp->ref, fp->stuckAt);
		}
	}
		
	//  Done writing file
	fclose(fptrOut);
}

void writeAllFaults(char *cp){
	//  Function to write all possible faults 
	//  For debug
	FILE *fptr;
	NSTRUC *np;
	char fileName[MAXLINE];
	
	sscanf(cp, "%s", fileName);
	
	if((fptr = fopen(fileName,"w")) == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
	}else{
		printf("==> Writing file of PO outputs: %s\n",fileName);
	}
	
	//  Cycle through fault list and mark if they are found
	for(int i=0;i<NodeV.size();++i){
		np = &NodeV[i];
		for(int s = 0;s<2;++s){
			fprintf(fptr,"%d@%d\n",np->ref, s);
		}
	}	
	//  Done writing file
	fclose(fptr);
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  Set Done to 1 which will terminates the program.
-----------------------------------------------------------------------*/
void quit(char*)
{
   Done = 1;
}

/*======================================================================*/

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
  This routine clears the memory space occupied by the previous circuit
  before reading in new one. It frees up the dynamic arrays Node.unodes,
  Node.dnodes, Node.flist, Node, Pinput, Poutput, and Tap.
-----------------------------------------------------------------------*/
void clear(void)
{

   
   NodeV.clear();
   FaultV.clear();
   PI_Nodes.clear();
   PO_Nodes.clear();
   Nnodes = 0;
   Npi = 0;
   Npo = 0;
   ref2index.clear();
   PI_list.clear();
   testPatterns.clear();
   Gstate = EXEC;
}




/*-----------   Display Functions-----------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/

void pc(char *cp)
{
	int i, j;
   
	vector<NSTRUC>::iterator np;


   
	printf("Node    Logic   Level   Type    In      \t\t\tOut\n");
	printf("------  ------  ------  ------  ------  \t\t\t-------\n");
	for(np=NodeV.begin();np!=NodeV.end();np++) {
      
		printf("\t\t\t\t\t\t\t\t");
		for(j = 0; j<np->fout; j++) 
			printf("%d ",np->downNodes[j]);
		printf("\r%5d   %d       %5d   %s\t", np->ref, np->logic3[0], np->level, gname(np->gateType));
		for(j = 0; j<np->fin; j++) 
			printf("%d ",np->upNodes[j]);
		printf("\n");
	}
   
	printf("Primary inputs:  ");
    for(i=0;i<PI_Nodes.size();i++) {
		printf("%d ",PI_Nodes[i]);
	}
	printf("\n");
	printf("Primary outputs: ");
	for(i=0;i<PO_Nodes.size();i++) {
		printf("%d ",PO_Nodes[i]);
	}
	printf("\n\n");
	printf("Number of nodes = %d\n", Nnodes);
	printf("Number of primary inputs = %d\n", Npi);
	printf("Number of primary outputs = %d\n", Npo);
   
}

void printNode(char *nodeStr){
	//  Simple function to print the node
	//  >>printnode 22
	int a[64], i, k, nodeRef;
	int n;
	
	stringstream temp(nodeStr);
	temp>>nodeRef;
	
	if(nodeRef>(ref2index.size()-1)){
		printf("\n Node %d out of range\n",nodeRef);
		return;
	}
	NSTRUC *np;
	np = &NodeV[ref2index[nodeRef]];
	printf("Node %d:\n",np->ref);
	printf("  Gate Type: %s\n",gname(np->gateType));
	printf("  Node Type: %s\n",nname(np->nodeType));	
	printf("  Level: %d\n", np->level);
	printf("  Logic:\n   ");
	for(k=0;k<3;k++){
		n = np->logic3[k];
		// Loop to calculate and store the binary format
		for (i = 0; i<bitWidth; i++) {
			a[bitWidth-i-1] = n & 1;
			n = n>>1;
		}
		// Loop to print the binary format of given number
		for (i = 0;i<bitWidth;++i) 
		{
			printf("%d", a[i]);
		}
		printf("\n   ");
	}
	
}

void printTestPatterns(void){
	int j, k;
	printf("PI List:\n");
	for(k=0;k<PI_list.size();k++){
		printf("%d,",PI_list[k]);
	}
	printf("\nTest Patterns:\n");
	for(j=0;j<testPatterns.size();j++){
		for(k=0;k<testPatterns[j].size();k++){
			printf("%c,",testPatterns[j][k]);
		}
		printf("\n");
	}
	
}

void printFaultList(void){
	vector<FSTRUC>::iterator iterF;
	printf("\nFault List:\n");
	for(iterF = FaultV.begin();iterF!=FaultV.end();++iterF){
		printf("%d@%d\n",iterF->ref, iterF->stuckAt);
		
	}
	
}



const char *gname(int tp)
{
   switch(tp) {
      case 0: return("PI");
      case 1: return("BRANCH");
      case 2: return("XOR");
      case 3: return("OR");
      case 4: return("NOR");
      case 5: return("NOT");
      case 6: return("NAND");
      case 7: return("AND");
	  case 8: return("XNOR");
   }
}

const char *nname(int tp)
{
   switch(tp) {
      case 0: return("GATE");
      case 1: return("PI");
      case 2: return("FB");
      case 3: return("PO");
   }
}

void help(char*)
{
   printf("READ filename - ");
   printf("read in circuit file and creat all data structures\n");
   printf("PC - ");
   printf("print circuit information\n");
   printf("HELP - ");
   printf("print this help information\n");
   printf("LEV filename - ");
   printf("Levelize the currently loaded circuit, writes levels to 'filename'\n");
   printf("LOGICSIM filename1 filename2 - ");
   printf("Perform logic simulation; read PI inputs from 'filename1', writes PO outputs to 'filename2'\n");
   printf("PRINTNODE node# - ");
   printf("Prints the information for the node specified\n");
   printf("QUIT - ");
   printf("stop and exit\n");
}


/*========================= End of program ============================*/

