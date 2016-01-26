//============================================================================
// Name        : UnionTest.cpp
// Author      : 
// Version     :
// Copyright   : My copyright notice
// Description : MAF in C++, Ansi-style
//============================================================================

using namespace std;
#include "UnionClient.h"
#include "connector/AbstractConnector.h"
#include "connector/CoutConnector.h"
#include "connector/StandardConnector.h"

#include <thread>
#include <chrono>

/**
 * test client that goes with the vs studio project
 */
class CoutConnector testConnector;
class StandardUVConnector upcConnector("localhost", "9100");

std::string prompt("> ");
std::string testbedName("UnionTest");
std::string roomToJoin("lobby");
std::string loginPolicy("b2lkPXJvb21uYW1lJmNhbl9lZGl0PTEmdXNlcm5hbWU9dXNlcm5hbWUmbmFtZT1QcmV6aStNb25pdG9yJnV0Yz0xNDg2NTY4MzUzLjk1JnJvb21fZW1wdHk9MQ==");
std::string loginSignature("0TSPohHP45eQPCgB6evBKL+PyaU=");

int main(int argc, char **argv) {
	std::string host;
	std::string service;

	if (argc >= 0) {
		testbedName = argv[0];
	}
	bool testConnection = false;
	bool testJoin = false;
	bool testLogin = false;
	int argi = 1;
	while (argi < argc) {
		string arg(argv[argi]);
		if (arg[0] == '-') {
			if (arg == "-c") {
				testConnection = true;
			} else if (arg == "-j") {
				testJoin = true;
			} else if (arg == "-l") {
				testLogin = true;
			} else if (arg == "-r") {
				if (argi < argc) {
					argi++;
					roomToJoin.assign(argv[argi]);
				}
			} else if (arg == "-p") {
				if (argi < argc) {
					argi++;
					loginPolicy.assign(argv[argi]);
				}
			} else if (arg == "-s") {
				if (argi < argc) {
					argi++;
					loginSignature.assign(argv[argi]);
				}
			}
		} else if (host == "") {
			host = arg;
		} else if (service == "") {
			service = arg;
		}
		argi++;
	}
	cerr << "Union Host: " << host << ":" << service << endl;
	cerr << "Room to join: " << roomToJoin << endl;
	cerr << "Login policy: " << loginPolicy << endl;
	cerr << "Login signature: " << loginSignature << endl;

	if (host != "") {
		upcConnector.SetAllConnectionHost(host);
	}
	if (service != "") {
		upcConnector.SetAllConnectionService(service);
	}
	cout << testbedName << " initialized...\n";

	UnionClient unionClient(upcConnector);
	unionClient.GetDefaultLog().SetLevel(0);
	cout << "Testing 2 ... 3 .. 4\n";

	//	unionClient.SetHeartbeatFrequency(60000);
	//	unionClient.SetConnectionTimeout(60000);
	//	unionClient.SetReadyTimeout(60000);
	// would be possible to add extra connections here, but the StandardConnector already implements everything
	//	upc.Connection(socketConnection);

	bool ready = false;
	bool connectError = false;
	bool selected = false;
	UPCStatus joinStatus = UPC::Status::UNKNOWN_ERROR;

	auto readyListenerHandle = unionClient.AddEventListener(Event::READY, [&ready](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Got a 'ready' from the server" << std::endl;
		ready = true;
	});
	auto disconnectedListenerHandle = unionClient.AddEventListener(Event::DISCONNECTED, [&ready](EventType t, std::string a, UPCStatus s) {
		std::cerr << "test: disconnected from server, " + a + "..." << std::endl;
		ready = false;
	});
	auto selectConnectionListenerHandle = unionClient.AddEventListener(Event::SELECT_CONNECTION, [&selected](EventType t, std::string a, UPCStatus s) {
		std::cerr << "test: new connection selected, " + a + "..." << std::endl;
		selected = true;
	});
	auto connectFailureListenerHandle = unionClient.AddEventListener(Event::CONNECT_FAILURE, [&ready, &connectError](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Connection broken, " + a + "..." << std::endl;
		ready = false;
		connectError = true;
	});

	cerr << "About to Connect ... \n";
	unionClient.Connect();
	cerr << "Connecting ...\n";
	for (;;) {
		if (ready) {
			break;
		} else if (connectError) {
			cerr << "Connect error ... fatal .. bye!" << endl;
			return -1;
		}
	}
	cerr << "We've got a ready ...\n";
	if (testConnection) {
		if (ready) return 0;
		else return -1;
	}

	//	auto roomJoined = unionClient.Self()->NotifyRoomInfo::AddEventListener(Event::JOIN_ROOM,
	//			[] (int, std::basic_string<char>, std::basic_string<char>, std::shared_ptr<const Room>, int) {
	//		;
	//	});

	std::cerr << "Inside the presentation loop ... I am number " << unionClient.Self()->GetClientID() << endl;
	CBClientInfoRef addMeetingOccupantCB;
	CBClientInfoRef removeMeetingOccupantCB;
	CBClientInfoRef roomLeftCB;
	CBRoomInfoRef joinRoomResultCB;
	CBClientInfoRef roomJoinedCB;
	CBAttrInfoRef attributeChangedCB;

	UPCStatus roomJoinStatus;
	bool haveRoomResult = false;
	bool haveLoginResult = false;
	joinRoomResultCB = unionClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::JOIN_RESULT, [&roomJoinStatus, &haveRoomResult, &haveLoginResult](EventType e, RoomQualifier q, RoomID roomID, RoomRef rr, UPCStatus s) {
		if (roomID == roomToJoin) {
			roomJoinStatus = s;
			haveRoomResult = true;
			if (s == UPC::Status::SUCCESS) {
				cerr << "Join room result successful!! :)" << endl;
			} else {
				cerr << "Join room result unsuccessful :( " << UPC::Status::GetStatusString(s) << endl;
			}
		} else {
			haveLoginResult = true;
		}
	});
	if (!testJoin) {
		roomJoinedCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::JOIN_ROOM,
			[&addMeetingOccupantCB, &removeMeetingOccupantCB](EventType e, ClientID clientID, ClientRef cr, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
			cerr << " joining " << roomID << endl;

			if (roomID == roomToJoin) {
				cerr << "Sending credentials.." << endl;
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
				RoomRef meetingRoom = rr;
				addMeetingOccupantCB = meetingRoom->NotifyClientInfo::AddEventListener(Event::ADD_OCCUPANT,
					[](EventType e, ClientID clientID, ClientRef, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
					cerr << roomID << " adding occupant " << clientID << endl;
				});
				removeMeetingOccupantCB = meetingRoom->NotifyClientInfo::AddEventListener(Event::REMOVE_OCCUPANT,
					[](EventType e, ClientID clientID, ClientRef, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
					cerr << roomID << " removing occupant " << clientID << endl;
				});
			}
		});

		roomLeftCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::LEAVE_ROOM,
			[](EventType e, ClientID, ClientRef, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
			cerr << " leaving " << roomID << endl;
		});

		attributeChangedCB = unionClient.Self()->NotifyAttrInfo::AddEventListener(Event::UPDATE_ATTR,
			[](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
			cerr << "attrib " << n << " -> " << v << " in scope " << s << " for client " << (c ? c->GetName() : "senki") << endl;
		});
	}
//	RoomRef theRoom = unionClient.GetRoomManager().CreateRoom("theRoom");
//	RoomRef lobby = unionClient.GetRoomManager().Request("lobby");
	RoomRef lobby = unionClient.GetRoomManager().JoinRoom("lobby");

	for (;;) {
		if (haveRoomResult) {
			break;
		}
	}
	if (testJoin) {
		if (roomJoinStatus == UPC::Status::SUCCESS)
			return 0;
		else
			return -1;
	}

	lobby->AddMessageListener("MODULE_MSG", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});
	lobby->AddMessageListener("CHAT_MESSAGE", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});
	lobby->AddMessageListener("COMMAND", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});

	std::cerr << "added message listener " << unionClient.Self()->GetClientID() << endl;

	std::string cmdIn;
	std::cout << prompt;
	std::getline(cin, cmdIn);

	cout << "Testing 3 ... 2 .. 1\n";
	return 0;
}
