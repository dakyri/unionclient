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
	ConnectingConnector() {}
	virtual ~ConnectingConnector() {}

	virtual bool IsReady() const{
		return true;
	}
	virtual int Connect(){
		NotifyListeners(Event::BEGIN_CONNECT, nullptr, "", UPC::Status::SUCCESS);
		NotifyListeners(Event::CONNECTED, nullptr, "", UPC::Status::SUCCESS);
		return 0;
	}

	virtual int Disconnect(){
		NotifyListeners(Event::DISCONNECTED, nullptr, "", UPC::Status::SUCCESS);
		return 0;
	}

	virtual int Send(const std::string msg){ return 0; }
};


class HelloConnector: public ConnectingConnector {
public:
	HelloConnector() {}
	virtual ~HelloConnector() {}

	virtual bool IsReady() const{
		return false;
	}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u66</M><L><A><![CDATA[Union Server 2.1.0 (build 590)]]></A><A>1505998205ea8eab26-e9d5-4b3e-8d71-be6527b885b6</A><A>1.10.3</A><A>true</A><A></A><A>0</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};


class MalformedXMLConnector: public ConnectingConnector {
public:
	MalformedXMLConnector() {}
	virtual ~MalformedXMLConnector() {}

	virtual bool IsReady() const{
		return false;
	}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u29</M><L><A>3</A></L></U><MALFORMED_XML></MALFORMED XML>", UPC::Status::SUCCESS);
		return 0;
	}
};


class ClientMetadataConnector: public ConnectingConnector {
public:
	ClientMetadataConnector() {}
	virtual ~ClientMetadataConnector() {}

	virtual bool IsReady() const{
		return false;
	}

	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u29</M><L><A>3</A></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};

class ReadyConnector: public ConnectingConnector {
public:
	ReadyConnector() {}
	virtual ~ReadyConnector() {}

	virtual bool IsReady() const{
		return true;
	}
	virtual int Send(const std::string msg){
		NotifyListeners(Event::RECEIVE_DATA, nullptr, "<U><M>u63</M><L></L></U>", UPC::Status::SUCCESS);
		return 0;
	}
};


class FailingLowLevelConnector: public TestConnector {
public:
	FailingLowLevelConnector() {}
	virtual ~FailingLowLevelConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Low level IO Error]", -1);
		return 0;
	}
};

class FailingRefusedConnector: public TestConnector {
public:
	FailingRefusedConnector() {}
	virtual ~FailingRefusedConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Because]", UPC::Status::CONNECT_REFUSED);
		return 0;
	}
};

class FailingIncompatibleConnector: public TestConnector {
public:
	FailingIncompatibleConnector() {}
	virtual ~FailingIncompatibleConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Version]", UPC::Status::PROTOCOL_INCOMPATIBLE);
		return 0;
	}
};

class FailingTerminatedConnector: public TestConnector {
public:
	FailingTerminatedConnector() {}
	virtual ~FailingTerminatedConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Got terminated]", UPC::Status::SESSION_TERMINATED);
		return 0;
	}
};

class FailingNotFoundConnector: public TestConnector {
public:
	FailingNotFoundConnector() {}
	virtual ~FailingNotFoundConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Lost]", UPC::Status::SESSION_NOT_FOUND);
		return 0;
	}
};

class FailingServerKillConnector: public TestConnector {
public:
	FailingServerKillConnector() {}
	virtual ~FailingServerKillConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Killed]", UPC::Status::SERVER_KILL_CONNECT);
		return 0;
	}
};

class FailingClientKillConnector: public TestConnector {
public:
	FailingClientKillConnector() {}
	virtual ~FailingClientKillConnector() {}

	virtual bool IsReady() const{
		return false;
	}
	virtual int Connect(){
		NotifyListeners(Event::CONNECT_FAILURE, nullptr, "[Killed]", UPC::Status::CLIENT_KILL_CONNECT);
		return 0;
	}
};

class ConnectionTest: public ::testing::Test {
public:
	virtual void setUp() {
	}
	HelloConnector helloConnector;
	MalformedXMLConnector malformedXMLConnector;
	ClientMetadataConnector metadataConnector;
	ReadyConnector readyConnector;
	FailingLowLevelConnector failingLowLevelConnector;
	FailingRefusedConnector failingRefusedConnector;
	FailingIncompatibleConnector failingIncompatibleConnector;
	FailingTerminatedConnector failingTerminatedConnector;
	FailingNotFoundConnector failingNotFoundConnector;
	FailingServerKillConnector failingServerKillConnector;
	FailingClientKillConnector failingClientKillConnector;

	UnionClient helloClient{helloConnector};
	UnionClient malformedXMLClient{malformedXMLConnector};
	UnionClient metadataClient{metadataConnector};
	UnionClient readyClient{readyConnector};
	UnionClient failingLowLevelClient{failingLowLevelConnector};
	UnionClient failingRefusedClient{failingRefusedConnector};
	UnionClient failingIncompatibleClient{failingIncompatibleConnector};
	UnionClient failingTerminatedClient{failingTerminatedConnector};
	UnionClient failingNotFoundClient{failingNotFoundConnector};
	UnionClient failingServerKillClient{failingServerKillConnector};
	UnionClient failingClientKillClient{failingClientKillConnector};
};

TEST_F(ConnectionTest, BeginsConnectionOK) {
	bool cnx=false;
	auto cnxListenerHandle = helloClient.AddEventListener(Event::BEGIN_CONNECT, [&cnx] (EventType t, std::string a, UPCStatus s) {
		cnx = true;
	});
	helloClient.Connect();
	helloClient.DisableHeartbeat();
	EXPECT_TRUE(cnx);
}

TEST_F(ConnectionTest, MakesConnectionOK) {
	bool cnx=false;
	auto cnxListenerHandle = helloClient.AddEventListener(Event::CONNECTED, [&cnx] (EventType t, std::string a, UPCStatus s) {
		cnx = true;
	});
	helloClient.Connect();
	helloClient.DisableHeartbeat();
	EXPECT_TRUE(cnx);
}

TEST_F(ConnectionTest, MakesDisconnectionOK) {
	bool dnx=false;
	auto dnxListenerHandle = helloClient.AddEventListener(Event::DISCONNECTED, [&dnx] (EventType t, std::string a, UPCStatus s) {
		dnx = true;
	});
	helloClient.Disconnect();
	helloClient.DisableHeartbeat();
	EXPECT_TRUE(dnx);
}

TEST_F(ConnectionTest, SignalsHelloOK) {
	helloConnector.Send("<U><M>u65</M><L><A>Custom Client</A><A>Desktop testbed;2.1.1 (Build 302)</A><A>1.10.3.0</A></L></U>");
	Version v = Version::FromVersionString("1.10.3");
	bool tv = (v == helloClient.GetServerUPCVersion());
	helloClient.DisableHeartbeat();
	EXPECT_TRUE(tv);
}

TEST_F(ConnectionTest, HandlesXMLErrorOK) {
	bool err=false;
	UPCStatus status;
	auto dnxListenerHandle = malformedXMLClient.AddEventListener(Event::IO_ERROR, [&err, &status] (EventType t, std::string a, UPCStatus s) {
		err = true;
		status = s;
	});
	malformedXMLConnector.Send("<U><M>u65</M><L><A>Custom Client</A><A>Desktop testbed;2.1.1 (Build 302)</A><A>1.10.3.0</A></L></U>");
	malformedXMLClient.DisableHeartbeat();
	EXPECT_TRUE(err);
	EXPECT_EQ(status, UPC::Status::XML_ERROR);
}

TEST_F(ConnectionTest, SignalsClientMetadataOK) {
	metadataConnector.Send("<U><M>u65</M><L><A>Custom Client</A><A>Desktop testbed;2.1.1 (Build 302)</A><A>1.10.3.0</A></L></U>");
	metadataClient.DisableHeartbeat();
	EXPECT_EQ("3", metadataClient.Self()->GetClientID());
}

TEST_F(ConnectionTest, SignalsReadyOK) {
	bool ready=false;
	auto readyListenerHandle = readyClient.AddEventListener(Event::READY, [&ready] (EventType t, std::string a, UPCStatus s) {
		ready = true;
	});
	readyConnector.Send("<U><M>u65</M><L><A>Custom Client</A><A>Desktop testbed;2.1.1 (Build 302)</A><A>1.10.3.0</A></L></U>");
	readyClient.DisableHeartbeat();
	EXPECT_TRUE(ready);
}

TEST_F(ConnectionTest, FailsConnectionLowLevel) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	auto cnxListenerHandle = failingLowLevelClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingLowLevelClient.Connect();
	failingLowLevelClient.DisableHeartbeat();
	EXPECT_EQ(-1, failStatus);
	EXPECT_TRUE(cnxFail);
}


TEST_F(ConnectionTest, FailsRefused) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingRefusedClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingRefusedClient.Connect();
	failingRefusedClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::CONNECT_REFUSED, failStatus);
	EXPECT_TRUE(cnxFail);
}


TEST_F(ConnectionTest, FailsIncompatible) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingIncompatibleClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingIncompatibleClient.Connect();
	failingIncompatibleClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::PROTOCOL_INCOMPATIBLE, failStatus);
	EXPECT_TRUE(cnxFail);
}

TEST_F(ConnectionTest, FailsTerminated) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingTerminatedClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingTerminatedClient.Connect();
	failingTerminatedClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::SESSION_TERMINATED, failStatus);
	EXPECT_TRUE(cnxFail);
}

TEST_F(ConnectionTest, FailsNotFound) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingNotFoundClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingNotFoundClient.Connect();
	failingNotFoundClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::SESSION_NOT_FOUND, failStatus);
	EXPECT_TRUE(cnxFail);
}

TEST_F(ConnectionTest, FailsServerKill) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingServerKillClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingServerKillClient.Connect();
	failingServerKillClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::SERVER_KILL_CONNECT, failStatus);
	EXPECT_TRUE(cnxFail);
}

TEST_F(ConnectionTest, FailsClientKill) {
	bool cnxFail=false;
	UPCStatus failStatus=0;
	std::string failMessage;
	auto cnxListenerHandle = failingClientKillClient.AddEventListener(Event::CONNECT_FAILURE, [&cnxFail, &failStatus] (EventType t, std::string a, UPCStatus s) {
		cnxFail = true;
		failStatus = s;
	});
	failingClientKillClient.Connect();
	failingClientKillClient.DisableHeartbeat();
	EXPECT_EQ(UPC::Status::CLIENT_KILL_CONNECT, failStatus);
	EXPECT_TRUE(cnxFail);
}
