//============================================================================
// Name        : ConnectSuccess.cpp
// Author      : 
// Version     :
// Copyright   : My copyright notice
// Description : MAF in C++, Ansi-style
//============================================================================


//============================================================================
//
//  This sample shows a simple successful connection to a union server,
//  and an immediate disconnection afterwards.
//
//  The sample constis of the following steps:
//
//  STEP 1: Create unionClient
//  STEP 2: register event listeners
//  STEP 3: Initiate connection to server
//  STEP 4: wait for a read or connection error event
//  STEP 5: we're connected!
//  STEP 6: Disconnect
//
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

void process_arg_list(int, char**);

int main(int argc, char **argv) {
	process_arg_list(argc, argv);
    //
    //  STEP 1: Create unionClient
    //  --------------------------
    //

	UnionClient unionClient(upcConnector);
    unionClient.GetDefaultLog().SetLevel(0);

    //
    // STEP 2: register event listeners
    // --------------------------------
    //
    // We're registering an event handler for the connection ready
    // and for the connection failure events. We're anticipating the
    // ready event in case of a successful connection.
    //

	bool ready = false;
	bool selected = false;
	bool disconnected = false;
    bool connectError = false;

    // register the a ready event handle
    CBStatusMessageRef readyListenerHandle = unionClient.AddEventListener(Event::READY, [&ready](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Got a 'ready' from the server" << std::endl;
		ready = true;
	});

    // register a connect failure handle
    CBStatusMessageRef connectFailureListenerHandle = unionClient.AddEventListener(Event::CONNECT_FAILURE, [&ready, &connectError](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Connection broken, " + a + "..." << std::endl;
		ready = false;
		connectError = true;
	});


	// register a disconnection handle
	CBStatusMessageRef disconnectedListenerHandle = unionClient.AddEventListener(Event::DISCONNECTED, [&disconnected, &ready](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Disconnected from server, " + a + "..." << std::endl;
		ready = false;
		disconnected = true;
	});

	// register a select connection handle
	CBStatusMessageRef selectConnectionListenerHandle = unionClient.AddEventListener(Event::SELECT_CONNECTION, [&selected](EventType t, std::string a, UPCStatus s) {
		std::cerr << "New connection selected, " + a + "..." << std::endl;
		selected = true;
	});
    //
    // STEP 3: Initiate connection to server
    // -------------------------------------
    //

	cerr << "About to Connect ..." << endl;
	unionClient.Connect();
	cerr << "Connecting ..." << endl;

    //
    // STEP 4: wait for a read or connection error event
    // -------------------------------------------------
    //
    // We need to wait for a successful connection event in this sample.
    // In a more realistic use scenario, other operations would persist,
    // and eventually the event handler would signal that the connection
    // is ready.
    //
	for (;;) {
		usleep(100);
		if (ready) {
			break;
		} else if (connectError) {
			cerr << "Connect error ... fatal .. bye!" << endl;
			return -1;
		}
	}
	cerr << "Union connection is ready ..." << endl;

    //
    // STEP 5: we're connected!
    // ------------------------
    //
    // This is where one would join lobbies, exchange messages, etc.
    // For the purpse of this sample, we're not doing anything
    //

    //
    // STEP 6: Disconnect
    // ------------------
    //

    cerr << "Disconnecting..."<<endl;
    unionClient.Disconnect();

	for (;;) {
		usleep(100);
		if (disconnected) {
			cerr << "Got disconnect .. bye!" << endl;
			break;
		} else if (connectError) {
			cerr << "Connect error ... on disconnect .. bye!" << endl;
			return -1;
		}
	}

	cerr << "Disconnected." << endl;

	//
	// Cleanup. remove handles
	//
	readyListenerHandle = nullptr;
	connectFailureListenerHandle = nullptr;
	disconnectedListenerHandle = nullptr;
	selectConnectionListenerHandle = nullptr;

	return 0;
}



void process_arg_list(int argc, char **argv) {
	int argi = 1;
	bool hasHost = false;
	bool hasService = false;
	while (argi < argc) {
		string arg(argv[argi]);
		if (arg[0] == '-') {
		} else if (!hasHost) {
			hasHost = true;
			host = arg;
		} else if (!hasService) {
			hasService = true;
			service = arg;
		}
		argi++;
	}

	if (hasHost) {
		upcConnector.SetAllConnectionHost(host);
	}
	if (hasService) {
		upcConnector.SetAllConnectionService(service);
	}

	cerr << "Union Host: " << host << ":" << service << endl;
}



