/* Kyle Tseng, Kousthubh Dixit, Kris Fausnight*/


#include "main.h"

Circuit * ckt = NULL;
enum e_state Gstate;
int Done = 0;
char circuitFile[MAXLINE];

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

   printf("RTG numTests reportFile - ");
   printf("Performs Random Test Generation and simulation\n");

   printf("PODEM node sav - ");
   printf("Performs PODEM to find a test vector for node@sav\n");

   printf("ATPG cktFile algorithm - ");
   printf("Performs dynamic ATPG using algorithm on circuit in cktFile\n");

   printf("ATPG_DET cktFile algorithm - ");
   printf("Performs ATPG using algorithm on circuit in cktFile\n");

   printf("LOGICSIM inputFile outputFile - ");
   printf("Reads input vectors from inputFile and writes simulation PO outputs to outputFile\n");

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
	//printf("Read Circuit\n");
	printf("Reading file: %s\n\n",buf);
	//////////////////
	
   delete ckt;
   ckt = new Circuit(buf);

   Gstate = CKTLD;
	printf("==> OK\n");
}

void pc(char *cp){
   printf("Node\tLogic\tLevel\tType\tIn\tOut\n");
	printf("-----  ------  ------  ------  ------  \t-------\n");
   cktMap nodes = ckt->getNodes();

   for (cktMap::iterator it = nodes.begin(); it != nodes.end(); ++it) {
      printf("\t\t\t\t\t\t\t\t");
      int nodeID = it->first;
      int level = it->second->getLevel() + 1;
      LOGIC value = it->second->getValue();
      gateT gateType = it->second->getGateType();

      printf("\r%d\t", nodeID);
      cout << value << "\t";
      printf("%d\t", level);
      cout << gateType << "\t";
      //printf("\r%5d   %s     %3d   %s\t", it->first, it->second->getValue(), it->second->getLevel(), it->second->getGateType());
      for (int i = 0; i < it->second->getNumFanOuts(); i++) {
         printf("%d ",it->second->getDownstreamIDs()[i]);
      }
         printf("\t");
      for (int i = 0; i < it->second->getNumFanIns(); i++) {
         printf("%d ", it->second->getUpstreamIDs()[i]);
      }
      printf("\n");
   }

   printf("Primary inputs:  ");
   for(int i = 0;i < ckt->getPINodeList().size();i++) {
	   printf("%d ",ckt->getPINodeList()[i]->getNodeID());
	}
	printf("\n");
	printf("Primary outputs: ");
	for(int i = 0;i < ckt->getPONodeList().size();i++) {
      printf("%d ",ckt->getPONodeList()[i]->getNodeID());
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

inputList * readTestPatterns(char *filename){
	//  Read test patternf ile
	//  First line is list of PI's
	//  Subsequent lines are logic corresponding to those PI's.
	//  Open file
   ifstream inputFile(filename);
   string currLine;
   vector<int> nodeIDs;

   inputList * inputVectors = new inputList();

   if (inputFile.fail()) {
      return NULL;
   }

   if (inputFile.good()) {
      getline(inputFile, currLine);
      int ID;

      istringstream ss(currLine);
      for (int i = 0; i < ckt->getNumPI(); i++) {
         ss >> ID;
         nodeIDs.push_back(ID);
      }

      while (getline(inputFile, currLine)) {
         istringstream ss(currLine);
         vector<int> inputs;
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
   if (testVectors == NULL) {
      cout << patternFile << " cannot be read.";
      return;
   }

   cktList POs = ckt->getPONodeList();
   for (int i = 0; i < POs.size() - 1; i++) {
         fprintf(fptrOut, "%d,", (*POs[i]).getNodeID());
   }
   fprintf(fptrOut, "%d\n", POs.back()->getNodeID());

   for (int i = 0; i < testVectors->size(); i++) {
      ckt->simulate((*testVectors)[i]);
      for (int j = 0; j < POs.size() - 1; j++) {
         fprintf(fptrOut, "%d,", (*POs[j]).getValue());
      }
      fprintf(fptrOut, "%d\n", POs.back()->getValue());
   }
   fclose(fptrOut);
	printf("\n==> OK\n");
}

void rfl(char* cp) {
   FILE *fptr;
	char writeFile[MAXLINE];
	sscanf(cp, "%s", writeFile);

   faultSet reducedFaults = ckt->generateFaults(true);

   for (faultSet::iterator it = reducedFaults.begin(); it != reducedFaults.end(); ++it) {
      fprintf(fptr, "%d@%d\n", (*it)->getNode()->getNodeID(), (*it)->getSAV());
   }

   fclose(fptr);
   printf("\n==OK\n");
}

void dfs(char* cp) {
   char readFile[MAXLINE];
   char writeFile[MAXLINE];

   sscanf(cp, "%s %s", readFile, writeFile);
   inputList* testPatterns = readTestPatterns(readFile);
   faultSet rflist = ckt->generateFaults(true);
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
   sscanf(cp, "%d %d", &nodeID, &sav);

   Fault* fault = ckt->createFault(nodeID, sav);
   inputMap* testVector = ckt->PODEM(fault);
   cout << "PODEM complete\n-----------------\n";
   if (testVector == NULL) {
      cout << "PODEM could not find a solution.\n";
      return;
   }
   printf("Test vector for %d@%d:\n", nodeID, sav);
   cout << testVector << "\n";
   printf("==> OK\n");
}

void atpg_det(char* cp) {
   char cktFile[MAXLINE];
   char alg[MAXLINE];

   sscanf(cp, "%s %s", cktFile, alg);
   string algorithm = alg;
   double fc;
   struct timeval begin, end;
   inputList testVectors;
   if (algorithm.compare("PODEM") != 0 && algorithm.compare("podem") != 0) {
      printf("Starting DAlg...\n");
      ATPG_DET(cp);
      printf("Algorithm: Dalg\n");
   } else {
      delete ckt;
      string cf = cktFile;
      ckt = new Circuit(cktFile);
      gettimeofday(&begin,0);
      fc = ckt->atpg_det(&testVectors);
      gettimeofday(&end, 0);
      printf("Algorithm: PODEM\n");
      printf("Circuit: %s\n", ckt->getCktName().c_str());
      printf("Fault Coverage: %.2f\%\n", fc);

      long seconds = end.tv_sec - begin.tv_sec;
      long microseconds = end.tv_usec - begin.tv_usec;
      double elapsed = seconds + microseconds*1e-6;
      printf("Finished in %.3f seconds\n", elapsed);
      podemATPGReport(fc, elapsed, &testVectors);
   }
   printf("==> OK\n");
}

void atpg(char* cp) {
   char cktFile[MAXLINE];
   char alg[MAXLINE];

   sscanf(cp, "%s %s", cktFile, alg);
   string algorithm = alg;
   struct timeval begin, end;
   double fc;
   inputList testVectors;
   bool Dalg = false;
   if (Dalg && (strchr(alg,'p') != NULL || strchr(alg, 'P'))) {
      printf("Starting Dalg based ATPG...\n");
      gettimeofday(&begin,0);
   ///dalg

      gettimeofday(&end, 0);
   } else {
      delete ckt;
      ckt = new Circuit(cktFile);
      printf("Starting PODEM based ATPG...\n");
      gettimeofday(&begin,0);
      fc = ckt->atpg(&testVectors);
      gettimeofday(&end, 0);
      //output test vectors
   }

   printf("\nAlgorithm: %s\n", alg);
   printf("Circuit: %s\n", cktFile);
   printf("Fault Coverage: %f\%\n", fc);

   long seconds = end.tv_sec - begin.tv_sec;
   long microseconds = end.tv_usec - begin.tv_usec;
   double elapsed = seconds + microseconds*1e-6;
   printf("Finished in %.3f seconds\n", elapsed);
   podemATPGReport(fc, elapsed, &testVectors);
   printf("==> OK\n");
}

void rtg(char* cp) {
   char reportFile[MAXLINE];
   int nTests;
   double fc;

   sscanf(cp, "%d %f %s", &nTests, &fc, reportFile);

   FILE *fptrOut;
   fptrOut = fopen(reportFile, "w");
	if(fptrOut == NULL) {
		printf("File %s cannot be written!\n", reportFile);
		return;
	}else{
		printf("==> Writing file of PO outputs: %s\n",reportFile);
	}
   inputList randInputs = ckt->randomTestsGen(nTests);

   for (int i = 0; i < randInputs.size(); i++) {
      ckt->reset();
      ckt->simulate(randInputs[i]);
   }

   fprintf(fptrOut, "Test Vector\t\tPOs:\t");
   cktList POs = ckt->getPONodeList();
   for (int i = 0; i < POs.size() - 1; i++) {
         fprintf(fptrOut, "%d\t", (*POs[i]).getNodeID());
   }
   fprintf(fptrOut, "%d\n", POs.back()->getNodeID());

   for (int i = 0; i < randInputs.size(); i++) {
      ckt->simulate(randInputs[i]);
      stringstream ss;
      ss << randInputs[i];
      fprintf(fptrOut, "%s\t", ss.str().c_str());
      for (int j = 0; j < POs.size() - 1; j++) {
         fprintf(fptrOut, "%d\t", (*POs[j]).getValue());
      }
      fprintf(fptrOut, "%d\n", POs.back()->getValue());
   }
   fclose(fptrOut);
	printf("\n==> OK\n");

}
void pfs(char* cp) {}
void dalg(char* cp) {
   DALG(cp);
}

void podemATPGReport(double fc, double elapsedTime, inputList* testVectors) {
   FILE* fptr;
   FILE* pFile;
   char fileName[MAXLINE];
   char patternFile[MAXLINE];
   sprintf(fileName, "ATPG_OUT/%s_PODEM_ATPG_report.txt", ckt->getCktName().c_str());
   sprintf(patternFile, "ATPG_OUT/%s_PODEM_ATPG_patterns.txt", ckt->getCktName().c_str());
   fptr = fopen(fileName, "w");
   pFile = fopen(patternFile, "w");

   if(fptr == NULL) {
      printf("File %s cannot be written.\n", fileName);
      return;
   }

   if(pFile == NULL) {
      printf("File %s cannot be written.\n", patternFile);
      return;
   }

   for (inputMap::iterator it = testVectors->at(0)->begin();
         it != testVectors->at(0)->end(); ++it) {
      fprintf(pFile, "%d ", it->first);
   }
   fprintf(pFile, "\n");
   int temp;
   for (int i = 1; i < testVectors->size(); i++) {
      for(inputMap::iterator it = testVectors->at(i)->begin(); 
                  it != testVectors->at(i)->end(); ++it){
         stringstream ss;
         ss << (LOGIC)it->second;
         fprintf(pFile, "%s ", ss.str().c_str());
      }
      fprintf(pFile, "\n");
   }

   fprintf(fptr, "\nAlgorithm: PODEM\n");
   fprintf(fptr, "Circuit: %s\n", ckt->getCktName().c_str());
   fprintf(fptr, "Fault Coverage: %f\%\n", fc);
   fprintf(fptr, "Time: %0.3f\n", elapsedTime);
   printf("\n==> Writing ATPG report: %s\n",fileName);
   fclose(fptr);
   fclose(pFile);
}

void exit(char*) {
   Done = 1;
}