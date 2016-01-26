//============================================================================
// Name		: TrackMessageSuccess.cpp
// Author	  :
// Version	 :
// Copyright   : My copyright notice
// Description : MAF in C++, Ansi-style
//============================================================================


//============================================================================
//
//  This sample shows a simple successful connection to a union server,
//  and and a login and some tracking on the room
//
//  The sample constis of the following steps:
//
//  STEP 1: Create unionClient
//  STEP 2: register event listeners
//  STEP 3: Initiate connection to server
//  STEP 4: wait for a ready or connection error event
//  STEP 5: we're connected!
//  STEP 6: join the lobby and login
//  STEP 7: Attribute listeners
//  STEP 8: Disconnect
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

std::string lobbyName("lobby");
std::string loginPolicy("b2lkPXJvb21uYW1lJmNhbl9lZGl0PTEmdXNlcm5hbWU9dXNlcm5hbWUmbmFtZT1QcmV6aStNb25pdG9yJnV0Yz0xNDg2NTY4MzUzLjk1JnJvb21fZW1wdHk9MQ==");
std::string loginSignature("0TSPohHP45eQPCgB6evBKL+PyaU=");

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
	// STEP 5: join the room
	// ------------------------
	//
	// This is where one we join teh lobby. The actual join is below, and login is done within
	// the join loop on the second successful join
	//

	RoomRef meetingRoom;
	UPCStatus lobbyJoinStatus = UPC::Status::UNKNOWN_ERROR;
	bool haveLobbyJoinStatus = false;
	bool loginAttempt = false;
	bool loginSucceed = false;
	bool policyExpired = false;

	//
	// STEP 6: login
	// ------------------------
	//
	// This is where one we login to a presentation. We're doing this
	// automatically on successful join of the lobby
	//
	CBModMsgRef roomModuleMessageCB;
	CBModMsgRef roomChatMessageCB;
	CBModMsgRef roomCommandMessageCB;

	CBClientInfoRef roomJoinedCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::JOIN_ROOM,
		[&meetingRoom,&loginAttempt,&loginSucceed,&roomModuleMessageCB,&roomChatMessageCB,&roomCommandMessageCB](EventType e, ClientID clientID, ClientRef cr, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
		cerr << "Joining " << roomID << endl;

		if (roomID == lobbyName) {
			cerr << "Sending credentials.." << endl;
			loginAttempt = true;
			rr->SendModuleMessage("login",
			{
				{ "policy", loginPolicy },
				{ "signature", loginSignature },
				{ "currentVersion", "0" },
				{ "reconnecting", "false" },
				{ "newReconnectFlow", "true" }
			});
		} else {
			cerr << "Authentication successful!" << endl;
			meetingRoom = rr;
			loginSucceed = true;
			//
			// STEP 7: message listeners
			// ------------------------
			//
			// After we log in to the lobby, and then into the prezentation, we add attribute listeners
			//

			cerr << "Adding meeting room message listeners" << endl;
			roomModuleMessageCB = meetingRoom->AddMessageListener("MODULE_MSG", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
				std::cerr << std::endl;
				for (auto it: msg) {
					std::cerr << it << std::endl;
				}
				std::cerr << std::endl;
			});
			roomChatMessageCB = meetingRoom->AddMessageListener("CHAT_MESSAGE", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
				std::cerr << std::endl;
				for (auto it: msg) {
					std::cerr << it << std::endl;
				}
				std::cerr << std::endl;
			});
			roomCommandMessageCB = meetingRoom->AddMessageListener("COMMAND", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
				std::cerr << std::endl;
				for (auto it: msg) {
					std::cerr << it << std::endl;
				}
				std::cerr << std::endl;
			});
		}
	});

	// STEP 6.1 other listeners needed for correct login procedure
	//
	// add attribute listeners for the join results and leave results in case login errors are caused by incorrect rooms
	//
	//
	CBRoomInfoRef joinRoomResultCB = unionClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::JOIN_RESULT, [&lobbyJoinStatus, &haveLobbyJoinStatus](EventType e, RoomQualifier q, RoomID roomID, RoomRef rr, UPCStatus s) {
		if (s == UPC::Status::SUCCESS) {
			cerr << "Join room " << roomID << " result successful!! :)" << endl;
		} else {
			cerr << "Join room " << roomID << " result unsuccessful :( " << UPC::Status::GetStatusString(s) << endl;
		}
		if (roomID == lobbyName) {
			if (lobbyJoinStatus == UPC::Status::UNKNOWN_ERROR) lobbyJoinStatus = s;
			haveLobbyJoinStatus = true;
		}
	});

	CBClientInfoRef roomLeftCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::LEAVE_ROOM,
		[&loginAttempt,&loginSucceed](EventType e, ClientID, ClientRef, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
		if (s == UPC::Status::SUCCESS) {
			cerr << "Leave room " << roomID << " result successful!! :)" << endl;
		} else {
			cerr << "Leave room " << roomID << " result unsuccessful :( " << UPC::Status::GetStatusString(s) << endl;
		}
	});

	//
	// add a MODULE_MESSAGE listener to check for bad policies
	//
	//
	CBModMsgRef moduleMessageCB = unionClient.Self()->AddMessageListener("MODULE_MSG", [&policyExpired] (UserMessageID mid, StringArgs msg) {
		cerr << "Received a MODULE_MSG message for the client library" << endl;
		cerr << " >>";
		for (auto it: msg) {
			cerr << it << "|";
			if (it == "policy-expired") {
				policyExpired = true;
			}
		}
		cerr << endl;
	});

	//
	//
	// here is where we actually join
	//
	//
	RoomRef lobby = unionClient.GetRoomManager().JoinRoom(lobbyName);

	//
	// for the moment we are going to just sit here until all the login stuff is done
	//
	float timeWaiting = 0;
	for (;;) {
		usleep(10000);
		timeWaiting+=0.01;
		if (loginSucceed) {
			break;
		} else if (haveLobbyJoinStatus && lobbyJoinStatus != UPC::Status::SUCCESS) {
			cerr << "Failed to join to lobby " << lobbyName << " correctly!" << endl;
			unionClient.Disconnect();
			return -1;
		} else if (policyExpired) {
			cerr << "Login Policy expired, exitting!" << endl;
			unionClient.Disconnect();
			return -1;
		} else if (timeWaiting > loginTimeout) {
			cerr << "Login timeout, presuming failure" << endl;
			unionClient.Disconnect();
			return -1;
		} else if (connectError) {
			cerr << "Connect error ... fatal .. bye!" << endl;
			unionClient.Disconnect();
			return -1;
		}
	}

	// STEP 7.2 Cleanup
	// ----------------
	// remove event handlers

	moduleMessageCB = nullptr;
	roomLeftCB = nullptr;
	joinRoomResultCB = nullptr;
	roomJoinedCB = nullptr;
	roomModuleMessageCB = nullptr;
	roomCommandMessageCB = nullptr;
	roomChatMessageCB = nullptr;

	//
	// STEP 8: Disconnect
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

	return loginSucceed;
}




void process_arg_list(int argc, char **argv) {
	int argi = 1;
	bool hasHost = false;
	bool hasService = false;
	while (argi < argc) {
		string arg(argv[argi]);
		if (arg[0] == '-') {
			if (arg == "-r") {
				if (argi < argc-1) {
					argi++;
					lobbyName.assign(argv[argi]);
				}
			} else if (arg == "-t") {
				if (argi < argc-1) {
					argi++;
					loginTimeout = atof(argv[argi]);
				}
			} else if (arg == "-p") {
				if (argi < argc-1) {
					argi++;
					loginPolicy.assign(argv[argi]);
				}
			} else if (arg == "-s") {
				if (argi < argc-1) {
					argi++;
					loginSignature.assign(argv[argi]);
				}
			}
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
	cerr << "Lobby name: " << lobbyName << endl;
	cerr << "Login policy: " << loginPolicy << endl;
	cerr << "Login signature: " << loginSignature << endl;
}




