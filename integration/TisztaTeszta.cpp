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
#ifndef _MSC_VER
#include <unistd.h>
#else
#include <Windows.h>
#endif

/**
 * all in one, all bells and whistles test.
 * a bit more useful than the hard wired tests
 */
class CoutConnector testConnector;
class StandardUVConnector upcConnector("localhost", "9100");

std::string prompt("> ");
std::string lobbyName("lobby");
std::string loginPolicy("b2lkPXJvb21uYW1lJmNhbl9lZGl0PTEmdXNlcm5hbWU9dXNlcm5hbWUmbmFtZT1QcmV6aStNb25pdG9yJnV0Yz0xNDg2NTY4MzUzLjk1JnJvb21fZW1wdHk9MQ==");
std::string loginSignature("0TSPohHP45eQPCgB6evBKL+PyaU=");

int main(int argc, char **argv) {
	std::string host;
	std::string service;
	int timeout = 15000;
	int retries = 15;
	int limit = 45;
	int argi = 1;
	while (argi < argc) {
		string arg(argv[argi]);
		if (arg[0] == '-') {
			if (arg == "-r") {
				if (argi < argc) {
					argi++;
					lobbyName.assign(argv[argi]);
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
			} else if (arg == "-timeout") {
				if (argi < argc) {
					argi++;
					timeout = 1000 * atoi(argv[argi]);
				}
			} else if (arg == "-retries") {
				if (argi < argc) {
					argi++;
					retries = atoi(argv[argi]);
				}
			} else if (arg == "-limit") {
				if (argi < argc) {
					argi++;
					limit = atoi(argv[argi]);
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
	cerr << "Lobby name: " << lobbyName << endl;
	cerr << "Login policy: " << loginPolicy << endl;
	cerr << "Login signature: " << loginSignature << endl;

	if (host != "") {
		upcConnector.SetAllConnectionHost(host);
	}
	if (service != "") {
		upcConnector.SetAllConnectionService(service);
	}
	cout << " initialized...\n";

	UnionClient unionClient(upcConnector);
	unionClient.GetDefaultLog().SetLevel(1);
	cout << "Testing 2 ... 3 .. 4\n";

	//	unionClient.SetHeartbeatFrequency(60000);
	//	unionClient.SetConnectionTimeout(60000);
	//	unionClient.SetReadyTimeout(60000);
	// would be possible to add extra connections here, but the StandardConnector already implements everything
	//	upc.Connection(socketConnection);
	unionClient.GetConnectionMonitor().SetAutoReconnectAttemptLimit(limit);
	unionClient.GetConnectionMonitor().SetAutoReconnectFrequency(timeout, timeout+1000, true);
	upcConnector.SetConnectionFailLimit(retries);

	bool ready = false;
	bool connectError = false;
	bool selected = false;
	int reconnect = 0;

	CBStatusMessageRef readyListenerHandle = unionClient.AddEventListener(Event::READY, [&ready,&reconnect](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Got a 'ready' from the server" << std::endl;
		if (ready) {
			reconnect++;
			std::cerr << "Is a reconnection!" << std::endl;
		} else {
			ready = true;
		}
	});
	CBStatusMessageRef disconnectedListenerHandle = unionClient.AddEventListener(Event::DISCONNECTED, [&ready](EventType t, std::string a, UPCStatus s) {
		std::cerr << "Disconnected from server, " + a + "..." << std::endl;
	});
	CBStatusMessageRef selectConnectionListenerHandle = unionClient.AddEventListener(Event::SELECT_CONNECTION, [&selected](EventType t, std::string a, UPCStatus s) {
		std::cerr << "New connection selected, " + a + "..." << std::endl;
		selected = true;
	});
	CBStatusMessageRef connectFailureListenerHandle = unionClient.AddEventListener(Event::CONNECT_FAILURE, [&ready, &connectError](EventType t, std::string a, UPCStatus s) {
		connectError = true;
		if (s == UPC::Status::NO_VALID_CONNECTION_AVAILABLE) {
			std::cerr << "All connection options exhausted :S ..." << std::endl;
			exit(-1);
		} else if (s == UPC::Status::CONNECT_TIMEOUT) {
			std::cerr << "Connection Timeout ..." << std::endl;
//			exit(-1);
		} else {
			std::cerr << a << std::endl;
		}
	});

	cerr << "About to Connect ... \n";
	unionClient.Connect();
	cerr << "Connecting ...\n";
// effectively we're doing a busy wait here. there are probably better things we could do while we are waiting ....
	for (;;) {
#ifdef _MSC_VER
		Sleep(10);
#else
		usleep(10000);
#endif
		if (ready) {
			break;
		} else if (connectError) {
			cerr << "Connect error ... fatal .. bye!" << endl;
			return -1;
		}
	}
	cerr << "Union connection is ready ...\n";

	std::cerr << "Inside presentation loop ... I am number " << unionClient.Self()->GetClientID() << endl;
	CBClientInfoRef addMeetingOccupantCB;
	CBClientInfoRef removeMeetingOccupantCB;
	RoomRef meetingRoom;

	UPCStatus roomJoinStatus;
	bool haveJoinStatus = false;
	bool loginAttempt = false;
	bool loginFail = false;
	bool loginSucceed = false;
	CBRoomInfoRef joinRoomResultCB = unionClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::JOIN_RESULT, [&roomJoinStatus, &haveJoinStatus, &loginAttempt, &loginFail](EventType e, RoomQualifier q, RoomID roomID, RoomRef rr, UPCStatus s) {
		if (s == UPC::Status::SUCCESS) {
			cerr << "Join room " <<roomID << " result successful!! :)" << endl;
		} else {
			cerr << "Join room " << roomID <<" result unsuccessful :( " << UPC::Status::GetStatusString(s) << endl;
		}
		if (roomID == lobbyName) {
			roomJoinStatus = s;
			haveJoinStatus = true;
			if (loginAttempt) {
				loginFail = true;
				loginAttempt = false;
			}
		}
	});
	CBClientInfoRef roomJoinedCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::JOIN_ROOM,
		[&addMeetingOccupantCB, &removeMeetingOccupantCB,&meetingRoom,&loginAttempt,&loginSucceed](EventType e, ClientID clientID, ClientRef cr, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
		cerr << " joining " << roomID << endl;

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

	CBClientInfoRef roomLeftCB = unionClient.Self()->NotifyClientInfo::AddEventListener(Event::LEAVE_ROOM,
		[](EventType e, ClientID, ClientRef, int, RoomID roomID, RoomRef  rr, UPCStatus s) {
		cerr << "Leaving " << roomID << endl;
	});

	CBAttrInfoRef attributeChangedCB = unionClient.Self()->NotifyAttrInfo::AddEventListener(Event::UPDATE_ATTR,
		[](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
		cerr << "attrib " << n << " -> " << v << " in scope " << s << " for client " << (c ? c->GetName() : "senki") << endl;
	});
	RoomRef lobby = unionClient.GetRoomManager().JoinRoom(lobbyName);

	CBModMsgRef moduleMessageCB = lobby->AddMessageListener("MODULE_MSG", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});
	CBModMsgRef chatMessageCB = lobby->AddMessageListener("CHAT_MESSAGE", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});
	CBModMsgRef commandMessageCB = lobby->AddMessageListener("COMMAND", [] (UserMessageID mid,/* RoomID room, */StringArgs msg) {
		std::cerr << std::endl;
		for (auto it: msg) {
			std::cerr << it << std::endl;
		}
		std::cerr << std::endl;
	});

	std::string cmdIn;
	std::cout << prompt;

	for (;;) {
#ifdef _MSC_VER
		Sleep(10);
#else
		usleep(10000);
#endif
		if (reconnect > 2) {
			break;
		}
	}
	unionClient.Disconnect();
	cout << "Hasta manana\n";
	return 0;
}
