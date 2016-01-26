/*
 * ConnectionState.h
 *
 *  Created on: Apr 23, 2014
 *      Author: dak
 */

#ifndef CONNECTIONSTATE_H_
#define CONNECTIONSTATE_H_

/**
 * @class ConnectionState ConnectionState.h
 * wrapper class for connection state constants used for layers, connectors, and connections in the io layer, and for the general state of UPC comms in the main client lib
 */
class ConnectionState {
public:
	static const int UNKNOWN = -1;
	static const int NOT_CONNECTED = 0;
	static const int READY  = 1;
	static const int CONNECTION_IN_PROGRESS = 2;
	static const int DISCONNECTION_IN_PROGRESS = 3;
	static const int LOGGED_IN = 4;
};

#endif /* CONNECTIONSTATE_H_ */
