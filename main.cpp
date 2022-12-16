/* Kyle Tseng, Kousthubh Dixit, Kris Fausnight*/


#include "main.h"

Circuit * ckt = NULL;
enum e_state Gstate;
int Done = 0;

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
	
    cout << buf << "\n";

   delete ckt;
   ckt = new Circuit(buf);

   Gstate = CKTLD;
	printf("==> OK\n");
}

void pc(char *cp){
   printf("Node    Logic   Level   Type    In      \t\t\tOut\n");
	printf("------  ------  ------  ------  ------  \t\t\t-------\n");
   
   cktMap nodes = ckt->getNodes();
   for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
      printf("\t\t\t\t\t\t\t\t");
      printf("\r%5d   %s     %3d   %s\t", it->first, it->second->getValue(), it->second->getLevel(), it->second->getGateType());
      for (int i = 0; i < it->second->getNumFanOuts(); i++) {
         printf("%d ",it->second->getDownstreamIDs());
      }

      for (int i = 0; i < it->second->getNumFanIns(); i++) {
         printf("%d ", it->second->getDownstreamIDs()[i]);
      }
   }

   printf("Primary inputs:  ");
   for(int i = 0;i < ckt->getPINodeList().size();i++) {
	   printf("%d ",ckt->getPINodeList()[i]);
	}
	printf("\n");
	printf("Primary outputs: ");
	for(int i = 0;i < ckt->getPONodeList().size();i++) {
      printf("%d ",ckt->getPONodeList()[i]);
	}
	printf("\n\n");
	printf("Number of nodes = %d\n", nodes.size());
	printf("Number of primary inputs = %d\n", ckt->getPINodeList().size());
	printf("Number of primary outputs = %d\n", ckt->getPONodeList().size());
}

void quit(char*){
   Done = 1;
}

void lev(char *cp) {
   FILE *fptr;
	char buf[MAXLINE];
	sscanf(cp, "%s", buf);
   fptr = fopen(buf, "w");

   if((fptr = fopen(buf,"w")) == NULL) {
		printf("File %s cannot be written!\n", buf);
		return;
	}else{
		printf("==> Writing file: %s\n",buf);
	}

   fprintf(fptr, "%s\n", ckt->getCktName());
   fprintf(fptr, "#PI: %d\n", ckt->getNumPI());
   fprintf(fptr, "#PO: %d\n", ckt->getNumPO());
   fprintf(fptr, "#Nodes: %d\n", ckt->getNodes().size());
   fprintf(fptr, "#Gates: %d\n", ckt->getNumGates());

   int i;
   cktMap nodes = ckt->getNodes();
   for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
      fprintf(fptr, "%d %d \n", it->first, it->second->getLevel());
   }

   fclose(fptr);

   printf("==> OK\n");
}

void rtg(char* cp) {}
void pfs(char* cp) {}
void dalg(char* cp) {}

inputList * readTestPatterns(char *filename){
	//  Read test patternf ile
	//  First line is list of PI's
	//  Subsequent lines are logic corresponding to those PI's.
	//  Open file
   ifstream inputFile(filename);
   string currLine;
   vector<int> nodeIDs;

   inputList * inputVectors = new inputList();

   if (inputFile.good()) {
      getline(inputFile, currLine);
      int ID;
      for (int i = 0; i < ckt->getNumPI(); i++) {
         istringstream ss(currLine);
         ss >> ID;
         nodeIDs.push_back(ID);
      }

      while (getline(inputFile, currLine)) {
         vector<int> inputs;
         istringstream ss(currLine);
         int in;
         for (int i = 0; i < ckt->getNumPI(); i++) {
            ss >> in;
            inputs.push_back(in);
         }

         inputVectors->push_back(ckt->createInputVector(nodeIDs, inputs));
      }
   }
   return inputVectors;
}

faultList * readFaults(char* filename) {
   ifstream inputFile(filename);
   string currLine;
   faultList* faults = new faultList();

   if(inputFile.good()) {
      while(getline(inputFile, currLine)) {
         int id, sav;
         istringstream ss(currLine);
         ss >> id >> sav;
         Fault* fault = ckt->createFault(id, sav);
         faults->push_back(fault);
      }
   }
   return faults;
}

void logicSim(char *cp) {
	//  Read File	
	//  Input in form "fileToRead.txt fileToWrite.txt"
	char patternFile[MAXLINE];
	char writeFile[MAXLINE];
	sscanf(cp, "%s %s", patternFile, writeFile);

   FILE *fptrOut;
   fptrOut = fopen(writeFile, "w");
	if(fptrOut == NULL) {
		printf("File %s cannot be written!\n", writeFile);
		return;
	}else{
		printf("==> Writing file of PO outputs: %s\n",writeFile);
	}

	inputList* testVectors = readTestPatterns(patternFile);
   cktList POs = ckt->getPONodeList();
   for (int i = 0; i < POs.size(); i++) {
         fprintf(fptrOut, "%d,", POs[i]->getNodeID());
   }
   fprintf(fptrOut, "\b\n");

   for (int i = 0; i < testVectors->size(); i++) {
      ckt->simulate((*testVectors)[i]);
      for (int j = 0; j < POs.size(); j++) {
         fprintf(fptrOut, "%d,", POs[i]->getValue());
      }
      fprintf(fptrOut, "\b\n");
   }
   fclose(fptrOut);

	printf("\n==> OK\n");
}

void rfl(char* cp) {
   FILE *fptr;
	char writeFile[MAXLINE];
	sscanf(cp, "%s", writeFile);

   faultList reducedFaults = ckt->generateFaults(true);

   for (int i = 0; i < reducedFaults.size(); i++) {
      fprintf(fptr, "%d@%d\n", reducedFaults[i]->getNode()->getNodeID(),
                  reducedFaults[i]->getSAV());
   }

   fclose(fptr);
   printf("\n==OK\n");
}

void dfs(char* cp) {
   char readFile[MAXLINE];
   char writeFile[MAXLINE];

   sscanf(cp, "%s %s", readFile, writeFile);
   inputList* testPatterns = readTestPatterns(readFile);
   faultList rflist = ckt->generateFaults(true);
   faultMap* faultSimResults = ckt->deductiveFaultSim(&rflist, testPatterns);

   FILE* fptr;
   if((fptr = fopen(writeFile,"w")) == NULL) {
		printf("File %s cannot be written!\n", writeFile);
		return;
	}else{
		printf("==> Writing file: %s\n", writeFile);
	}

   fprintf(fptr, "%s\n", ckt->getCktName());
   fprintf(fptr, "# of Vectors that detected a fault: %d", faultSimResults->size());
   int testNum = 0;
	int numFaults = 0;
	for (faultMap::iterator it = faultSimResults->begin(); it != faultSimResults->end(); ++it) {
		fprintf(fptr, "Test Vector\t#%d: %s tests:\n", testNum, it->first);
		for (int i = 0; i < it->second->size(); i++) {
         fprintf(fptr, "%s\n", it->second->at(i));
			numFaults++;
		}
		cout << "\n\n";
		testNum++;
	}
   fprintf(fptr, "# of faults detected: %d", numFaults);
   fclose(fptr);

   printf("\n==> OK\n");
}

void printNode(char* cp) {
   int nodeID;
   sscanf(cp, "%d", nodeID);
   cktNode node = ckt->getNode(nodeID);
   printf("(node=%d, value=%s, US=<", node.getValue());

   for (int i = 0; i < node.getUpstreamList().size(); i++) {
      printf("%d,", node.getDownstreamIDs()[i]);
   }
   printf("\b>, DS=<");
   for (int i = 0; i < node.getDownstreamList().size(); i++) {
      printf("%d,", node.getDownstreamIDs()[i]);
   }
   printf("\b>)\n");
   printf("==> OK\n");
}

void podem(char* cp) {
   int nodeID, sav;
   sscanf(cp, "%d %d", nodeID, sav);
   Fault* fault = ckt->createFault(nodeID, sav);
   inputMap* testVector = ckt->PODEM(fault);

   printf("Test vector for %d@%d:\n%s\n", nodeID, sav, testVector);
   printf("==> OK\n");
}

void atpg_det(char* cp) {
   char cktFile[MAXLINE];
   char alg[MAXLINE];

   sscanf(cp, "%s %s", cktFile, alg);
   delete ckt;
   ckt = new Circuit(cktFile);

   faultList faults = ckt->generateFaults(true);
   inputList testVectors;
   faultList detectedFaults;
   for (int i = 0; i < faults.size(); i++) {
      inputMap* tv = ckt->PODEM(faults[i]);
      if (tv != NULL) {
         testVectors.push_back(tv);
         detectedFaults.push_back(faults[i]);
      }
   }

   printf("Algorithm: %s\n", alg);
   printf("Circuit: %s\n", ckt->getCktName());
   printf("Fault Coverage: %f\%\n", ckt->faultCoverage(&detectedFaults));

}

void atpg(char* cp) {
   char cktFile[MAXLINE];
   char alg[MAXLINE];

   sscanf(cp, "%s %s", cktFile, alg);
   delete ckt;
   ckt = new Circuit(cktFile);

   ckt->atpg();
}


