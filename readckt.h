#ifndef READCKT_H
#define READCKT_H

#include "includes.h"
#include "structures.h"
#include "defines.h"
//#include "Circuit.h"
//#include "cktNode.h"



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
void dfs_logicSim(int patt);
void dfs_logicSim(char *, int);
void addPiNodesToQueue(void);
void addNodeToQueue(vector< pair<int,int> >& queue, int nodeRef);
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
void lSim(char *cp);

//   Display printouts
void printInputPatterns(void);
void printFaultList(void);

//  Node Setup
void setPI_forPLS(int, int);
void setPI_forPFS(int);
void setFaults(int, int);
void resetFaultMasks(void);
void reducedFL();

//  File Read/Write
bool readFaultList(char *);
bool rtp(char *);
void writeOutputPatterns(char *);
void writeInputPatterns(char *, bool);
void writeFaultsDetected(char *);
void writeAllFaults(char *);
void writeFaultCoverageReport(vector<float>,char *);

bool setup_Dalg(void);
void parallelFS(char *cp);
const char *logicname(int tp);
NSTRUC *getNodePtr(int ref);
void getCircuitNameFromFile(char *fileName);
int nInputsX_Dalg(int nodeRef);
bool backward_logic(int nodeRef, bool &isJ, vector<int>& inputChangedList);
enum e_logicType checkLogic_Dalg(int nodeRef, bool &isOK, bool &isJ, bool &isD);
void branchPropogate_Dalg(int ref_B,enum e_logicType setLogic,  int origin);
void printNode_Dalg(int nodeRef);
void reloadState_Dalg(set<int>& J_front_save, set<int>& D_front_save, vector< pair<int,enum e_logicType> >& logicState);
int propogate_Jfrontier_Dalg(int nodeRef, int gateRef);
void saveState_Dalg(set<int>& J_front_save, set<int>& D_front_save, vector< pair<int,enum e_logicType> >& logicState);
void propogate_Dfrontier_Dalg(int nodeRef, bool XOR_version);
bool backwardsImply_Dalg(void);
bool forwardImply_Dalg(void);
void printFrontiers_Dalg(void);
bool D_algorithm(void);
int faultAtPO_Dalg(void);
void addInputPattern_Dalg(void);
void writeAtpgReport(char *algType, double elapsedTime);
void resetNodes_Dalg(void);
void writeSingleReport_Dalg();


#include "readckt.cpp"
#endif