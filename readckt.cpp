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
   enum e_ntype ntype;  
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
void single_dfs(int );
void dfs_logicSim(char *, int);
/*----------------- Command definitions ----------------------------------*/
#define NUMFUNCS 8
void cread(char *);
void pc(char *);
void help(char *);
void quit(char *);
void lev(char *);
void logicSim(char *);
void logicInit(void);
void rfl(char *);
void multi_dfs(char*);
struct cmdstruc command[NUMFUNCS] = {
	{"READ", cread, EXEC},
	{"PC", pc, CKTLD},
	{"HELP", help, EXEC},
	{"QUIT", quit, EXEC},
	{"LEV", lev, CKTLD},
	{"LOGICSIM", logicSim, CKTLD},
	{"RFL", rfl, CKTLD},
	{"DFS", multi_dfs, CKTLD},
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
int dfs_count = 1;
std::vector<NSTRUC> NodeV;
std::vector<NSTRUC> PI_Nodes;
std::vector<NSTRUC> PO_Nodes;
std::vector<int> index2ref;
std::vector<int> ref2index;
std::vector<int> nodeQueue;
std::map<int, vector<int> > dfs_fault_list;
std::map<int, int> fault_vals, previous_logic;
vector<int> PI_list, final, prevLogic, reVisit;
vector<vector<char> > inputPatterns;
vector<vector<int> > int_inputPatterns;

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
int main()
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
	std::vector<NSTRUC>::iterator printnode, outitr;
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
	//  Done with input file; close it out
	fclose(fptr);
	
	
	//  Levelize Nodes
	levelizeNodes();
	
	//  Simulate logic
	processNodeQueue();
	
	
	//  Print Confirmation
	printf("==> OK\n");
	// Write to output file
    outitr = PO_Nodes.begin();
	if((fptr = fopen(writeFile,"w")) == NULL) {
		printf("File %s cannot be read!\n", readFile);
		return;
	}
    for(printnode=NodeV.begin();printnode!=NodeV.end();printnode++) 
	{
		if(outitr->indx == printnode->indx){
			fprintf(fptr,"%d, %d \n",printnode->ref,printnode->logic);
			if(outitr!=PO_Nodes.end()){
				outitr++;
			}
		}
		
	}
   fclose(fptr);
	
}

void dfs_logicSim(char *cp, int i)
{
	std::vector<NSTRUC>::iterator printnode, outitr;
	vector<vector<char> >::iterator ch;
	FILE *fptr;
	int j;
	vector<int>::iterator inp_cnt;
	NSTRUC *np;
	inp_cnt = PI_list.begin();
	for(j = 0; j < int_inputPatterns[i].size(); j++){
		//  Find the node for given PI
		np = &NodeV[ref2index[PI_list[j]]];
		//  Check if this is actually a PI
		if(np->type!=IPT){
			printf("Node %d is not a PI\n",PI_list[j]);
			return;
		}
		//  Update logic of PI
		np->logic = int_inputPatterns[i][j];
		//  Add these PI's to the queue to simulate logic
		nodeQueue.push_back(PI_list[j]); 
   	}
	levelizeNodes();
	processNodeQueue();
	printf("==> OK\n");
}
void processNodeQueue(void){
	//  Function to process all of the nodes in the queue
	//  Need to determine current logic of these nodes based on the inputs
	//  Assumes levelization has already occurred.
	NSTRUC *np;
	
	//  Start at level 0, work up
	int curLevel = 0;
	while (nodeQueue.size()>0){
		for(int i=0;i<nodeQueue.size();i++){
			//  Get pointer to current node in queue.
			np = &NodeV[ref2index[nodeQueue[i]]];
			//  Check if level is low enough to proceed
			if(np->level<=curLevel){
				//  Logic simulation of this node
				simNode(np->ref);
				//  Now add all downstream nodes to the queue
				for(int k = 0;k<np->fout;k++){
					nodeQueue.push_back(np->downNodes[k]);
				}
				//  Done; remove this node
				nodeQueue.erase(nodeQueue.begin()+i);
				
			}
		
		}
		//  Move to next level
		curLevel++;
		
	}
}

void simNode(int nodeRef){
	//  Function to determine current logic of this node
	//  based on logic of upstream nodes.
	NSTRUC *np;
	int i;
	std::vector<bool> inputs;
	bool firstIn;
	
	//  Get pointer to current node
	np = &NodeV[ref2index[nodeRef]];
	
	if(np->type == IPT){
		//  Input; logic already set
		//  Nothing to do here, return
		return;
	}
	
	if(np->fin==0){
		//  No upstream nodes
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
			//  Set equal to the input
			np->logic = inputs[0];
			return;
			break;
		case XOR:
			//  If any inputs are different than the first one
			//  then true can be returned.
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
		case AND:
			//  If any inputs are false, return false
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
			//  If any inputs are false, return true
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==false){
					np->logic = true;
					return;
				}
			}
			np->logic = false;
			return;
			break;
		case OR:
			//  If any inputs are true, return true
			for(i=0;i<inputs.size();i++){
				if(inputs[i]==true){
					np->logic = true;
					return;
				}
			}
			np->logic = false;
			return;
			break;
		case NOR:
			//  If any inputs are true, return false;
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
			//  Return the inverse of the input
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

bool readTestPatterns(char *patternFile){
	fstream fptrIn;
	string lineStr;
	fptrIn.open(patternFile, ios::in);
	char foo;
	int temp;
	vector<char> tempPattern;
	
	if(!fptrIn.is_open()){
		printf("File %s cannot be read!\n", patternFile);
		return false;
	}
	PI_list.clear();
	inputPatterns.clear();
	int_inputPatterns.clear();
	bool firstLine = true;
	while(getline(fptrIn, lineStr)){
		if(firstLine){
			stringstream ss(lineStr);
			while(ss>>temp){ 
				PI_list.push_back(temp);
				ss>>foo;
			}
			firstLine = false;
		}else{
			tempPattern.clear();
			int i = 0;
			while(i<lineStr.size()){
				tempPattern.push_back(lineStr[i]);
				i=i+2;
			}
			inputPatterns.push_back(tempPattern);			
		}
	}
	fptrIn.close();
	for(int j=0; j<inputPatterns.size();j++){
		int_inputPatterns.push_back(std::vector<int>());
		for(int k=0; k<inputPatterns[j].size(); k++){
			int_inputPatterns[j].push_back(int(inputPatterns[j][k])-48);
		}
	}
	for(int j=0;j<PI_list.size();j++){
		if(PI_list[j]>(ref2index.size()-1)){
			printf("\nWarning, input file %s contains inputs exceeding range\n", patternFile);
			return false;
		}
		if(NodeV[ref2index[PI_list[j]]].type != IPT){
			printf("\nWarning, input file %s contains inputs that are not PI\n", patternFile);
			return false;
		}
	}
	for(int j=0;j<inputPatterns.size();j++){
		if(inputPatterns[j].size()!=PI_list.size()){
			printf("\nWarning, input file %s test pattern size does not match PI size",patternFile);
			return false;
		}
	}
	return true;
}

void multi_dfs(char *cp){
	dfs_fault_list.clear();
	fault_vals.clear();
	inputPatterns.clear();
	int_inputPatterns.clear();
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
	readTestPatterns(readFile);
	for(np = NodeV.begin(); np!= NodeV.end(); np++){
			if(np->ref > max){
				max = np->ref;
		}
	}
	for(i = 0; i < int_inputPatterns.size(); i++)
	{
		dfs_logicSim(cp, i);
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
			}
			x++;
		}
		dfs_count++;
		for(np = PO_Nodes.begin(); np!= PO_Nodes.end(); np++)
		{
			for(it = dfs_fault_list[np->ref].begin(); it != dfs_fault_list[np->ref].end(); it++){
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
	NSTRUC *np = &NodeV[ref2index[val]];
	for(j = 0; j < np->fin; j++){
		inNode = &NodeV[ref2index[np->upNodes[j]]];
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
	vector<NSTRUC>::iterator nstr_it;
	char readFile[MAXLINE];
	NSTRUC *np, *inNode, *ptr1, *ptr2;
	np = &NodeV[ref2index[val]];
	for(i = 0; i < np->fin; i++){
		if(dfs_count == 1){
			if(dfs_fault_list.find(np->upNodes[i]) == dfs_fault_list.end()){
				reVisit.push_back(np->upNodes[i]);
				single_dfs(np->upNodes[i]);
			}
		}	
	}
	switch (np->type){
		case IPT:
			output.clear();
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		case BRCH:
			output.clear();
			for(j = 0; j < np->fin; j++){
				inNode = &NodeV[ref2index[np->upNodes[j]]];
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
				inNode = &NodeV[ref2index[np->upNodes[j]]];
				for(ind = 0; ind < dfs_fault_list[inNode->ref].size(); ind++)
					output.push_back(dfs_fault_list[inNode->ref][ind]);
			}
			output.push_back(np->ref);
			dfs_fault_list.insert(pair<int,vector<int> >(np->ref, output));
			fault_vals.insert(pair<int, int>(np->ref, (1-np->logic)));
			break;
		case XOR:
			output.clear();
			output1.clear();
			output2.clear();
			ptr1 = &NodeV[ref2index[np->upNodes[0]]];
			ptr2 = &NodeV[ref2index[np->upNodes[1]]];
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
			printf("Node type %d not recognized\n",np->type);
	}
	
}
void rfl(char *cp)
{
	std::vector<NSTRUC>::iterator np;
	int checkpoints = Npi;
	int j;
	FILE *fptr;
	char writeFile[MAXLINE];
	sscanf(cp, "%s", writeFile);
	if((fptr = fopen(writeFile,"w")) == NULL) {
		printf("File %s cannot be read!\n", writeFile);
		return;
	}
   
	for(np = NodeV.begin(); np!= NodeV.end(); np++){
		if(np->type == 0){
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
	fclose(fptr);
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

   
	printf(" Node   Type \tIn     \t\t\tOut     \t\tLogic\n");
	printf("------ ------\t-------\t\t\t-------\n");
	for(np=NodeV.begin();np!=NodeV.end();np++) {
      
		printf("\t\t\t\t\t");
		for(j = 0; j<np->fout; j++) 
			printf("%d ",np->downNodes[j]);
		printf("\t\t\t %d", np->logic);
		printf("\r%5d  %s\t", np->ref, gname(np->type));
		for(j = 0; j<np->fin; j++) 
			printf("%d ",np->upNodes[j]);
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

