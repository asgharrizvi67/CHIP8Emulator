#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>

using namespace std; 

int main(int argc, char* argv[]) {
	// Each instruction is 2 bytes. 
	// Take file from command line, and print out the instruction. 
	//
	if(argc < 2) {
		cout << "Please enter a filename." << endl; 
		return 1; 
	}

	cout << "Opening file: " << argv[1] << " ..." << endl;
	ifstream is{argv[1], ifstream::binary};
	if(is) {
		is.seekg(0, is.end);
		int length = is.tellg(); 
		is.seekg(0, is.beg);
		char* codebuffer = new char[length];
	
		cout << "Reading file of length " << length << "..." << endl; 

		is.read(codebuffer, length);
		
		cout << "Printing CHIP8 Instructions" << endl;

		for(int i = 0; i < length; i++) {
			printf("%d: %x\n", i, (unsigned char)(codebuffer[i])); 
		}

		is.close(); 
		delete[] codebuffer; 
	} else {
		cout << "Could not create input stream!" << endl; 
		return 2; 
	}

}
