#ifndef MAIN_H
#define MAIN_H

//#include "readckt.h"
#include "Circuit.h"
#include "includes.h"
#include "defines.h"
#include "structures.h"
#include "readckt.h"

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
void rtg(char *);
void atpg_det(char*);
void atpg(char *);
void podem(char*);
void dalg(char*);
void dfs(char*);
void exit(char*);

void podemATPGReport(double fc, double time, inputList* testVectors);

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
	{"DFS", dfs, CKTLD},
	{"PRINTNODE", printNode, CKTLD},
	{"PFS", pfs, CKTLD},
	{"RTG", rtg,CKTLD},
    {"PODEM", podem, CKTLD},
    {"DALG", dalg, CKTLD},
    {"ATPG_DET", atpg_det, EXEC},
    {"ATPG", atpg, EXEC},
	{"EXIT", exit, EXEC},
	//{"WRITEALLFAULTS", writeAllFaults, CKTLD},
};

#endif