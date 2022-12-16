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


#include "readckt.cpp"
#endif