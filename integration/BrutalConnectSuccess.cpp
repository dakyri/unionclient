//============================================================================
// Name        : ConnectSuccess.cpp
// Author      : 
// Version     :
// Copyright   : My copyright notice
// Description : MAF in C++, Ansi-style
//============================================================================


//============================================================================
// Evil stress test. Exposes many things, not least out of order deletion of
// statics.
//============================================================================


using namespace std;
#include "UnionClient.h"
#include "connector/AbstractConnector.h"
#include "connector/CoutConnector.h"
#include "connector/StandardConnector.h"

#include <thread>
#include <chrono>
#ifndef _MSC_VER
#include <unistd.h>
#endif

StandardUVConnector upcConnector("localhost", "9100");

std::string host("localhost");
std::string service("9100");

float loginTimeout = 20;


int main(int argc, char **argv) {

	UnionClient unionClient(upcConnector);
	unionClient.GetDefaultLog().SetLevel(4);

	cerr << "About to Connect ..." << endl;
	unionClient.Connect();
	unionClient.Disconnect();

	cerr << "Disconnected." << endl;

	return 0;
}

