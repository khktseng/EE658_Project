/*=======================================================================
  A simple parser for "self" format

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




                                    Author: Chihang Chen
                                    Date: 9/16/94

=======================================================================*/


#include <cstdio>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vector>

using namespace std;

#define MAXLINE 81               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

enum e_state {EXEC, CKTLD};         /* Gstate values */
/* Node Type, column 1 of circuit format */
enum e_ntype {
	GATE = 0, 
	PI = 1, 
	FB = 2, 
	PO = 3,
};    
//  gate types, Column 3 of circuit format	
enum e_gtype {
	IPT = 0, 
	BRCH = 1, 
	XOR = 2, 
	OR = 3, 
	NOR = 4, 
	NOT = 5, 
	NAND = 6, 
	AND = 7,
};  

struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   void (*fptr)(char *);             /* function pointer of the commands */
   enum e_state state;        /* execution state sequence */
};

typedef struct n_struc {
   unsigned indx;             /* node index(from 0 to NumOfLine - 1 */
   unsigned ref;              /* line number(May be different from indx */
   enum e_gtype type;         /* gate type */
   unsigned fin;              /* number of fanins */
   unsigned fout;             /* number of fanouts */
   std::vector<int> upNodes;
   std::vector<int> downNodes;
   int level;                 /* level of the gate output */
   bool logic;
} NSTRUC;                     

/*----------------  Function Declaration -----------------*/
void allocate(void);
void clear(void);
const char *gname(int );
void levelizeNodes(void);
void genNodeIndex(void);
void processNodeQueue(void);
void processNodeQueue(void);
void simNode(int );
/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 6
void cread(char *);
void pc(char *);
void help(char *);
void quit(char *);
void lev(char *);
void logicSim(char *);
void logicInit(void);
struct cmdstruc command[NUMFUNCS] = {
	{"READ", cread, EXEC},
	{"PC", pc, CKTLD},
	{"HELP", help, EXEC},
	{"QUIT", quit, EXEC},
	{"LEV", lev, CKTLD},
	{"LOGICSIM", logicSim, CKTLD},
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
std::vector<NSTRUC> NodeV;
std::vector<NSTRUC> PI_Nodes;
std::vector<NSTRUC> PO_Nodes;
std::vector<int> index2ref;
std::vector<int> ref2index;
std::vector<int> nodeQueue;
int maxLevels;
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
main()
{
   uint8_t com;
   char cline[MAXLINE], wstr[MAXLINE], *cp;

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



void logicSim(char *cp)
{
	
	//  Read File
	FILE *fptr;
	char readFile[MAXLINE];
	char writeFile[MAXLINE];
	char buf[MAXLINE];
	sscanf(cp, "%s %s", readFile, writeFile);
	
	//Debug //////////
	printf("Read: %s, Write: %s;\n", readFile, writeFile);
	////////////////
	
	//  First open the file of primary inputs
	if((fptr = fopen(readFile,"r")) == NULL) {
		printf("File %s cannot be read!\n", readFile);
		return;
	}
	
	int nPI = 0; //  # of primary inputs found in input file
	
	NSTRUC *np;
	
	//  Cycle through the input file line-by-line
	int tempPI, tempLogic;
	while(fgets(buf, MAXLINE, fptr) != NULL) {
      if(sscanf(buf,"%d, %d", &tempPI, &tempLogic) == 2) {
         //  Success, read a valid line
		 //  Find the node for given PI
		 np = &NodeV[ref2index[tempPI]];
		 //  Check if this is actually a PI
		 if(np->type!=IPT){
			 printf("Node %d is not a PI\n",tempPI);
			 fclose(fptr);
			 return;
		 }
		 //  Update logic of PI
		 np->logic = tempLogic;
		 //  Add these PI's to the queue to simulate logic
		 nodeQueue.push_back(tempPI);
      }
	}
	
	//  Levelize Nodes
	levelizeNodes();
	
	//  Create list of nodes to analyze
	processNodeQueue();
	
	
	
	fclose(fptr);
}
void processNodeQueue(void){
	NSTRUC *np;
	
	int curLevel = 0;
	while (nodeQueue.size()>0){
		for(int i=0;i<nodeQueue.size();i++){
			np = &NodeV[ref2index[nodeQueue[i]]];
			if(np->level<=curLevel){
				simNode(np->ref);
				nodeQueue.erase(nodeQueue.begin()+i);
				for(int k = 0;k<np->fout;k++){
					nodeQueue.push_back(np->downNodes[k]);
				}
			}
		
		}
		curLevel++;
		
	}
	
}

void simNode(int nodeRef){
	NSTRUC *np;
	int i;
	std::vector<bool> inputs;
	bool firstIn;
	
	np = &NodeV[ref2index[nodeRef]];
	
	if(np->type == IPT){
		//  Nothing to do here, return
		return;
	}
	if(np->fin==0){
		//  Nothing to do, return;
		return;
	}
	
	//  Generate array of bool inputs
	NSTRUC *inNode;
	for(i=0;i<np->fin;i++){
		inNode = &NodeV[ref2index[np->upNodes[i]]];
		inputs.push_back(inNode->logic);
	}
	
	
	switch (np-> type){
		case IPT:
			//  Nothing to do here
			break;
		case BRCH:
			//  Set all down-nodes to current logic
			//for( i=0;i<np->fout;i++){
			//	int downNodeRef = np->downNodes[i];
			//	NodeV[ref2index[downNodeRef]].logic = np->logic;
			//}
			np->logic = inputs[0];
			return;
			break;
		case XOR:
			firstIn = inputs[0];
			for(i = 0;i<inputs.size();i++){
				if(inputs[i]!=firstIn){
					np->logic = true;
					return;
				}
			}
			np->logic = false;
			return;
			break;
		case OR:
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==true){
					np->logic = true;
					return;
				}
			}
			np->logic = false;
			return;
			break;
		case AND:
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==false){
					np->logic = false;
					return;
				}
			}
			np->logic = true;
			return;
			break;
		case NAND:
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==false){
					np->logic = true;
					return;
				}
			}
			np->logic = false;
			return;
			break;
		case NOR:
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==true){
					np->logic = false;
					return;
				}
			}
			np->logic = true;
			return;
			break;
		case NOT:
			if(inputs[0]==true){
				np->logic = false;
			}else{
				np->logic = true;
			}
			return;
			break;
		default:
			printf("Node type %d not recognized\n",np->type);
	}
		
		
		
		

	
}
	



/*-----------------------------------------------------------------------
input: circuit description file name
output: nothing
called by: main
description:
  This routine reads in the circuit description file and set up all the
  required data structure. It first checks if the file exists, then it
  sets up a mapping table, determines the number of nodes, PI's and PO's,
  allocates dynamic data arrays, and fills in the structural information
  of the circuit. In the ISCAS circuit description format, only upstream
  nodes are specified. Downstream nodes are implied. However, to facilitate
  forward implication, they are also built up in the data structure.
  To have the maximal flexibility, three passes through the circuit file
  are required: the first pass to determine the size of the mapping table
  , the second to fill in the mapping table, and the third to actually
  set up the circuit information. These procedures may be simplified in
  the future.
-----------------------------------------------------------------------*/
void cread(char *cp)
{
	char buf[MAXLINE];
	int  i, j, k, nd, tp = 0;
	FILE *fd;
	
	
	std::vector<NSTRUC>::iterator nodeIter;

	sscanf(cp, "%s", buf);
	if((fd = fopen(buf,"r")) == NULL) {
		printf("File %s does not exist!\n", buf);
		return;
	}
	
	//  Enter filename into global filename variable
	strcpy(curFile, buf);

	if(Gstate >= CKTLD) clear();
	
	
	Nnodes = Npi = Npo   = Ngates =0;
	fseek(fd, 0L, 0);
	int index=0;
	while(fscanf(fd, "%d %d", &tp, &nd) != EOF) {
		NSTRUC tempNode;
		tempNode.ref = nd;
		tempNode.level = -1;
		tempNode.logic = true;
		tempNode.indx = index++;
		
		//  Keep track of the number of gates
		if (tp>1){
			//  This is a gate
			
		}
		Nnodes ++;
		
		if(tp==PI){
			PI_Nodes.push_back(tempNode);
			Npi++;
		}else if (tp==PO){
			PO_Nodes.push_back(tempNode);
			Npo++;
		}
		
		switch(tp) {
			case PI:

			case PO:

			case GATE:
				fscanf(fd, "%d %d %d", &tempNode.type, &tempNode.fout, &tempNode.fin);
				if(tempNode.type>1){
					Ngates++;
				}
				break;
				
			case FB:
				tempNode.fout = tempNode.fin = 1;
				fscanf(fd, "%d", &tempNode.type);
				break;

			default:
				printf("Unknown node type!\n");
				exit(-1);
        }

		for(i=0;i<tempNode.fin;i++){
			fscanf(fd, "%d", &nd);
			tempNode.upNodes.push_back( nd);
		}
		NodeV.push_back(tempNode);
    }
	//  Done processing through file
	fclose(fd);
	
	genNodeIndex();
	
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		for(j=0;j<nodeIter->fin;j++){
			int ref = nodeIter->upNodes[j];
			NodeV[ref2index[ref]].downNodes.push_back(nodeIter->ref);
			
		}
		
	}
	
   
	
	Gstate = CKTLD;
	printf("==> OK\n");
}

void genNodeIndex(void){
	//  Regenerates reference vectors of the global NodeV vector
	// 	index2ref supplies the reference # for a given index
	//  ref2index supplies the index for a given reference #
	
	std::vector<NSTRUC>::iterator nodeIter;
	int i;
	
	//  Clear vectors first
	index2ref.clear();
	ref2index.clear();
	
	
	//  Create the index2ref array
	//  Also find the maximum value of the reference #
	int maxRef = 0;
	for (nodeIter=NodeV.begin();nodeIter!=NodeV.end();++nodeIter){
		index2ref.push_back(nodeIter->ref);
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

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main
description:
  The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/

void pc(char *cp)
{
	int i, j;
   
	std::vector<NSTRUC>::iterator np;

   
	printf(" Node   Type \tIn     \t\t\tOut     \t\t\tLogic\n");
	printf("------ ------\t-------\t\t\t-------\n");
	for(np=NodeV.begin();np!=NodeV.end();np++) {
      
		printf("\t\t\t\t\t");
		for(j = 0; j<np->fout; j++) printf("%d ",np->downNodes[j]);
		printf("\t\t %d", np->logic);
		printf("\r%5d  %s\t", np->ref, gname(np->type));
		for(j = 0; j<np->fin; j++) printf("%d ",np->upNodes[j]);
		printf("\n");
	}
   
	printf("Primary inputs:  ");
    for(np=PI_Nodes.begin();np!=PI_Nodes.end();np++) {
		printf("%d ",np->ref);
	}
	printf("\n");
	printf("Primary outputs: ");
	for(np=PO_Nodes.begin();np!=PO_Nodes.end();np++) {
		printf("%d ",np->ref);
	}
	printf("\n\n");
	printf("Number of nodes = %d\n", Nnodes);
	printf("Number of primary inputs = %d\n", Npi);
	printf("Number of primary outputs = %d\n", Npo);
   
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
	
	std::vector<NSTRUC>::iterator np;
	
	//  Levelization algorithm	
	int noAction;  //  Indicator to see if any levels are applied
	while(1){
		noAction = 1;
		for(np=NodeV.begin();np!=NodeV.end();++np){
			//  Check to see if level is applied
			if(np->level<0){
				//  No level applied yet
				//  See if level can be applied
				if(np->type == 0){
					// Easy; this is a primary input so assign "0"
					np->level = 0;//  New level applied!
					noAction = 0;
				}else{
					//  Check to see if all of the inputs are levelized yet
					int maxLevel = 0;
					for(int j = 0; j<np->fin; j++){
						int nLevel = getLevel(np->upNodes[j]);
						if (nLevel<0){
							//  1 input not levelized yet; no need to continue
							break;
						}else{
							maxLevel = (maxLevel>nLevel) ? maxLevel : nLevel;
						}
					}
					//  Current level is the maximum of the input levels +1
					np->level = maxLevel+1;
					noAction = 0;//  New level applied!
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
	std::vector<NSTRUC>::iterator np;
   
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
	/*
	//  Debug Print to Console
	printf("%s\n", curFile);
	printf("#PI: %d\n#PO: %d\n",Npi, Npo);
	printf("#Nodes: %d\n#Gates: %d\n",Nnodes, nGates);
	for(i=0;i<Nnodes;i++){
		np = &Node[i];
		printf("%d %d\n",np->num, np->level);
	}
	*/
	
	
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
	fclose(fptr);;
	
	//  Print OK to console
	printf("==> OK\n");
	
}



/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  The routine prints ot help inormation for each command.
-----------------------------------------------------------------------*/
void help(char*)
{
   printf("READ filename - ");
   printf("read in circuit file and creat all data structures\n");
   printf("PC - ");
   printf("print circuit information\n");
   printf("HELP - ");
   printf("print this help information\n");
   printf("QUIT - ");
   printf("stop and exit\n");
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
   Nnodes = 0;
   Npi = 0;
   Npo = 0;
   index2ref.clear();
   ref2index.clear();
   Gstate = EXEC;
}


/*-----------------------------------------------------------------------
input: gate type
output: string of the gate type
called by: pc
description:
  The routine receive an integer gate type and return the gate type in
  character string.
-----------------------------------------------------------------------*/
//char *gname(tp)
//int tp;
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
   }
}
/*========================= End of program ============================*/

