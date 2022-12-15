#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include "includes.h"
#include "Circuit.h"
#include "cktNode.h"
#include "Fault.h"
#include "defines.h"
#include "Logic.h"

using namespace std;

typedef struct testStruct {
	int value;
} TS;
int main(){
	Circuit ckt("circuits/c1.ckt");

	inputMap testInp;
	testInp[1] = ONE;
	testInp[2] = ZERO;
	testInp[5] = X;
	testInp[6] = X;
	testInp[9] = X;

	Fault* testFault = ckt.createFault(1, 0);
	ckt.addFault(testFault);
	ckt.simulate(&testInp);
	ckt.printPO();

	vector<int> nodeIDs = ckt.getNodeIDs();
	faultList faults;
	inputList ins;

	cout << "# of nodes : " << nodeIDs.size() << "\n";

	for (int i = 0 ;i < nodeIDs.size(); i++) {
		for (int j = 0; j < 2; j++) {
			Fault* fault = ckt.createFault(nodeIDs[i], j);
			inputMap* test = ckt.PODEM(fault);
			faults.push_back(fault);
			if (test != NULL) ins.push_back(test);
		}
	}

	cout << "# of faults : " << faults.size() << "\n";
	cout << "# of inputs : " << ins.size() << "\n";

	faultMap fm = ckt.deductiveFaultSim(&faults, &ins);

	
	cout << "# vectors detecting a fault: " << fm.size() << "\n\n";

	for (faultMap::iterator it = fm.begin(); it != fm.end(); ++it) {
		cout << "Test Vector: " << it->first << " tests ";
		cout << it->second->size();
		cout << "\n";
	}


/*
	inputMap input;
	input[1] = X;
	input[2] = X;
	input[3] = ONE;
	input[4] = ZERO;
	input[5] = ZERO;

	ckt.simulate(&input);

	LOGIC n50 = ckt.getNodeLogic(50);
	LOGIC n51 = ckt.getNodeLogic(51);
	LOGIC n52 = ckt.getNodeLogic(52);

	cout << "node 50: " << n50 << "\n";
	cout << "node 51: " << n51 << "\n";
	cout << "node 52: " << n52 << "\n";

	ckt.reset();

	ckt.simulate(&input);

	 n50 = ckt.getNodeLogic(50);
	 n51 = ckt.getNodeLogic(51);
	 n52 = ckt.getNodeLogic(52);

	cout << "node 50: " << n50 << "\n";
	cout << "node 51: " << n51 << "\n";
	cout << "node 52: " << n52 << "\n";
*/


	/*pil2 = pil;
	pil.pop_back();

	cout << pil.size() << " " << pil2.size() << "\n";
	cout << ckt.getMaxLevels() << "\n";

	map<int, vector<int>> themap;

	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
			LOGIC l1 = (LOGIC) i;
			LOGIC l2 = (LOGIC) j;

			//LOGIC LXOR = l1  ^ l2;
			//cout << l1 << "^" << l2 << "= " << LXOR << "\n";
			//LOGIC LOR = l1 | l2;
			//cout << l1 << "|" << l2 << "= " << LOR << "\n";
			//LOGIC LAND = l1 & l2;
			//cout << l1 << "&" << l2 << "= " << LAND << "\n";
		}
	}

	for(int i = 0; i < 4; i++) {
		LOGIC l1 = (LOGIC) i;
		LOGIC LNOT = ~l1;
		//cout <<"~" << l1 << "= " << LNOT << "\n\n";
	}

	cout << sizeof(long) << "\n";

	

	
	ifstream theFile("add2.ckt");
	string line;
	int ref;
	int n1, n2, n3, n4;
	int gateT;
	
	int t = 1;
	vector<int> nums;

	cout << nums.size() << "\n";
	
	nodeT n = (nodeT) t;
	
	//cout << n << "\n \n";
	
	if(theFile.is_open()) {
			theFile >> n1 >> n2 >> n3 >> n4;
			
			cout << n1 << " " << n2 << " " << n3 << " " << n4 << "\n";
			
			sscanf(line.c_str(), "%d %d %d %d", &n1, &n2, &n3, &n4); 
	}*/
}

