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
#include <stdlib.h>     
#include <time.h>  
#include <set>
#include <map>

using namespace std;

#define MAXLINE 100               /* Input buffer size */
#define MAXNAME 31               /* File name size */
#define N_DROP	5	  	//Drop faults after detected this many times

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


struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   void (*fptr)(char *);             /* function pointer of the commands */
   enum e_state state;        /* execution state sequence */
};

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
	vector<int> faultFound; // empty if not found, otherwise index of input patterns
} FSTRUC;
/*----------------  Function Declaration -----------------*/
void clear(void);
const char *gname(int );
const char *nname(int );
const char *logicname(int );
void levelizeNodes(void);
void genNodeIndex(void);
int intCeil(int,int);
NSTRUC *getNodePtr(int);
//  Logic Simulation
void processNodeQueue(void);
void simNode(int );
void single_dfs(int );

//void dfs_logicSim(char *, int);
void dfs_logicSim( int);
void addPiNodesToQueue(void);
void addNodeToQueue(vector< pair<int,int> >&, int );
void logicInit(void);
char getLogic(int , int);
bool simNode3(int);
void addOutputPattern(int);
void checkFaults(int,int, int);
void genRandomInputs(int);
void genAllFaults(void);
float getFaultCoverage(void);
void parallelFaultSimulation(void);
void parallelLogicSimulation(void);
void dropFaults(void);
void reduced_fault_list(void);
//   Display printouts
void printInputPatterns(void);
void printFaultList(void);
//  Node Setup
void setPI_forPLS(int, int);
void setPI_forPFS(int);
void setFaults(int, int);
void resetFaultMasks(void);
//  D Algorithm
bool setup_Dalg(void);
void resetNodes_Dalg(void);
bool D_algorithm(void);
bool imply_check_Dalg(void);
int nInputsX_Dalg(int);
int propogate_Jfrontier_Dalg(int , int );
void propogate_Dfrontier_Dalg(int , bool );
void saveState_Dalg(set<int>& , set<int>& , vector< pair<int,enum e_logicType> >& );
void reloadState_Dalg(set<int>&, set<int>& , vector< pair<int,enum e_logicType> >& );
bool isErrAtPO_Dalg(void);
bool forwardImply_Dalg(void);
bool backwardsImply_Dalg(void);
void backward_logic(int , bool &, vector<int> &);
enum e_logicType checkLogic_Dalg(int, bool &, bool &, bool &);
void branchPropogate_Dalg(int ,enum e_logicType,  int );
void printNode_Dalg(int );
void printFrontiers_Dalg(void);

//  File Read/Write
bool readFaultList(char *);
bool readTestPatterns(char *);
void writeOutputPatterns(char *);
void writeInputPatterns(char *, bool);
void writeFaultsDetected(char *);
void writeAllFaults(char *);
void writeFaultCoverageReport(vector<float>,char *);
void getCircuitNameFromFile(char *);
void writeSingleReport_Dalg(void);

/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 14
void cread(char *);
void pc(char *);
void help(char *);
void quit(char *);
void lev(char *);
void logicSim(char *);
void logicInit(void);
void rfl(char *);
void multi_dfs(char*);
void printNode(char *);
void pfs(char *);
void randomTestGenerator(char *);
void DALG(char *);
void DALG_DEBUG(char *);

struct cmdstruc command[NUMFUNCS] = {
	{"READ", cread, EXEC},
	{"PC", pc, CKTLD},
	{"HELP", help, EXEC},
	{"QUIT", quit, EXEC},
	{"LEV", lev, CKTLD},
	{"LOGICSIM", logicSim, CKTLD},
	{"RFL", rfl, CKTLD},
	{"DFS", multi_dfs, CKTLD},
	{"PRINTNODE", printNode, CKTLD},
	{"PFS", pfs, CKTLD},
	{"RTG", randomTestGenerator,CKTLD},
	{"WRITEALLFAULTS", writeAllFaults, CKTLD},
	{"DALG", DALG, CKTLD},
	{"DALG_DEBUG", DALG_DEBUG, CKTLD},
};

/*----------------Global Variables-----------------------------------------*/
enum e_state Gstate = EXEC;     /* global exectution sequence */
int Nnodes;                     /* number of nodes */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int Ngates;
int Done = 0;                   /* status bit to terminate program */
char *circuitName;
char currentCircuit[MAXNAME];			/* Name of current circuit ex: "c17" */

int dfs_count = 1;
vector<NSTRUC> NodeV;
vector<int> PI_Nodes;
vector<int> PO_Nodes;
vector<int> index2ref;
vector<int> ref2index;

//  DFS variables
map<int, vector<int> > dfs_fault_list;
//map<int, int> fault_vals, previous_logic;
map<int, int> fault_vals;
vector<int> final, prevLogic;
vector<string> file_output;
vector<int> reVisit;

//  Logic Simulation
vector<int> PI_list;
vector<vector<char> > inputPatterns;
//vector<vector<int> > int_inputPatterns;

// D Algorithm
set<int> D_frontier;
set<int> J_frontier;
vector< pair<int,int> > nodeQueueForward; //  First->level, second->node reference
vector< pair<int,int> > nodeQueueBackward; //  First->level, second->node reference
int faultyNode_Dalg;
bool stuckAt_Dalg;
int debugMode_Dalg = 0;


vector< pair<int,int> > nodeQueue; //  First->level, second->node reference
const int bitWidth = 8*sizeof(int);
bool eventDriven = true;

//  Fault Lists
vector<FSTRUC> FaultV;
vector<FSTRUC> FaultV_Dropped;

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
	
	/* initialize random seed: */
	srand(time(NULL));
	
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

/*------------------------------------------------------------*/
/*------  D-Algorithm  ---------------------------------------*/
/*------------------------------------------------------------*/
void DALG(char *cp)
{
	NSTRUC *np;
	int stuckAt;
	char outFile[MAXNAME];
	
	//  Read in file names
	//  global variable "faultyNode_Dalg"
	sscanf(cp, "%d %d", &faultyNode_Dalg, &stuckAt);
	
	//  Get name of output report for results of this algorithm.
	sprintf(outFile,"%s_DALG_%d@%d",currentCircuit, faultyNode_Dalg, stuckAt);
	
	if(stuckAt>0){
		stuckAt_Dalg = true;
	}else{
		stuckAt_Dalg = false;
	}
	
	//Debug //////////
	printf("\nD-Algorithm\n");
	printf("Node %d stuck at %d\n",faultyNode_Dalg, stuckAt_Dalg);
	printf("Output File: %s;\n", outFile);
	////////////////////////////////////
	
	//  Setup for D algorithm
	bool isValid;
	isValid = setup_Dalg();
	if(!isValid){
		printf("Node %d is not a valid node.  Exiting function\n", faultyNode_Dalg);
		return;
	}
	
	
	//  Run D Algorithm
	bool success_dalg;
	success_dalg = D_algorithm();
	
	//  Print message to console
	if(!success_dalg){
		if(debugMode_Dalg>0){
			printf("Failed D Algorithm\n");
		}
		
	}else{
		if(debugMode_Dalg>0){
			printf("Completed D Algorithm\n");
			//  Print primary Inputs
			printf("Primary Inputs\n");
			for(int i = 0;i<PI_Nodes.size();++i){
				printf("%d, ", PI_Nodes[i]);
			}
			printf("\n");
			for(int i = 0;i<PI_Nodes.size();++i){
				np = getNodePtr(PI_Nodes[i]);
				printf("%s, ", logicname(np->logic5));
			}	
			//  Print Primary Outputs
			printf("Primary Outputs\n");
			for(int i = 0;i<PO_Nodes.size();++i){
				printf("%d, ", PO_Nodes[i]);
			}
			printf("\n");
			for(int i = 0;i<PO_Nodes.size();++i){
				np = getNodePtr(PO_Nodes[i]);
				printf("%s, ", logicname(np->logic5));
			}
		}
		
		writeSingleReport_Dalg();
	}
	
	
}

bool setup_Dalg(void){
	//  Setup to before starting D algorithm
	
	//  First check that this node is valid
	bool isValid = false;
	for(int i = 0;i<NodeV.size();++i){
		if(NodeV[i].ref == faultyNode_Dalg){
			isValid = true;
			break;
		}
	}
	if(!isValid){
		//  Invalid node, cannot proceed
		return false;
	}
	
	//  Reset frontier arrays
	D_frontier.clear();
	J_frontier.clear();
	resetNodes_Dalg();
	
	//  Assign Fault
	NSTRUC *np;
	np = getNodePtr(faultyNode_Dalg);
	if(stuckAt_Dalg){
		//  Normally 0, stuck at 1.  Dbar = 0/1
		np->logic5 = Dbar;
	}else{
		//  Normally 1, stuck at 0.  D = 1/0
		np->logic5 = D;
	}
	
	//  To start process, add this node to the queue
	if(debugMode_Dalg>1){
		printf("Adding node %d to forward queue\n", faultyNode_Dalg);
		printf("Adding node %d to backward queue\n", faultyNode_Dalg);
	}
	addNodeToQueue(nodeQueueForward, faultyNode_Dalg);
	addNodeToQueue(nodeQueueBackward, faultyNode_Dalg);
	
	return true;
	
}

bool D_algorithm(void){
	NSTRUC *np;
	bool success, isErrAtPO;
	set<int> tryList;
	set<int>::iterator Dptr, Jptr;
	int nodeD, nodeJ;
	//  Placeholders for current circuit state in case backtracking is needed
	set<int> J_front_save;
	set<int> D_front_save;
	vector< pair<int,enum e_logicType> > logicState;
	
	if(debugMode_Dalg>1){
		printf("Starting Recursive D Algorithm\n");
	}
	
	//  Imply and Check.  Return failure if this fails
	//  Process all nodes in the queue forward and backwards
	while((nodeQueueForward.size()>0)||(nodeQueueBackward.size()>0)){
		success = forwardImply_Dalg();
		if(!success){
			if(debugMode_Dalg>1){
				printf("Failed forward imply\n");
			}
			return false;
		}
		success = backwardsImply_Dalg();
		if(!success){
			if(debugMode_Dalg>1){
				printf("Failed backwards imply\n");
			}
			return false;
		}
	}
	
	if(debugMode_Dalg>1){
		printFrontiers_Dalg();
	}
	
	
	//  D Frontier Handling
	//  Is the error at the Primary Outputs?
	isErrAtPO = isErrAtPO_Dalg();
	//  Try to propogate error to Primary Outputs
	if(!isErrAtPO){
		if(debugMode_Dalg>1){
			printf("Error Not at PO\n");
		}
		//  If D frontier is empty, encountered failure
		if(D_frontier.size()==0){
			if(debugMode_Dalg>1){
				printf("Failed D Algorithm; D_frontier is empty\n");
			}
			return false;
		}
		//  Try each node in the D-frontier
		tryList.clear();
		//  Try each node in the D frontier		
		Dptr = D_frontier.begin();
		while(Dptr!=D_frontier.end()){
			//  See if this node has been tried
			nodeD = *Dptr;
			if(debugMode_Dalg>1){
				printf("Trying D Frontier Node %d\n", nodeD);
			}
			set<int>::iterator it = tryList.find(nodeD);
			if(it==tryList.end()){
				//  This node has not been tried yet
				tryList.insert(nodeD);
				//  Save this current state in case backtracking is needed
				saveState_Dalg(J_front_save, D_front_save, logicState);
				//  Set inputs to propogate D
				//  Also adds nodes to the process queue
				propogate_Dfrontier_Dalg(nodeD, true);
				//  See if it was succesful
				success = D_algorithm();
				if (success){
					return true;
				}else{
					if(debugMode_Dalg>1){
						printf("Failed propogate D frontier at node %d; backtracking\n", nodeD);
					}
					reloadState_Dalg(J_front_save, D_front_save, logicState);
					//  Special case for XOR; need to try other condition
					//  Set inputs to propogate D
					propogate_Dfrontier_Dalg(nodeD, false);
					success = D_algorithm();
					if (success){
						return true;
					}else{
						if(debugMode_Dalg>1){
							printf("Failed propogate D frontier at node %d with alt XOR; backtracking\n", nodeD);
						}
						
						reloadState_Dalg(J_front_save, D_front_save, logicState);
					}
				}
				Dptr = D_frontier.begin();
			}else{
				Dptr++;
			}
		}
		if(debugMode_Dalg>1){
			printf("Tried all D-frontier and failed\n");
		}
		
		return false;
	}
	
	
	
	//  J Frontier Handling
	if(J_frontier.size()==0){
		//  Job is done!  Return success
		return true;
	}
	//  Try each node in the D-frontier
	tryList.clear();
	int k = 0;
	//  Try each node in the D frontier	
	Jptr = 	J_frontier.begin();
	while(Jptr!=J_frontier.end()){
		//  See if this node has been tried
		nodeJ = *Jptr;
		if(tryList.find(nodeJ)==tryList.end()){
			//  This node has not been tried yet
			tryList.insert(nodeJ);
			
			while(nInputsX_Dalg>0){
				//  Save this current state in case backtracking is needed
				saveState_Dalg(J_front_save, D_front_save, logicState);
				//  Set inputs to backwards propogate J
				//  Also adds nodes to the process queue
				int gateRef = propogate_Jfrontier_Dalg(nodeJ, -1);
				//  See if it was succesful
				success = D_algorithm();
				if(success){
					return success;
				}else{
					if(debugMode_Dalg>1){
						printf("Failed propogate J frontier at node %d; backtracking\n", nodeJ);
					}
					
					reloadState_Dalg(J_front_save, D_front_save, logicState);				
					propogate_Jfrontier_Dalg(nodeJ, gateRef);
					
				}
							
			}
			Jptr = 	J_frontier.begin();;//  Start again at front of J-frontier
		}else{
			Jptr++;
		}
	}
	return false;
	
}




int propogate_Jfrontier_Dalg(int nodeRef, int gateRef){
	//  Assign controlling value to gate of this J-frontier node
	NSTRUC *np, *npUp;
	int gateToSet;
	enum e_logicType logicSet;
	
	np = getNodePtr(nodeRef);
	
	//  Find gate to be set
	if(gateRef>=0){
		//  Setting previous decision back to non-controlling value
		gateToSet = gateRef;
		switch(np->gateType){
			case AND:
			case NAND:
				logicSet=one;
				break;
			case OR:
			case NOR:
			case XOR:
			case XNOR:
				logicSet=zero;
				break;
			default:
				printf("\nError, propogate Jfrontier at node %d with uknown gate type %s\n", np->ref, gname(np->gateType));
				logicSet=zero;
		}
		
	}else{
		//  Find a input which is "X" and set it to a controlling value
		gateToSet = 0;
		for(int i=0;i<np->upNodes.size();++i){
			npUp = getNodePtr(np->upNodes[i]);
			if(npUp->logic5 == X){
					gateToSet = i;
					break;
			}
		}
		switch(np->gateType){
			case AND:
			case NAND:
				logicSet=zero;
				break;
			case OR:
			case NOR:
			case XOR:
			case XNOR:
				logicSet=one;
				break;
			default:
				printf("\nError, propogate Jfrontier at node %d with uknown gate type %s\n", np->ref, gname(np->gateType));
				logicSet=zero;
		}
	}
	
	//  Set logic of this upstream node and add to queue to process
	npUp = getNodePtr(np->upNodes[gateToSet]);
	npUp->logic5 = logicSet;
	if(debugMode_Dalg>1){
		printf("Adding node %d to backward queue\n", npUp->ref);
		printf("Adding node %d to backward queue\n", np->ref); 
	}
	addNodeToQueue(nodeQueueBackward, npUp->ref);
	addNodeToQueue(nodeQueueBackward, np->ref);//  Also re-evaluate thsi node
	if(debugMode_Dalg>1){
		printf("Propogating J-Frontier\n   Node %d assigning %s to upstream node %d\n",
				np->ref, logicname(logicSet), npUp->ref);
	}
	

	//  Return the gate that was set in case this decision must be reversed
	return gateToSet;
	
	
}

void propogate_Dfrontier_Dalg(int nodeRef, bool XOR_version){
	//  Assign non-controlling values to all gates of this D-frontier node
	NSTRUC *np, *npUp;
	
	np = getNodePtr(nodeRef);
	
	//  Find each "X" value in the upstream nodes
	for(int k=0;k<np->upNodes.size();++k){
		npUp = getNodePtr(np->upNodes[k]);
		if(npUp->logic5==X){
			switch (np->gateType){
				case AND:
				case NAND:
					npUp->logic5 = one;
					break;
				case OR:
				case NOR:
					npUp->logic5 = zero;
					break;
				case XOR:
				case XNOR:
					if(XOR_version){
						npUp->logic5 = one;
					}else{
						npUp->logic5 = zero;
					}
					break;
				default:
					printf("\nNode %d is in the D frontier and should not be\n",nodeRef);
			}
			if(debugMode_Dalg>1){
				printf("Propogating D-Frontier\n   Node %d assigning %s to upstream node %d\n",
					np->ref, logicname(npUp->logic5), npUp->ref);
			}
			
			//  Now add this upstream node to the queue to be backwards-processed
			if(debugMode_Dalg>1){
				printf("Adding node %d to backward queue\n", npUp->ref);
			}
			addNodeToQueue(nodeQueueBackward, npUp->ref);
		}		
	}
	
	//  Now add this  node to the queue to be forward-processed
	if(debugMode_Dalg>1){
		printf("Adding node %d to forward queue\n", np->ref);
	}
	addNodeToQueue(nodeQueueForward, np->ref);
	
}

bool forwardImply_Dalg(void){
	NSTRUC *np;
	int nodeRef;
	bool isOK, isJ, isD;
	enum e_logicType newLogic;
	
	
	
	
	while(nodeQueueForward.size()>0){
		//  Get pointer to the last node in the queue
		//  This will have the lowest level
		nodeRef = nodeQueueForward.back().second;
		nodeQueueForward.pop_back();//  Remove this node
		np = getNodePtr(nodeRef);
		
		if(debugMode_Dalg>1){
			printf("\nForward Imply, Node %d\n",nodeRef);
			printNode_Dalg(nodeRef);
		}
				
		
		//  Sim node and determine if there is a conflict
		//  Also determine if this is a J or D frontier
		newLogic = checkLogic_Dalg(nodeRef, isOK, isJ, isD);
		if(!isOK){
			printf("Failed Forward Imply; node %d, logic conflict\n", nodeRef);
			return false;
		}
		if(isJ){
			printf("Unexpected:  Forward Imply, found J Frontier on %d\n",nodeRef);
			//J_frontier.push_back(nodeRef);
		}else if(isD){
			//  New D frontier
			if(debugMode_Dalg>1){
				printf("Adding node %d to D frontier",nodeRef);
			}
			
			D_frontier.insert(nodeRef);
		}else{
			//  Erase this node if it's already in the D frontier
			D_frontier.erase(nodeRef);
			//  Propogate new value forward
			//  Only propogate if the logic has changed or this is the faulty node
			
			if((np->logic5 != newLogic)||(np->ref == faultyNode_Dalg)){
				if(np->ref != faultyNode_Dalg){
					if(debugMode_Dalg>1){
						printf("Node %d, logic change from %s to %s\n", np->ref, logicname(np->logic5), logicname(newLogic));
					}
					np->logic5 = newLogic;
				}
				
				
				if(debugMode_Dalg>1){
					printf("Adding downstream nodes to queue:\n");
				}
				for(int i=0;i<np->downNodes.size();++i){
					if(debugMode_Dalg>1){
						printf("  Adding node %d to forward queue\n",np->downNodes[i]);
					}
					addNodeToQueue(nodeQueueForward, np->downNodes[i]);
				}
			}
			
		}
	}
	return true;
}

bool backwardsImply_Dalg(void){
	NSTRUC *np, *npUp, *npDown;
	int nodeRef;
	bool isOK, isJ, isD;
	enum e_logicType newLogic;
	
	
	while(nodeQueueBackward.size()>0){
		//  Get pointer to the first node in the queue
		//  This will have the highest level
		nodeRef = nodeQueueBackward.front().second;
		nodeQueueBackward.erase(nodeQueueBackward.begin());//  Remove this node
		np = getNodePtr(nodeRef);
		
		if(debugMode_Dalg>1){
			printf("\nBackwards Imply, Node %d\n",nodeRef);
			printNode_Dalg(nodeRef);
		}
		
		
		//  No need to process anything, go to next node
		if(np->nodeType==PI){
			if(debugMode_Dalg>1){
				printf("Backwards Imply, reached PI node %d; no need to continue\n",np->ref);
			}
			
			continue;
		}
		
		//  If this is a branch, propagate through upstream and downstream nodes
		if(np->gateType==BRCH){
			if(debugMode_Dalg>1){
				printf("Branch Node %d Reached, applying downstream logic\n",np->ref);
			}
			//  Branches can only have 1 input
			branchPropogate_Dalg(np->ref, np->logic5, np->downNodes[0]);
			
			continue;
		}
				
		//  Sim node and determine if there is a conflict
		//  Only good for determining if there is a conflict; isJ and isD ignored
		newLogic = checkLogic_Dalg(nodeRef, isOK, isJ, isD);
		if(!isOK){
			if(debugMode_Dalg>1){
				printf("Failed Backwards Imply; node %d, logic conflict\n", nodeRef);
			}
			return false;
		}

		
		//  Main logic function
		vector<int> inputChangedList;
		
		if(newLogic!=np->logic5){
			//  Special handling for faulty node
			if(np->ref == faultyNode_Dalg){
				if(newLogic==X){
					backward_logic(nodeRef, isJ, inputChangedList);
				}
			}else{
				backward_logic(nodeRef, isJ, inputChangedList);
			}
		}else{
			isJ = false;
		}
		
		
		
		//
		
		if(isJ){
			// This node is a J-frontier, can't go any further
			if(debugMode_Dalg>1){
				printf("  Found new J-frontier, node %d\n", nodeRef);
			}
			J_frontier.insert(nodeRef);
		}else{
			// This node is not a J-frontier; continue upstream
			//  Erase this from the J_frontier in case it is there
			J_frontier.erase(nodeRef);
			//  Push all the downstream nodes to the queue
			if(debugMode_Dalg>1){
				printf("  Not a J frontier; adding nodes to backward queue:\n", nodeRef);
			}
			for(int i = 0;i<inputChangedList.size();++i){
				if(debugMode_Dalg>1){
					printf("  Adding node %d to backward queue\n",inputChangedList[i]);
				}
				addNodeToQueue(nodeQueueBackward, inputChangedList[i]);		
			}
		}
	}
	
	return true;
}

void backward_logic(int nodeRef, bool &isJ, vector<int>& inputChangedList){
	//  Main logic function to backwards propogate a given gate logic output
	//  to the gate upstream nodes
	NSTRUC *np, *npUp;
	enum e_logicType logUp, logNew, logEval;
	bool foo;
	
	// Current gate being evaluated
	np = getNodePtr(nodeRef);
	
	
	//  Determine logic output for this gate
	//  0 or 1 if it is the faulty node, otherwise the current logic
	if(np->ref == faultyNode_Dalg){
		if(np->logic5 == D){
			logEval = one;
		}else{
			logEval = zero;
		}
	}else{
		logEval = np->logic5;
	}
	
	//  Flip if necessary
	if((np->gateType==NOT)||(np->gateType==NOR)||
		(np->gateType==NAND)||(np->gateType==XNOR)){
		logEval = NOT_LOGIC5[logEval];
	}
	
	
	//  Easy solution for single input buffer or NOT gate
	if(np->upNodes.size()==1){
		npUp = getNodePtr(np->upNodes[0]);
		npUp->logic5 = logEval;
		if(debugMode_Dalg>1){
			printf("Applying logic, Node: %d = %s\n", npUp->ref, logicname(npUp->logic5));
		}
		inputChangedList.push_back(npUp->ref);
		return;
	}
	
	
	//  Sanity Check
	if((logEval!=one)&&(logEval!=zero)){
		printf("Unexpected Behavior, backward logic of:%s node: %d\n", logicname(logEval), nodeRef);
	}
	
	
	//  Get all the input logics
	vector<enum e_logicType>inArr;
	for(int i=0;i<np->upNodes.size();i++){
		npUp = getNodePtr(np->upNodes[i]);
		inArr.push_back(npUp->logic5);
	}
	//  Get the # of "X" inputs to the current node
	int nX = nInputsX_Dalg(nodeRef);  // Count the # of X's
	
	//  AND/NAND gate handling
	if((np->gateType==AND)||(np->gateType==NAND)){
		if(logEval==one){
			//  Output is 1; Set to all inputs to 1
			isJ = false;
			for(int i=0;i<np->upNodes.size();i++){
				npUp = getNodePtr(np->upNodes[i]);
				if(npUp->logic5 == X){
					npUp->logic5 = one;
					if(debugMode_Dalg>1){
						printf("Applying logic, Node: %d = %s\n", npUp->ref, logicname(npUp->logic5));
					}
					inputChangedList.push_back(npUp->ref);
				}
				
			}
		}else if(nX==1){
			//  Output is zero and only 1 unknown input
			//  Assign 0 to only unknown input
			isJ = false;
			for(int i=0;i<np->upNodes.size();i++){
				npUp = getNodePtr(np->upNodes[i]);
				if(npUp->logic5 == X){
					npUp->logic5 = zero;
					if(debugMode_Dalg>1){
						printf("Applying logic, Node: %d = %s\n", npUp->ref, logicname(npUp->logic5));
					}
					inputChangedList.push_back(npUp->ref);
				}
			}
		}else{
			//  Output is zero and more than 1 unknown inputs
			//  Choice must be made; add to J frontier
			isJ = true;
		}
		return;
	}
	
	//  OR/NOR gate handling
	if((np->gateType==OR)||(np->gateType==NOR)){
		if(logEval==zero){
			//  Output is 0; set all inputs to 0
			isJ = false;
			for(int i=0;i<np->upNodes.size();i++){
				npUp = getNodePtr(np->upNodes[i]);
				if(npUp->logic5 == X){
					npUp->logic5 = zero;
					if(debugMode_Dalg>1){
						printf("Applying logic, Node: %d = %s\n", npUp->ref, logicname(npUp->logic5));
					}
					inputChangedList.push_back(npUp->ref);
				}
			}
		}else if(nX==1){
			//  Output is 1 and only 1 unkown;
			//  Set unknown to "1";
			isJ = false;
			for(int i=0;i<np->upNodes.size();i++){
				npUp = getNodePtr(np->upNodes[i]);
				if(npUp->logic5 == X){
					npUp->logic5 = one;
					if(debugMode_Dalg>1){
						printf("Applying logic, Node: %d = %s\n", npUp->ref, logicname(npUp->logic5));
					}
					inputChangedList.push_back(npUp->ref);
				}
			}
		}else{
			//  Output is 1 and there are multiple unknowns
			//  Choice must be make; add to J frontier
			isJ = true;
		}
		return;
	}
	
	//  XOR/XNOR gate handling
	if((np->gateType==XOR)||(np->gateType==XNOR)){
		//  Assume only 2 inputs to XOR gate
		if(nX==1){
			//  1 unknown, inputs are opposite
			isJ = false;
			NSTRUC *np1, *np2;
			np1 = getNodePtr(np->upNodes[0]);
			np2 = getNodePtr(np->upNodes[1]);
			if(np1->logic5==X){
				np1->logic5 = NOT_LOGIC5[np2->logic5];
				if(debugMode_Dalg>1){
					printf("Applying logic, Node: %d = %s\n", np1->ref, logicname(np1->logic5));
				}
				inputChangedList.push_back(np1->ref);
			}else{
				np2->logic5 = NOT_LOGIC5[np1->logic5];
				if(debugMode_Dalg>1){
					printf("Applying logic, Node: %d = %s\n", np2->ref, logicname(np2->logic5));
				}
				inputChangedList.push_back(np2->ref);
			}
		}else{
			//  2 unknowns to XOR gate; choice must be made
			isJ = true;
		}
	}
	
}
		

enum e_logicType checkLogic_Dalg(int nodeRef, bool &isOK, bool &isJ, bool &isD){
	//  Simulate 5-value logic based on Logic Tables
	//  Does not actually apply any logic to this node; only returns simulated output
	//  if there is a conflict, if it a D-frontier, or if it is a J-frontier
	NSTRUC *np, *npUp;
	enum e_logicType logSim, in1, in2, logAssigned, logActual;
	enum e_gateType gateType;
	
	// Current gate being evaluated
	np = getNodePtr(nodeRef);
	gateType = np->gateType;
	
	//  Check if this is the faulty node
	//if(nodeRef==faultyNode_Dalg){
	//	isOK = true;
	//	isJ = false;
	//	isD = false;
	//	return np->logic5;
	//}
	
	//  Get all the input logics
	vector<enum e_logicType>inArr;
	for(int i=0;i<np->upNodes.size();i++){
		npUp = getNodePtr(np->upNodes[i]);
		inArr.push_back(npUp->logic5);
	}
	
	if(inArr.size()==0){
		//  This must be a PI
		isOK = true;
		isJ = false;
		isD = false;
		return np->logic5;
	}else if(inArr.size()==1){
		logSim = inArr[0];
	}else{
		in1 = inArr[0];
		for(int i=1;i<inArr.size();i++){
			//  For more than 3 inputs, simulate as cascading gates
			in2 = inArr[i];
			switch(gateType){
				case AND:
				case NAND:
					logSim = AND_LOGIC5[in1][in2];
					//printf("input1: %s, input2: %s, ouput: %s\n", logicname(in1), logicname(in2), logicname(logSim));
					break;
				case OR:
				case NOR:
					logSim = OR_LOGIC5[in1][in2];
					break;
				case XOR:
				case XNOR:
					logSim = XOR_LOGIC5[in1][in2];
					break;
				default:
					logSim = in1;
			}
			in1 = logSim; //  Input to next gate
		}
	}
	//  Inversion if necessary
	if((gateType==NAND)||(gateType==NOR)||(gateType==NOT)||(gateType==XNOR)){
		logSim = NOT_LOGIC5[logSim];
	}
	
	//  Find out if there is a conflict / inconsistency
	logActual = np->logic5;
	
	//  Special handling for faulty node
	if(np->ref == faultyNode_Dalg){
		isJ = false;
		isD = false;
		
		if(logSim == X){
			isOK = true;
			return logSim;
		}
		if((logSim==1)&&(logActual==D)){
			//  Need to assign 1 at this node for D 
			isOK = true;
			return D;
		}else if((logSim==0) && (logActual == Dbar)){
			//  Need to assign a 0 at this node for Dbar
			isOK = true;
			return logSim;
		}else{
			isOK = false;
			return logSim;
		}
		
		
	}
	
	
	if((logSim==logActual) || (logSim==X)||(logActual==X)){
		//  No Problem
		isOK = true;
	}else{		
		//  Conflict
		printf("  input logic: %s, gate logic: %s\n", logicname(logSim), logicname(logActual));
		
		isOK = false;
		isJ = false;
		isD = false;
		return logSim;
	}
	
	//  Find out if this is a J-frontier
	//  1 or 0 on output, not fully defined by inputs
	if((logActual==one)||(logActual==zero)){
		if(logSim==X){
			isJ = true;
			isD = false;
			return logSim;
		}else{
			isJ = false;
		}
	}else{
		isJ = false;
	}
	
	//  Find out if this is a D-Frontier
	//  One or more inputs are D and output is X
	if(logSim==X){
		isD = false;
		for(int i=0;i<inArr.size();i++){
			if((inArr[i]==D)||(inArr[i]==Dbar)){
				isD = true;
				break;
			}
		}
	}else{
		isD = false;
	}
	
	
	return logSim;
	
}

void branchPropogate_Dalg(int ref_B,enum e_logicType setLogic,  int origin){
	//  Propogate a logic through a fan-out branch system.
	NSTRUC *np, *npUp, *npDown;
	
	np = getNodePtr(ref_B);
	
	//  Assign logic
	//  Special condition for the faulty node
	if(np->ref == faultyNode_Dalg){
		if(np->logic5 == D){
			setLogic = one;
		}else{
			setLogic = zero;
		}
	}else{
		np->logic5 = setLogic;
		if(debugMode_Dalg>1){
			printf("  Node %d logic set to %s\n", np->ref, logicname(np->logic5));
		}
	}
	
	
	//  Propogate downwards
	//  Must use setLogic in case this is a faulty node
	if(np->gateType!=BRCH){
		//  Reached a stump
		//  Assign logic and assign to back-propogate
		if(debugMode_Dalg>1){
			printf("Adding node %d to backward queue\n", np->ref);
		}
		addNodeToQueue(nodeQueueBackward, np->ref);
	}else{
		//  Only 1 input to a branch
		npUp = getNodePtr(np->upNodes[0]);
		if(npUp->ref != origin){
			branchPropogate_Dalg(npUp->ref, setLogic, np->ref);
		}
	}
	
	//  Propogate Upwards
	//  Must use node logic in case it is a faulty node
	for(int i = 0;i<np->downNodes.size();++i){
		npDown = getNodePtr(np->downNodes[i]);
		if(npDown->ref == origin){
			continue;
		}
		if(npDown->gateType == BRCH){
			branchPropogate_Dalg(npDown->ref, np->logic5, np->ref);
		}else{
			//  Reached a gate or PO
			if(debugMode_Dalg>1){
				printf("Adding node %d to forward queue\n", npDown->ref);
			}
			addNodeToQueue(nodeQueueForward, npDown->ref);
		}
	}
}

void saveState_Dalg(set<int>& J_front_save, set<int>& D_front_save, vector< pair<int,enum e_logicType> >& logicState){
	//  Save the current state of the circuit analysis
	//  For use in case back-tracking is needed
	//  No need to save the node queues; should already be empty
	set<int>::iterator setIter;
	
	//   Save the J frontier and D frontier
	J_front_save.clear();
	D_front_save.clear();
	for(setIter = J_frontier.begin();setIter!=J_frontier.end();++setIter){
		J_front_save.insert(*setIter);
	}
	for(setIter = D_frontier.begin();setIter!=D_frontier.end();++setIter){
		D_front_save.insert(*setIter);
	}
	
	//  Save the logic states
	logicState.clear();
	NSTRUC *np;
	for(int i=0;i<NodeV.size();++i){
		np = &NodeV[i];
		logicState.push_back(make_pair(np->ref, np->logic5));
	}
}

void reloadState_Dalg(set<int>& J_front_save, set<int>& D_front_save, vector< pair<int,enum e_logicType> >& logicState){
	//  Reload the current state of the circuit analysis
	//  For use in case back-tracking is needed
	//  No need to reload the node queues; should already be empty
	
	set<int>::iterator setIter;
	J_frontier.clear();
	D_frontier.clear();
	for(setIter = J_front_save.begin();setIter!=J_front_save.end();++setIter){
		J_frontier.insert(*setIter);
	}
	for(setIter = D_front_save.begin();setIter!=D_front_save.end();++setIter){
		D_frontier.insert(*setIter);
	}
	
	//  Reload the logic states
	NSTRUC *np;
	for(int i=0;i<logicState.size();++i){
		np = getNodePtr(logicState[i].first);
		np->logic5 = logicState[i].second;
	}
	//  Ensure node processing queues are empty
	nodeQueueForward.clear();
	nodeQueueBackward.clear();
}




bool isErrAtPO_Dalg(void){
	//  Determine if error is at PO
	NSTRUC *np;
	
	for(int i=0;i<PO_Nodes.size();++i){
		np = getNodePtr(PO_Nodes[i]);
		if((np->logic5 == D)||(np->logic5 == Dbar)){
			return true;
		}
	}
	return false;
}

int nInputsX_Dalg(int nodeRef){
	//  Return the # of "X" inputs to the given node
	
	NSTRUC *np, *npUp;
	np = getNodePtr(nodeRef);
	
	int nX = 0;
	for(int i=0;i<np->upNodes.size();i++){
		npUp = getNodePtr(np->upNodes[i]);
		if(npUp->logic5==X){
			++nX;
		}
	}
	return nX;
}

void resetNodes_Dalg(void){
	//  Reset all nodes to X
	NSTRUC *np;
	for(int i = 0;i<NodeV.size();i++){
		np = &NodeV[i];
		np->logic5 = X;
	}
}

void printNode_Dalg(int nodeRef){
	NSTRUC *np, *npUp;
	np = getNodePtr(nodeRef);
	printf("Node %d:\n",np->ref);
	printf("  Gate Type: %s\n",gname(np->gateType));
	printf("  Node Type: %s\n",nname(np->nodeType));
	printf("  Logic:     %s\n",logicname(np->logic5));
	printf("  Inputs:\n    ");
	for(int i=0;i<np->upNodes.size();++i){
		printf("%d,\t", np->upNodes[i]);
	}
	printf("\n    ");
	for(int i=0;i<np->upNodes.size();++i){
		npUp = getNodePtr(np->upNodes[i]);
		printf("%s,\t", logicname(npUp->logic5));
	}
	printf("\n");
}

void printFrontiers_Dalg(void){
	set<int>::iterator setIt;
	printf("D Frontier:\n  ");
	if(D_frontier.size()==0){
		printf("empty[]");
	}else{
		for(setIt = D_frontier.begin();setIt!=D_frontier.end();++setIt){
		printf("%d, ", *setIt);
	}
	}
	
	printf("\nJ Frontier:\n  ");
	if(J_frontier.size()==0){
		printf("empty[]");
	}else{
		for(setIt = J_frontier.begin();setIt!=J_frontier.end();++setIt){
			printf("%d, ", *setIt);
		}
	}
	printf("\n");
	
}

void DALG_DEBUG(char *cp){
	printf("Current debug mode: %d\n",debugMode_Dalg);
	sscanf(cp, "%d", &debugMode_Dalg);
	printf("Debug mode set to: %d\n",debugMode_Dalg);
	
}

/*----------------------------------------------------*/
/*------Parallel Fault Simulation -------------------*/
/*----------------------------------------------------*/
void pfs(char *cp)
{
	//  Parallel fault simulation wrapper
	char patternFile[MAXLINE];
	char faultFile[MAXLINE];
	char outFile[MAXLINE];
	bool fileOK;
	
	 //  Ensure all nodes are simulated
	 //  Necessary for paralell fault simulation
	eventDriven = false;
	
	//  Read in file names
	sscanf(cp, "%s %s %s", patternFile, faultFile, outFile);
	//Debug //////////
	printf("\nParallel Fault Simulation\n");
	printf("Test Patterns: %s\n",patternFile);
	printf("Fault List: %s\n", faultFile);
	printf("Output: %s;\n", outFile);
	////////////////////////////////////
	
	//  Read Fault List
	//    Populates "FaultV"
	fileOK = readFaultList(faultFile);
	if(!fileOK){return;}
	//// --- Debug -------------
	//printFaultList(); //  Debug
	// --------------------------
	
	//  Read Test Patterns; 1 or multiple
	//  Test patterns saved to:
	//     	vector<int> "PI_list";
	//		vector<vector<char> > "inputPatterns";
	fileOK = readTestPatterns(patternFile);
	if(!fileOK){return;}
	
	//// --- Debug -------------
	//printInputPatterns();
	// --------------------------
	
	//  Perform fault simulation
	parallelFaultSimulation();
	
	//// --- Debug -------------
	//printFaultList(); //  Debug
	// --------------------------
	
	//  Done; write report
	writeFaultsDetected(outFile);
	
	//  Print "OK"
	printf("\n==> OK\n");
	
}

void parallelFaultSimulation(void){
	//  Perform fault simulation
	//  Assumes the following are already populated correctly:
	//		"PI_list"
	//		"inputPatterns"
	//		"FaultV"
	
	// Run logic simulation processing each node
	eventDriven = false;
	
	//  Determine the # of passes to simulate all faults
	//  Can only do (32 or 64 -1) faults
	//  first entry is always "no fault"
	int N_passes, N_faults, faults;
	int indStart, indEnd;
	N_faults = FaultV.size();
	N_passes = intCeil(N_faults, bitWidth-1);// Ceil function, A/B
	
	// Debug Printout //////////////////
	//printf("\n %d input patterns, %d Faults\n", inputPatterns.size(),N_faults);
	/////////////////////////////
	for(int patt = 0;patt<inputPatterns.size();++patt){
		
		for(int pass = 0;pass<N_passes;++pass){
			//  Find start and stop indexes in fault list for this pass
			indStart = pass*(bitWidth-1);
			indEnd = (pass+1)*(bitWidth-1)-1;
			indEnd = min(indEnd, (N_faults-1));
			//Debug ////////////////////
			//printf("Pass %d; Fault list start: %d, end: %d, total: %d\n",pass, indStart, indEnd, FaultV.size());
			///////////////////////////////
			
			//  Set input pattern at PI's
			setPI_forPFS(patt);
			// Set all fault masks to known value
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
	
	
}
void randomTestGenerator(char *cp)
{
	//  Performs random test pattern simulation
	char patternFile[MAXLINE];
	char fcFile[MAXLINE];
	int Ntot = 0;
	int Ntfcr = 0;
	
	//  Read in file names
	sscanf(cp, "%d %d %s %s", &Ntot, &Ntfcr, patternFile, fcFile);
	
	//Debug //////////
	printf("\nRandom Test Generation and FC Calculation\n");
	printf("Total Patterns: %d\nReportFrequency: %d\n", Ntot, Ntfcr);
	printf("Pattern File: %s\n",patternFile);
	printf("FC Report: %s\n", fcFile);
	////////////////////////////////////
	
	if((Ntot==0)||(Ntfcr==0)){
		printf("\nNumber of patterns 0; cannot run.\n");
		return;
	}
	
	//  Generate Fault Vector
	//  Generates "FaultV"
	//  Should eventually be output from reduced fault list
	genAllFaults();
	
	//  Determine how many simulations will be run
	int N_passes;
	vector<float> faultCoverage;
	//  Get ceil of N_patterns/bitWidth;
	N_passes = intCeil(Ntot, Ntfcr);
	for(int pass = 0;pass<N_passes;++pass){
		//  Generate random faults
		//  Populates "PI_list" and "inputPatterns"
		genRandomInputs(Ntfcr);
		
		//  Perform fault simulation
		parallelFaultSimulation();
		
		//  Get current fault coverage
		float FC = getFaultCoverage();
		printf("%d of %d Patterns.  Fault Coverage: %0.2f\n",
			(pass+1)*Ntfcr, Ntot, 100*FC);
		faultCoverage.push_back(FC);
		
		//  Drop Faults
		dropFaults();
		
		//  Record test patterns
		//  Second input indicates header row to be added
		writeInputPatterns(patternFile, (pass==0));
	}
	
	// Debug
	//printFaultList();
	
	//  Write fault coverage report
	writeFaultCoverageReport(faultCoverage, fcFile);
	
	//  Print "OK"
	printf("\n==> OK\n");

}

void dropFaults(void){
	//  Drop faults to speed up ATPG
	FSTRUC *fp;
	vector<FSTRUC>::iterator fIter;
	
	
	for(int k=0;k<FaultV.size();++k){
		fp = &FaultV[k];
		if(fp->faultFound.size()>=N_DROP){
			//printf("\nDropping fault %d\n",fp->ref);
			FaultV_Dropped.push_back(FaultV[k]);
			FaultV.erase(FaultV.begin()+k);
			--k;
		}
	}
	
}

float getFaultCoverage(void){
	//  Get the current fault coverage
	//  Based on the current vector FaultV
	
	int N_Total = 0;
	int N_Detected = 0;
	FSTRUC *fp;
	for(int k=0;k<FaultV.size();++k){
		fp = &FaultV[k];
		if(fp->faultFound.size()>0){
			++N_Detected;
		}
	}

	N_Total = FaultV.size()+FaultV_Dropped.size();
	N_Detected = N_Detected+FaultV_Dropped.size();
	
	float FC = N_Detected;
	FC = FC/N_Total;
	return FC;
	
}

void genAllFaults(void){
	//  Generates all possible faults and load to "FaultV"
	//  Placeholder until results of RFL can be used
	FSTRUC tempFault;
	NSTRUC *np;
	
	//  First clear out fault vector.
	FaultV.clear();
	FaultV_Dropped.clear();
	
	tempFault.faultFound.clear();
	for(int i=0;i<NodeV.size();++i){
		np = &NodeV[i];
		tempFault.ref = np->ref;
		tempFault.stuckAt = false;
		FaultV.push_back(tempFault);
		tempFault.stuckAt = true;
		FaultV.push_back(tempFault);			
	}	
}

void setFaults(int indStart, int indEnd){
	//  Cycles through the "FaultV" vector
	//  Each fault is set at that node with 
	//  the fault "fmask_OR" and "fmask_AND".
	int f;
	int ref;
	NSTRUC *np;
	FSTRUC *fp;
	
	resetFaultMasks();
	
	
	int bitCounter = 1;//  Start at second bit
	for(f = indStart;f<=indEnd;++f){
		fp = &FaultV[f];
		np = getNodePtr(fp->ref);
		//np = &NodeV[ref2index[fp->ref]];
		if(fp->stuckAt){
			//  Stuck at 1
			np->fmask_OR = (np->fmask_OR)|(1U<<bitCounter);
		}else{
			//  Stuck at 0
			np->fmask_AND = (np->fmask_AND)&(~(1U<<bitCounter));
		}
		bitCounter++;
	}
}

//  
void checkFaults(int indStart,int indEnd, int patt){
	//  This function checks to see if faults have been
	//  detected.  Cycles through each fault as indicated
	//  by the "indStart" and "indEnd" (referenced to "FaultV").
	//  Each PO node output is checked against bit 0 to see if the
	//  output has changed with that fault.  If it has, the 
	//  current pattern reference that found that fault is added.
	int f;
	unsigned int temp;
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
			np = getNodePtr(PO_Nodes[nPo]);
			//np = &NodeV[ref2index[PO_Nodes[nPo]]];
			//  Check if logic at bitCounter 
			//  is different than bit 0
			for(int k=0;k<2;k++){
				temp = np->logic3[k]& (1U<<bitCounter);
				temp = temp>>bitCounter;
				detected = detected|((np->logic3[k]&1U)!= temp);
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

void genRandomInputs(int N_patterns){
	//  Generate vector of random inputs.
	//  Random seed already initialized in "main".
	//  Populates:
	//		"PI_list"
	//		"inputPatterns"
	
	//  Add all PI's to the PI list
	PI_list.clear();
	for(int k =0;k<PI_Nodes.size();k++){
		PI_list.push_back(PI_Nodes[k]);
	}
	
	//  Clear test patterns
	inputPatterns.clear();
	
	int tempRand;
	vector<char> tempPattern;
	for(int patt = 0;patt<N_patterns;patt++){
		tempPattern.clear();
		for(int k=0;k<PI_Nodes.size();k++){
			tempRand = rand() ;
			if(tempRand&1){
				tempPattern.push_back('1');
			}else{
				tempPattern.push_back('0');
			}
		}//  Loop for each PI
		inputPatterns.push_back(tempPattern);
	}//  Loop for each pattern
	
	//printInputPatterns();//Debug
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
	printf("\nParallel Logic Simulation\n");
	printf("Input Logic File: %s\n", patternFile);
	printf("Output Logic File: %s\n", writeFile);
	////////////////
	
	//  Read Test Patterns; 1 or multiple
	//  Test patterns saved to:
	//     	vector<int> PI_list;
	//		vector<vector<char> > inputPatterns;
	bool fileOK = readTestPatterns(patternFile);
	if(!fileOK){return;}
	
	//// --- Debug -------------
	printInputPatterns();
	// --------------------------
	
	// Set all fault masks to known value
	//  No faults are being simulated here
	resetFaultMasks();
	
	//  Clear output
	outputPatterns.clear();
	
	//  Perform Simulation
	parallelLogicSimulation();
	
	//  Write Results
	writeOutputPatterns(writeFile);
		
	//  Print "OK"
	printf("\n==> OK\n");
}

void multi_dfs(char *cp){
	dfs_fault_list.clear();
	fault_vals.clear();
	inputPatterns.clear();
	//int_inputPatterns.clear();
	vector<int>::iterator it;
	vector<int> u, v;
	std::map<int, vector<int> >::iterator itr;
	vector<string>::iterator s;
	vector<NSTRUC>::iterator np, ns = NodeV.begin();
	char readFile[MAXLINE];
	char writeFile[MAXLINE];
	int i, j = 1, max = 0;
	vector<char>::iterator ch;
	fstream file;;
	NSTRUC *inNode;
	bool run_again = false;
	dfs_count = 1;
	final.clear();
	
	
	sscanf(cp, "%s %s", readFile, writeFile);
	//   Debug //////////////
	printf("\nDFS Simulation\n");
	printf("Input Patterns: %s\n",readFile);
	printf("Output Results: %s\n",writeFile);
	///////////////////
	
	readTestPatterns(readFile);
	
	for(np = NodeV.begin(); np!= NodeV.end(); np++){
			if(np->ref > max){
				max = np->ref;
		}
	}
	
	for(i = 0; i < inputPatterns.size(); i++)
	{
		dfs_logicSim( i);
		if(dfs_count == 1){
			for(np = NodeV.begin(); np != NodeV.end(); np++){
				prevLogic.push_back(np->logic);
				single_dfs(np->ref);
				
			}
		}
		else{
			int x = 0;
			for(np = NodeV.begin(); np != NodeV.end(); np++){
				if(np->logic != prevLogic[x]){
					dfs_fault_list.erase(np->ref);
					for(int k = 0; k < np->fin; k++){
						dfs_fault_list.erase(np->upNodes[k]);
						single_dfs(np->upNodes[k]);
					}
					
					single_dfs(np->ref);
				}
				x++;
			}
			
		}
		dfs_count++;
		for(int k = 0;k<PO_Nodes.size();++k)
		//for(np = PO_Nodes.begin(); np!= PO_Nodes.end(); np++)
		{
			NSTRUC *ndPtr = getNodePtr(PO_Nodes[k]);
			//np = &NodeV[ref2index[PO_Nodes[k]]];
			for(it = dfs_fault_list[ndPtr->ref].begin(); it != dfs_fault_list[ndPtr->ref].end(); it++){
				if(*it > 0 && *it <= max){
					if(std::find(final.begin(), final.end(), *it) == final.end())
					{
						final.push_back(*it);			
					}
				}
				
			}
		}
	}
	file.open(writeFile, ios_base::out);
    if(!file.is_open())
    {
        cout<<"Unable to open the file.\n";
        return;
	}
	for(it = final.begin(); it != final.end(); it++)
	{
		file<<*it<<"@"<<fault_vals[*it]<<"\n";
	}
	
	//  Print "OK"
	printf("\n==> OK\n");
}

void helper_dfs(int c, int i, int val){
	NSTRUC *inNode;
	vector<NSTRUC>::iterator nstr;
	int flag = 0, j, ind;
	vector<int> output1, output2, output3, output, c_list, n_list;
	vector<int>::iterator it, out;
	map<int, vector<int> >::iterator itr;
	output.clear();
	output1.clear();
	output2.clear();
	c_list.clear();
	n_list.clear();
	//NSTRUC *np = &NodeV[ref2index[val]];
	NSTRUC *np = getNodePtr(val);
	for(j = 0; j < np->fin; j++){
		inNode = getNodePtr(np->upNodes[j]);
		//inNode = &NodeV[ref2index[np->upNodes[j]]];
		if(inNode->logic == c){
			c_list.push_back(inNode->ref);
			if(c_list.size() == 1){
				for(it = dfs_fault_list[inNode->ref].begin(); it != dfs_fault_list[inNode->ref].end(); it++){
					output1.push_back(*it);
				}
			}
		} 
		else{
			n_list.push_back(inNode->ref);
			if(n_list.size() == 1){
				for(it = dfs_fault_list[inNode->ref].begin(); it != dfs_fault_list[inNode->ref].end(); it++){
					output2.push_back(*it);
				}
			}
		}
	}
	
	for(j = 0; j < c_list.size(); j++){
		std::set_intersection(output1.begin(),output1.end(), dfs_fault_list[c_list[j]].begin(),dfs_fault_list[c_list[j]].end(), std::inserter(output3 ,output3.begin()));
		output1.clear();
		copy(output3.begin(), output3.end(), std::inserter(output1, output1.begin()));
		output3.clear();
	}
	for(j = 0; j < n_list.size(); j++){
		std::set_union(output2.begin(),output2.end(), dfs_fault_list[n_list[j]].begin(),dfs_fault_list[n_list[j]].end(), std::inserter(output2 ,output2.begin()));
	}
	if(c_list.size() > 0){
		std::set_difference(output1.begin(),output1.end(), output2.begin(), output2.end(), std::inserter(output, output.begin()));
		fault_vals.insert(pair<int, int>(np->ref, (c ^ (1-i))));
	}
	else{
		copy(output2.begin(), output2.end(), std::inserter(output, output.begin()));
		fault_vals.insert(pair<int, int>(np->ref, (c ^ i)));
	}
	output.push_back(np->ref);
	dfs_fault_list.insert(pair<int, vector<int> >(np->ref, output));
}

void single_dfs(int val)
{
	int c, i, j, ind;
	std::map<int, vector<int> >::iterator itr;
	FILE *fptr;
	vector<int> output, output1, output2;
	vector<int>::iterator it;
	//vector<int>::iterator nstr_it;
	char readFile[MAXLINE];

	NSTRUC *np, *inNode, *ptr1, *ptr2;
	np = getNodePtr(val);
  	//np = &NodeV[ref2index[val]];
	for(i = 0; i < np->fin; i++){
		if(dfs_count == 1){
			if(dfs_fault_list.find(np->upNodes[i]) == dfs_fault_list.end()){
				reVisit.push_back(np->upNodes[i]);
				single_dfs(np->upNodes[i]);
			}
		}	
	}


	switch (np->gateType){
		case IPT:
			output.clear();
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		case BRCH:
			output.clear();
			for(j = 0; j < np->fin; j++){
				inNode = getNodePtr(np->upNodes[j]);
				//inNode = &NodeV[ref2index[np->upNodes[j]]];
				for(ind = 0; ind < dfs_fault_list[inNode->ref].size(); ind++)
					output.push_back(dfs_fault_list[inNode->ref][ind]);
			}
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		case OR:
			c = 1;
			i = 0;
			helper_dfs(c, i, val);
			break;
		case NOR:
			c = 1;
			i = 1;
			helper_dfs(c, i, val);
			break;
		case AND:
			c = 0;
			i = 0;
			helper_dfs(c, i, val);
			break;
		case NAND:
			c = 0;
			i = 1;
			helper_dfs(c, i, val);
			break;
		case NOT:
			output.clear();
			for(j = 0; j < np->fin; j++){
				inNode = getNodePtr(np->upNodes[j]);
				//inNode = &NodeV[ref2index[np->upNodes[j]]];
				for(ind = 0; ind < dfs_fault_list[inNode->ref].size(); ind++)
					output.push_back(dfs_fault_list[inNode->ref][ind]);
			}
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		case XOR:
			output.clear();
			ptr1 = getNodePtr(np->upNodes[0]);
			ptr2 = getNodePtr(np->upNodes[1]);
			
			//ptr1 = &NodeV[ref2index[np->upNodes[0]]];
			//ptr2 = &NodeV[ref2index[np->upNodes[1]]];
			
			//std::set_union(dfs_fault_list[ptr1->ref].begin(),dfs_fault_list[ptr1->ref].end(), dfs_fault_list[ptr2->ref].begin(),dfs_fault_list[ptr2->ref].end(), std::inserter(output1, output1.begin()));
			//std::set_intersection(dfs_fault_list[ptr1->ref].begin(),dfs_fault_list[ptr1->ref].end(), dfs_fault_list[ptr2->ref].begin(),dfs_fault_list[ptr2->ref].end(), std::inserter(output2, output2.begin()));
			//std::set_difference(output1.begin(),output1.end(), output2.begin(), output2.end(), std::inserter(output, output.begin()));
			output1.clear();
			output2.clear();
			//ptr1 = &NodeV[ref2index[np->upNodes[0]]];
			//ptr2 = &NodeV[ref2index[np->upNodes[1]]];
			if(ptr1->logic == ptr2->logic){
				std::set_union(dfs_fault_list[ptr1->ref].begin(),dfs_fault_list[ptr1->ref].end(), dfs_fault_list[ptr2->ref].begin(),dfs_fault_list[ptr2->ref].end(), std::inserter(output1, output1.begin()));
				std::set_intersection(dfs_fault_list[ptr1->ref].begin(),dfs_fault_list[ptr1->ref].end(), dfs_fault_list[ptr2->ref].begin(),dfs_fault_list[ptr2->ref].end(), std::inserter(output2, output2.begin()));
				std::set_difference(output1.begin(),output1.end(), output2.begin(), output2.end(), std::inserter(output, output.begin()));
			}
			else{
				std::set_union(dfs_fault_list[ptr1->ref].begin(),dfs_fault_list[ptr1->ref].end(), dfs_fault_list[ptr2->ref].begin(),dfs_fault_list[ptr2->ref].end(), std::inserter(output, output.begin()));
			}
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		default:
			printf("Node type %d not recognized\n",np->gateType);
	}
}

void parallelLogicSimulation(void){
	//  Parallel Logic Simulation
		
	//  Determine the # of passes to simulate input
	//  Can only do 32/64 patterns per pass (Depending on word width)
	int N_passes, N_patterns, n_patterns;
	N_patterns = inputPatterns.size();
	//  Get ceil of N_patterns/bitWidth;
	N_passes = intCeil(N_patterns, bitWidth);
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
}


char getLogic(int nodeRef,int index){
	//  Returns an ascii character based on the logic at the specified index
	//  Logic is 3x32 or 3x64, each column is a result from a test pattern
	NSTRUC *np;
	bool log[3];
	np = getNodePtr(nodeRef);
	//np = &NodeV[ref2index[nodeRef]];
	unsigned int mask = 1U<<index;
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
		np = getNodePtr(PI_list[PI]);
		//np = &NodeV[ref2index[PI_list[PI]]];

		//Get logic value of this PI for this test pattern
		char logic = inputPatterns[patt][PI];
		switch (logic){
			case '0':
				//0;0;0
				np->logic3[0] = 0u;
				np->logic3[1] = 0u;
				np->logic3[2] = 0u;
				np->logic = false;
				break;
			case '1':
				// 1;1;0
				np->logic3[0] = ~0u;
				np->logic3[1] = ~0u;
				np->logic3[2] = 0u;
				np->logic = true;
				break;
			default:
				// 0;1;0
				np->logic3[0] = 0u;
				np->logic3[1] = ~0u;
				np->logic3[2] = 0u;
				np->logic = false;
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
		np = getNodePtr(PI_list[PI]);
		//  Set to initial state of 'X'
		np->logic3[0] = 0;
		np->logic3[1] = ~0;// set to all 1's
		np->logic3[2] = 0;
		for(int patt =indStart;patt<=indEnd;patt++){
			//Cycle through each test pattern
			//Get logic value of this PI for this test pattern
			char logic = inputPatterns[patt][PI];
			//Left<<shift adds 0 to the LSB
			np->logic3[0] = np->logic3[0]<<1U;
			np->logic3[1] = np->logic3[1]<<1U;
			np->logic3[2] = np->logic3[2]<<1U;
			np->logic = false;
			switch (logic){
				case '0':
					//  Nothing to do here; already set to 
					// 0;0;0
					break;
				case '1':
					// 1;1;0
					np->logic3[0] += 1u;
					np->logic3[1] += 1u;
					np->logic = true;
					break;
				default:
					// 0;1;0
					np->logic3[1] += 1u;
			}
		}//  End loop for each test pattern
	}// End loop for each PI
}




void addNodeToQueue(vector< pair<int,int> >& queue, int nodeRef){

	//  First determine if this node is already in the queue
	vector< pair<int,int> > ::iterator qIter;
	for(qIter = queue.begin(); qIter!=queue.end();qIter++){
		if(qIter->second==nodeRef){
			//  Already in queue, exit
		return;
		}
	}

	//  Get pointer to this node
	NSTRUC *np;
	//np = &NodeV[ref2index[nodeRef]];
	np = getNodePtr(nodeRef);
	//  Add it to the queue
	//  First item is level, second is reference
	queue.push_back(make_pair(np->level, np->ref));
	//  Sort by the level, largest to smallest
	sort(queue.rbegin(), queue.rend());
}

void addPiNodesToQueue(void){
	nodeQueue.clear();
	for(int j=0;j<PI_list.size();j++){
		addNodeToQueue(nodeQueue, PI_list[j]);
	}
}

//void dfs_logicSim(char *cp, int i)
void dfs_logicSim(int patt)
{
	//std::vector<NSTRUC>::iterator printnode, outitr;
	//vector<vector<char> >::iterator ch;
	//FILE *fptr;
	int j;
	//char writeFile[MAXLINE];
	NSTRUC *np;
	
	//  Set input pattern at PI's
	setPI_forPFS(patt);
	
	// Set all fault masks to known value
	resetFaultMasks();
			
	//  Add PI list to queue
	addPiNodesToQueue();
	
	//levelizeNodes();//  Only done once when circuit read-in
	processNodeQueue();
	//printf("==> OK\n");
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
		np = getNodePtr(nodeQueue.back().second);
		nodeQueue.pop_back();//  Remove this node
		//  Perform logic simulation
		//printf("Processing Node %d\n",np->ref);
		logicChanged = simNode3(np->ref);
		//  Now add all downstream nodes to the queue if the logic changed
		//  If not event-driven, add all nodes regardless
		if ((logicChanged)||(eventDriven == false)){
			for(int k = 0;k<np->fout;k++){
				addNodeToQueue(nodeQueue, np->downNodes[k]);
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
	unsigned int oldLogic[3], temp0, temp1;
	
	//  Get pointer to current node
	np = getNodePtr(nodeRef);
	//np = &NodeV[ref2index[nodeRef]];
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
	npIn_A = getNodePtr(np->upNodes[0]);
	//npIn_A = &NodeV[ref2index[np->upNodes[0]]];
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
			npIn_B = getNodePtr(np->upNodes[1]);
			//npIn_B = &NodeV[ref2index[np->upNodes[1]]];
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
					npIn_B = getNodePtr(np->upNodes[inp]);
					//npIn_B = &NodeV[ref2index[np->upNodes[inp]]];
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
				npIn_B = getNodePtr(np->upNodes[inp]);
				//npIn_B = &NodeV[ref2index[np->upNodes[inp]]];
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
	
	//  Set single logic; needed for DFS
	np->logic = np->logic3[0] & 1U;
	
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

void rfl(char *cp)
{
	//std::vector<NSTRUC>::iterator np;
	//int checkpoints = Npi;
	//int j;
	FILE *fptr;
	char writeFile[MAXLINE];
	sscanf(cp, "%s", writeFile);
	
	//Debug//////////////
	printf("\nReduced Fault List\n");
	printf("Output File: %s\n",writeFile);
	///////////////////////
	
	// Run reduced fault list routine
	//  Populates FaultV
	reduced_fault_list();
	
	
	fptr = fopen(writeFile,"w");
	if(fptr == NULL) {
		printf("File %s cannot be written!\n", writeFile);
		return;
	}
	
	for(int i = 0;i<FaultV.size();++i){
		fprintf(fptr, "%d@%d\n", FaultV[i].ref, FaultV[i].stuckAt);
	}
   
   /*
	for(np = NodeV.begin(); np!= NodeV.end(); np++){
		if(np->gateType == IPT){
			fprintf(fptr, "%d@0\n", np->indx+1);
			fprintf(fptr, "%d@1\n", np->indx+1);
		}
		for(j = 0; j<np->fout; j++){
			if(np->fout > 1)
			{
				fprintf(fptr,"%d@0\n",np->downNodes[j]);
				fprintf(fptr,"%d@1\n",np->downNodes[j]);
			}
		}
	}
	*/
	fclose(fptr);
	
	//  Print "OK"
	printf("\n==> OK\n");
}

void reduced_fault_list(void){
	//  Generate the reduced fault list and populate the FaultV vector
	std::vector<NSTRUC>::iterator np;
	FSTRUC fault;
	fault.faultFound.clear();
	
	FaultV.clear();
	FaultV_Dropped.clear();
	
	for(np = NodeV.begin(); np!= NodeV.end(); np++){
		if(np->gateType == IPT){
			fault.ref = np->ref;
			fault.stuckAt = false;
			FaultV.push_back(fault);
			fault.stuckAt = true;
			FaultV.push_back(fault);
			//fprintf(fptr, "%d@0\n", np->indx+1);
			//fprintf(fptr, "%d@1\n", np->indx+1);
		}
		for(int j = 0; j<np->fout; j++){
			if(np->fout > 1)
			{
				fault.ref = np->downNodes[j];
				fault.stuckAt = false;
				FaultV.push_back(fault);
				fault.stuckAt = true;
				FaultV.push_back(fault);
				//fprintf(fptr,"%d@0\n",np->downNodes[j]);
				//fprintf(fptr,"%d@1\n",np->downNodes[j]);
			}
		}
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
	// Debug/////////////
	printf("Read Circuit\n");
	printf("File name: %s\n",buf);
	//////////////////
	
	fd = fopen(buf,"r");
	if(fd == NULL) {
		printf("File %s does not exist!\n", buf);
		return;
	}

	//  Parse and store the name of this circuit into
	//  the "currentCircuit" global variable.
	getCircuitNameFromFile(buf);
	
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
		tempNode.logic3[0] = 0u;
		tempNode.logic3[1] = ~0u;//  Set to all 1's
		tempNode.logic3[2] = 0u;
		tempNode.logic = false;
		tempNode.logic5 = X;
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
	
	printf("Parsed circuit %s\n", currentCircuit);
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
	nodePtr = getNodePtr(nodeRef);
	//nodePtr = &NodeV[ref2index[nodeRef]];

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
void getCircuitNameFromFile(char *fileName){
	//  Code to get name of circuit from file-name
	//  Example:  Converts "./circuits/c17.ckt" to "c17"
	
	//  Copy file name to currentCircuit string
	strcpy(currentCircuit, fileName);
	
	//  Remove everything before the '/' character
	char *strPtr;
    while(1)
    {
        strPtr= strchr(currentCircuit,'/');
        if((strPtr!=NULL) && (strPtr<(currentCircuit+MAXNAME-2))){
			//  Trim to everything after the '/' character
            strcpy(currentCircuit, strPtr+1);
        }else{
			//  '/' character not found; all done
            break;
        }
    }
	//  Remove the '.ckt' portion
    strPtr = strstr(currentCircuit,".ckt");
    if((strPtr!=NULL)&&(strPtr>currentCircuit)){
		//  if the ".ckt" string is found,
		//  set its location to null to mark end of string
        *strPtr = 0;
    }
	
}

void lev(char *cp)
{
	int i, j, k;
	//NSTRUC *np;
	vector<NSTRUC>::iterator np;
   
	
	
	//  Perform levelization
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
	fprintf(fptr,"%s\n", currentCircuit);
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
bool readFaultList(char *faultFile){
	//  Read the Fault List
	//  Format Example:
	//   1@0
	//   2@1
	//   21@0
	//   ...
	FILE *fd;
	fd = fopen(faultFile,"r");
	if(fd == NULL) {
		printf("\nFile %s cannot be opened\n", faultFile);
		return false;
	}
	FaultV.clear();
	FSTRUC tempFault;
	tempFault.faultFound.clear();
	while(fscanf(fd, "%d@%d", &tempFault.ref, &tempFault.stuckAt) != EOF){
		FaultV.push_back(tempFault);
	}
	fclose(fd);
	
	return true;
}

bool readTestPatterns(char *patternFile){
	//  Read test patternf ile
	//  First line is list of PI's
	//  Subsequent lines are logic corresponding to those PI's.
	fstream fptrIn;
	string lineStr;
	fptrIn.open(patternFile, ios::in);
	char foo;
	int tempPI;
	vector<char> tempPattern;
	
	//  Open file
	if(!fptrIn.is_open()){
		printf("File %s cannot be read!\n", patternFile);
		return false;
	}
	PI_list.clear();//  Clear list of PI's
	inputPatterns.clear();//  Clear loaded test patterns
	bool firstLine = true;
	while(getline(fptrIn, lineStr)){
		if(firstLine){
			// First line is list of PI's
			stringstream ss(lineStr);
			while(ss>>tempPI){ //  Convert ascii to integer
				//  Add this PI reference to the list
				PI_list.push_back(tempPI);
				//PI_list.push_back(NodeV[ref2index[tempPI]]);
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
			inputPatterns.push_back(tempPattern);			
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
		NSTRUC *np = getNodePtr(PI_list[j]);
		if(np->nodeType != PI){
			printf("\nWarning, input file %s contains inputs that are not PI\n", patternFile);
			return false;
		}
	}
	for(int j=0;j<inputPatterns.size();j++){
		//  Check that each test pattern size is equal to the PI list size
		if(inputPatterns[j].size()!=PI_list.size()){
			printf("\nWarning, input file %s test pattern size does not match PI size",patternFile);
			return false;
		}
	}
	/////////////////////////////////////////////////////////
	
	return true;
}

void writeOutputPatterns(char *fileName){
	FILE *fptrOut;
	
	fptrOut = fopen(fileName,"w");
	if(fptrOut == NULL) {
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

void writeInputPatterns(char *fileName, bool firstLine){
	//  Function to write the input test patterns
	//  The "firstLine" indicator means this is the first write
	//  and should include the PI names as a header
	
	FILE *fptrOut;
	
	
	if(firstLine){
		//  Ovewrite existing file
		fptrOut = fopen(fileName,"w");
		if(fptrOut == NULL) {
			printf("File %s cannot be written!\n", fileName);
			return;
		}
		//  Write header
		//  First line is list of PO's
		for(int k = 0;k<PI_Nodes.size();k++){
			fprintf(fptrOut,"%d",PI_Nodes[k]);
			//  If this is the last entry, line return else comma
			if(k<(PI_Nodes.size()-1)){
				fprintf(fptrOut,","); 
			}else{
				fprintf(fptrOut,"\n");
			}
		}
		
	}else{
		//  Append existing file
		fptrOut = fopen(fileName,"a");
		if(fptrOut == NULL) {
			printf("File %s cannot be written!\n", fileName);
			return;
		}
	}
	
	//  Write each input pattern
	for(int patt=0;patt<inputPatterns.size();++patt){
		for(int k = 0;k<inputPatterns[patt].size();k++){
			//  Write each value
			fprintf(fptrOut,"%c",inputPatterns[patt][k]);
			//  Print a , or a line-return if this is last entry
			if(k<(inputPatterns[patt].size()-1)){
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
	
	fptrOut = fopen(fileName,"w");
	if(fptrOut == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
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
	
	printf("==> Writing file of Faults Detected: %s\n",fileName);
}

void writeAllFaults(char *cp){
	//  Function to write all possible faults 
	//  For debug
	FILE *fptr;
	NSTRUC *np;
	char fileName[MAXLINE];
	
	sscanf(cp, "%s", fileName);
	
	fptr = fopen(fileName,"w");
	if(fptr == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
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
	
	printf("==> Writing File of All Faults: %s\n",fileName);
}

void writeFaultCoverageReport(vector<float> faultCoverage,char *fileName){
	FILE *fptr;
	
	fptr = fopen(fileName,"w");
	if(fptr == NULL) {
		printf("File %s cannot be written!\n", fileName);
		return;
	}
	
	for(int i=0;i<faultCoverage.size();++i){
		fprintf(fptr,"%0.2f\n",100*faultCoverage[i]);
	}
		
	//  Done writing file
	fclose(fptr);
	
	printf("\n==> Writing Fault Coverage File: %s\n",fileName);
}


void writeSingleReport_Dalg(void){
	// Write a single report for the output and input
	FILE *fptr;
	NSTRUC *np;
	
	char filename[MAXLINE];
	
	//  First write Input Test Pattern
	sprintf(filename,"%s_DALG_%d@%d.txt",currentCircuit, faultyNode_Dalg, stuckAt_Dalg);
	fptr = fopen(filename,"w");
	if(fptr == NULL) {
		printf("File %s cannot be written!\n", filename);
		return;
	}	
	for(int i=0;i<PI_Nodes.size();++i){
		np = getNodePtr(PI_Nodes[i]);
		if(i==0){
			fprintf(fptr,"%d",np->ref);
		}else{
			fprintf(fptr,",%d",np->ref);
		}
	}
	fprintf(fptr,"\n",np->ref);
	for(int i=0;i<PI_Nodes.size();++i){
		np = getNodePtr(PI_Nodes[i]);
		if(i==0){
			fprintf(fptr,"%s",logicname(np->logic5));
		}else{
			fprintf(fptr,",%s",logicname(np->logic5));
		}
	}	
	//  Done writing file
	fclose(fptr);
	

	printf("\n==> Writing D-algorithm single result: %s\n",filename);
	
}



/*------Miscellaneous-----------------------------------------------------
-----------------------------------------------------------------------*/
int intCeil(int A,int B){
	//  Find the ceiling of A/B
	return ((A+B-1)/(B));
}
NSTRUC *getNodePtr(int ref){
	//  Return the pointer to the node of the given reference #
	
	return &NodeV[ref2index[ref]];
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
   inputPatterns.clear();
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
		printf("\r%5d   %s     %3d   %s\t", np->ref, logicname(np->logic5), np->level, gname(np->gateType));
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
	np = getNodePtr(nodeRef);
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

void printInputPatterns(void){
	int j, k;
	printf("PI List:\n");
	for(k=0;k<PI_list.size();k++){
		printf("%d,",PI_list[k]);
	}
	printf("\nTest Patterns:\n");
	for(j=0;j<inputPatterns.size();j++){
		for(k=0;k<inputPatterns[j].size();k++){
			printf("%c,",inputPatterns[j][k]);
		}
		printf("\n");
	}
	
}

void printFaultList(void){
	vector<FSTRUC>::iterator iterF;
	printf("\nFault List:\n");
	for(iterF = FaultV.begin();iterF!=FaultV.end();++iterF){
		printf("%d@%d  %d\n",iterF->ref, iterF->stuckAt, iterF->faultFound.size());
		
	}
	if(FaultV_Dropped.size()>0){
		printf("\nDropped Fault List:\n");
		for(iterF = FaultV_Dropped.begin();iterF!=FaultV_Dropped.end();++iterF){
			printf("%d@%d  %d\n",iterF->ref, iterF->stuckAt, iterF->faultFound.size());
		
		}
	}
	
	
}


const char *logicname(int tp)
{
   switch(tp) {
      case 0: return("0");
      case 1: return("1");
      case 2: return("X");
      case 3: return("D");
      case 4: return("Dbar");
      case 5: return("NOT");
      case 6: return("NAND");
      case 7: return("AND");
	  case 8: return("XNOR");
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
   printf("PFS inputPatterns inputFaults outputFaultsFound - ");
   printf("Performs parallel fault simulation\n");
   printf("RTG NTotal Ntfcr testPatterns FC report - ");
   printf("Performs Random Test Generation; parallel fault simulation\n");
   printf("QUIT - ");
   printf("stop and exit\n");
}


/*========================= End of program ============================*/

