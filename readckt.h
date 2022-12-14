#ifndef READCKT_H
#define READCKT_H

#include "includes.h"
#include "structures.h"
#include "Circuit.h"
#include "cktNode.h"

#define NUMFUNCS 12
#define MAXLINE 100               /* Input buffer size */
#define MAXNAME 31               /* File name size */
#define N_DROP	5	  	//Drop faults after detected this many times

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

/*----------------  Function Declaration -----------------*/
void clear(void);
const char *gname(int );
const char *nname(int );
void levelizeNodes(void);
void genNodeIndex(void);
int intCeil(int,int);

//  Logic Simulation
void processNodeQueue(void);
void simNode(int );
void single_dfs(int );
void dfs_logicSim(char *, int);
void addPiNodesToQueue(void);
void addNodeToQueue(int );
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

//   Display printouts
void printInputPatterns(void);
void printFaultList(void);

//  Node Setup
void setPI_forPLS(int, int);
void setPI_forPFS(int);
void setFaults(int, int);
void resetFaultMasks(void);

//  File Read/Write
bool readFaultList(char *);
bool readTestPatterns(char *);
void writeOutputPatterns(char *);
void writeInputPatterns(char *, bool);
void writeFaultsDetected(char *);
void writeAllFaults(char *);
void writeFaultCoverageReport(vector<float>,char *);

/*----------------- Command definitions --------------*/
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

enum e_state {EXEC, CKTLD};         /* Gstate values */

struct cmdstruc {
   char name[MAXNAME];              /* command syntax */
   void (*fptr)(char *);            /* function pointer of the commands */
   enum e_state state;              /* execution state sequence */
};

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
};
#endif