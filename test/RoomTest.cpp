#include <cstdlib>
#include <gtest/gtest.h>

#include "UnionClient.h"

class TestConnector: public AbstractConnector {
public:
	TestConnector() {}
	virtual ~TestConnector() {}

	virtual bool IsReady() const{ return false; }
	virtual int Connect() { return 0; }
	virtual int Disconnect(){ return 0; }
	virtual int Send(const std::string msg){ return 0; }

	virtual void SetConnectionAttributes(ConnectionAttributes) {};
	virtual void SetActiveConnectionSessionID(std::string) {};
	virtual void SetConnectionAffinity(std::string host, int durationSec) {};
};


class ConnectingConnector: public TestConnector {
public:
	ConnectingConnector() { }
	virtual ~ConnectingConnector() {}

	virtual bool IsReady() const{
		return true;
	}
	virtual int Connect(){
		NotifyListeners(Event::BEGIN_CONNECT, nullptr, "", UPC::Status::SUCCESS);
		NotifyListeners(Event::CONNECTED, nullptr, "", UPC::Status::SUCCESS);
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u29</M><L><A>3</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}

	virtual int Disconnect(){
		NotifyListeners(Event::DISCONNECTED, nullptr, "", UPC::Status::SUCCESS);
		return 0;
	}

	virtual int Send(const std::string msg){ return 0; }

	// public calls to trick the client into a correct states for tests that follow
	void NotifyCreatedTestRoom(std::string s) {
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u32</M><L><A><![CDATA[theRoom]]></A><A>"+s+"</A></L></U>", UPC::Status::SUCCESS);
	}
	void NotifyJoinedTestRoom() {
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u6</M><L><A><![CDATA[theRoom]]></A></L></U>", UPC::Status::SUCCESS);
	}
	void NotifyTestOccupantAdded() {
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u36</M><L><A><![CDATA[theRoom]]></A><A><![CDATA[6]]></A><A><![CDATA[senki]]></A><A></A><A></A></L></U>", UPC::Status::SUCCESS);
	}
	void NotifyTestOccupantRemoved() {
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u37</M><L><A><![CDATA[theRoom]]></A><A><![CDATA[6]]></A></L></U>", UPC::Status::SUCCESS);
	}

	bool triggerNotifyOnSend=false;
};


class CreateRoomOKConnector: public ConnectingConnector {
public:
	CreateRoomOKConnector() {}
	virtual ~CreateRoomOKConnector() {}

	virtual int Send(const std::string msg){
		NotifyCreatedTestRoom("SUCCESS");
		return 0;
	}
};

class CreateRoomFailConnector: public ConnectingConnector {
public:
	CreateRoomFailConnector() {}
	virtual ~CreateRoomFailConnector() {}

	virtual int Send(const std::string msg){
		NotifyCreatedTestRoom("PERMISSION_DENIED");
		return 0;
	}
};


class RemoveRoomOKConnector: public ConnectingConnector {
public:
	RemoveRoomOKConnector() {}
	virtual ~RemoveRoomOKConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u33</M><L><A><![CDATA[theRoom]]></A><A>SUCCESS</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class RemoveRoomFailConnector: public ConnectingConnector {
public:
	RemoveRoomFailConnector() {}
	virtual ~RemoveRoomFailConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u33</M><L><A><![CDATA[theRoom]]></A><A>PERMISSION_DENIED</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class JoinRoomOKConnector: public ConnectingConnector {
public:
	JoinRoomOKConnector() {}
	virtual ~JoinRoomOKConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u72</M><L><A><![CDATA[theRoom]]></A><A>SUCCESS</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class JoinRoomFailConnector: public ConnectingConnector {
public:
	JoinRoomFailConnector() {}
	virtual ~JoinRoomFailConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u72</M><L><A><![CDATA[theRoom]]></A><A>PERMISSION_DENIED</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class LeaveRoomOKConnector: public ConnectingConnector {
public:
	LeaveRoomOKConnector() {}
	virtual ~LeaveRoomOKConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u76</M><L><A><![CDATA[theRoom]]></A><A>SUCCESS</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class LeaveRoomFailConnector: public ConnectingConnector {
public:
	LeaveRoomFailConnector() {}
	virtual ~LeaveRoomFailConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u76</M><L><A><![CDATA[theRoom]]></A><A>ERROR</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class AddOccupantConnector: public ConnectingConnector {
public:
	AddOccupantConnector() {}
	virtual ~AddOccupantConnector() {}

	virtual int Send(const std::string msg){
		if (triggerNotifyOnSend) NotifyTestOccupantAdded();
		return 0;
	}
};

class RemoveOccupantConnector: public ConnectingConnector {
public:
	RemoveOccupantConnector() {}
	virtual ~RemoveOccupantConnector() {}

	virtual int Send(const std::string msg){
		if (triggerNotifyOnSend) NotifyTestOccupantRemoved();
		return 0;
	}
};

class RoomCountConnector: public ConnectingConnector {
public:
	RoomCountConnector() {}
	virtual ~RoomCountConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u131</M><L><A><![CDATA[theRoom]]></A><A>2718</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class SnapshotConnector: public ConnectingConnector {
public:
	SnapshotConnector() {}
	virtual ~SnapshotConnector() {}

	virtual int Send(const std::string msg) {
		NotifyListeners(Event::RECEIVE_DATA, nullptr,
				"<U><M>u54</M><L><A></A><A><![CDATA[lobby]]></A><A>1</A><A>0</A><A><![CDATA[_MAX_CLIENTS|-1]]></A><A>14</A><A></A><A>0</A>"
				"<A><![CDATA[_ROLES|0|68|_CT|1401978171488|36]]></A><A><![CDATA[]]></A></L></U>",
		UPC::Status::SUCCESS);
		return 0;
	}
};

class MessageConnector: public ConnectingConnector {
public:
	MessageConnector() {}
	virtual ~MessageConnector() {}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u7</M><L><A>MODULE_MSG</A><A>2</A><A></A><A></A><A><![CDATA[synced]]></A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class RoomTest: public ::testing::Test {
public:
	virtual void setUp() {
		createOkClient.DisableHeartbeat();
		createFailClient.DisableHeartbeat();
		removeOkClient.DisableHeartbeat();
		removeFailClient.DisableHeartbeat();
		joinOkClient.DisableHeartbeat();
		joinFailClient.DisableHeartbeat();
		leaveOkClient.DisableHeartbeat();
		leaveFailClient.DisableHeartbeat();
		addOccupantClient.DisableHeartbeat();
		removeOccupantClient.DisableHeartbeat();
		roomCountClient.DisableHeartbeat();
		snapshotClient.DisableHeartbeat();
		messageClient.DisableHeartbeat();
	}
	CreateRoomOKConnector createOkConnector;
	CreateRoomFailConnector createFailConnector;
	RemoveRoomOKConnector removeOkConnector;
	RemoveRoomFailConnector removeFailConnector;
	JoinRoomOKConnector joinOkConnector;
	JoinRoomFailConnector joinFailConnector;
	LeaveRoomOKConnector leaveOkConnector;
	LeaveRoomFailConnector leaveFailConnector;
	AddOccupantConnector addOccupantConnector;
	RemoveOccupantConnector removeOccupantConnector;
	RoomCountConnector roomCountConnector;
	SnapshotConnector snapshotConnector;
	MessageConnector messageConnector;

	UnionClient createOkClient{createOkConnector};
	UnionClient createFailClient{createFailConnector};
	UnionClient removeOkClient{removeOkConnector};
	UnionClient removeFailClient{removeFailConnector};
	UnionClient joinOkClient{joinOkConnector};
	UnionClient joinFailClient{joinFailConnector};
	UnionClient leaveOkClient{leaveOkConnector};
	UnionClient leaveFailClient{leaveFailConnector};
	UnionClient addOccupantClient{addOccupantConnector};
	UnionClient removeOccupantClient{removeOccupantConnector};
	UnionClient roomCountClient{roomCountConnector};
	UnionClient snapshotClient{snapshotConnector};
	UnionClient messageClient{messageConnector};
};

TEST_F(RoomTest, CreateRoomOK) {
	RoomID roomId;
	UPCStatus status;
	CBRoomInfoRef r = createOkClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::CREATE_ROOM_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	createOkClient.GetRoomManager().CreateRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, CreateRoomFail) {
	RoomID roomId;
	UPCStatus status;
	CBRoomInfoRef r = createFailClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::CREATE_ROOM_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	createFailClient.GetRoomManager().CreateRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::PERMISSION_DENIED);
}


TEST_F(RoomTest, RemoveRoomOK) {
	RoomID roomId;
	UPCStatus status;
	CBRoomInfoRef r = removeOkClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::REMOVE_ROOM_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	removeOkClient.GetRoomManager().RemoveRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, RemoveRoomFail) {
	RoomID roomId;
	UPCStatus status;
	CBRoomInfoRef r = removeFailClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::REMOVE_ROOM_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	removeFailClient.GetRoomManager().RemoveRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::PERMISSION_DENIED);
}


TEST_F(RoomTest, JoinRoomOK) {
	RoomID roomId;
	UPCStatus status;
	joinOkClient.GetRoomManager().CreateRoom("theRoom");// this will generate a notification through the Send();... create room first
	CBRoomInfoRef r = joinOkClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::JOIN_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	joinOkClient.GetRoomManager().JoinRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, JoinRoomFail) {
	RoomID roomId;
	UPCStatus status;
	joinFailClient.GetRoomManager().CreateRoom("theRoom");// this will generate a notification through the Send();... create room first
	CBRoomInfoRef r = joinFailClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::JOIN_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	joinFailClient.GetRoomManager().RemoveRoom("theRoom");
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::PERMISSION_DENIED);
}



TEST_F(RoomTest, LeaveRoomOK) {
	RoomID roomId;
	UPCStatus status;
	RoomRef r=leaveOkClient.GetRoomManager().CreateRoom("theRoom");// this will generate a notification through the Send();... create room first
	leaveOkConnector.NotifyJoinedTestRoom(); // hack to set up a state which we can leave from
	CBRoomInfoRef cr = leaveOkClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::LEAVE_RESULT, [&status, &roomId](EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	r->Leave();
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, LeaveRoomFail) {
	RoomID roomId;
	UPCStatus status;
	RoomRef r=leaveFailClient.GetRoomManager().CreateRoom("theRoom");// this will generate a notification through the Send();... create room first
	leaveFailConnector.NotifyJoinedTestRoom(); // hack to set up a state which we can leave from
	CBRoomInfoRef cr = leaveFailClient.GetRoomManager().NotifyRoomInfo::AddEventListener(Event::LEAVE_RESULT, [&status, &roomId] (EventType t, RoomQualifier q, RoomID id, RoomRef r, UPCStatus s) {
		status = s;
		roomId = id;
	});
	r->Leave();
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(status, UPC::Status::UPC_ERROR);
}


TEST_F(RoomTest, AddOccupantOK) {
	RoomID roomId;
	ClientID clientId;
	UPCStatus status;
	addOccupantConnector.triggerNotifyOnSend = false;
	RoomRef r = addOccupantClient.GetRoomManager().CreateRoom("theRoom"); // this will generate a notification through the Send();... create room first
	CBClientInfoRef cr = r->NotifyClientInfo::AddEventListener(Event::ADD_OCCUPANT, [&roomId, &clientId, &status](EventType e, ClientID cid, ClientRef cr, int, RoomID rid, RoomRef  rr, UPCStatus s) {
		roomId = rid;
		clientId = cid;
		status = s;
	});
	addOccupantConnector.triggerNotifyOnSend = true;
	r->Join();
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(clientId, "6");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, RemoveOccupantOK) {
	RoomID roomId;
	ClientID clientId;
	UPCStatus status;
	removeOccupantConnector.triggerNotifyOnSend = false;
	RoomRef r = removeOccupantClient.GetRoomManager().CreateRoom("theRoom"); // this will generate a notification through the Send();... create room first
	removeOccupantConnector.NotifyTestOccupantAdded();
	removeOccupantConnector.NotifyJoinedTestRoom();
	CBClientInfoRef cr = r->NotifyClientInfo::AddEventListener(Event::REMOVE_OCCUPANT, [&roomId, &clientId, &status](EventType e, ClientID cid, ClientRef cr, int, RoomID rid, RoomRef  rr, UPCStatus s) {
		roomId = rid;
		clientId = cid;
		status = s;
	});
	removeOccupantConnector.triggerNotifyOnSend = true;
	r->Leave();
	EXPECT_EQ(roomId, "theRoom");
	EXPECT_EQ(clientId, "6");
	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, OccupantCountOK) {
	RoomRef r = roomCountClient.GetRoomManager().CreateRoom("theRoom"); // this will generate a notification through the Send();... create room first
	roomCountClient.GetRoomManager().JoinRoom("theRoom");
//	EXPECT_EQ(r->GetNumOccupants(), 2718);
}

TEST_F(RoomTest, DeserializeOK) {
	RoomRef r = snapshotClient.GetRoomManager().CreateRoom("theRoom"); // this will generate a notification through the Send();... create room first
	snapshotClient.GetRoomManager().JoinRoom("theRoom");
//	EXPECT_EQ(roomId, "theRoom");
//	EXPECT_EQ(clientId, "6");
//	EXPECT_EQ(status, UPC::Status::SUCCESS);
}

TEST_F(RoomTest, MessageOK) {
	RoomRef r = messageClient.GetRoomManager().CreateRoom("theRoom"); // this will generate a notification through the Send();... create room first
	messageClient.GetRoomManager().JoinRoom("theRoom");
//	EXPECT_EQ(roomId, "theRoom");
//	EXPECT_EQ(clientId, "6");
//	EXPECT_EQ(status, UPC::Status::SUCCESS);
}
