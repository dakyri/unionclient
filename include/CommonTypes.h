/*
 * CommonTypes.h
 *
 *  Created on: Apr 8, 2014
 *      Author: dak
 */

#ifndef COMMONTYPES_H_
#define COMMONTYPES_H_

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <forward_list>
#include <memory>
#include <tuple>
#include <functional>
#include <iterator>
#include <sstream>
#include <iostream>
#include <stdint.h>
//#include <thread>
#include <mutex>

class RoomManifest;
class ClientCache;
class ClientManifest;
class IFilter;
class AccountManager;
class AttributeManager;
class AttributeInitializer;
class Attribute;
class AttributeCollection;
class AttributeOwner;
class ClientManager;
class RoomManager;
class UVEventLoop;
class ConnectionMonitor;
class UnionBridge;
class DefaultLogger;
class ILogger;
class RoomSettings;
class RoomModules;
class UserAccount;
class Server;
class Room;
class AbstractConnector;
class AbstractConnection;
class Client;
class AttrMap;

typedef std::string RoomID;
typedef std::string ClientID;
typedef std::string UserID;
typedef std::string UserPasswd;
typedef std::string ModuleID;
typedef const char* UPCMessageID;
typedef std::string UserMessageID;
typedef std::string RoomQualifier;
typedef std::string Address;
typedef std::string Role;
typedef int EventType;
typedef int SyncState;
/** @typedef UPCStatus the type passed around by various callbacks. Negative values are used for low level comms errors. 0 is the success status, and positive values are various UPC generated statuses */
typedef int UPCStatus;

typedef std::string AttrName;
typedef std::string AttrVal;
typedef std::string AttrScope;

typedef std::vector<std::string> StringArgs;

typedef std::shared_ptr<const AttrMap> AttrMapRef;
typedef std::shared_ptr<const IFilter> IFilterRef;

inline std::string std_to_string(uint32_t n) {
	// FIXME to_string is broken in gcc .. this will have to do for the moment
	char buf[4*sizeof(uint32_t)];
#ifdef _MSC_VER
	sprintf_s(buf, 4 * sizeof(uint32_t), "%u", n);
#else
	snprintf(buf, 4 * sizeof(uint32_t), "%u", n);
#endif
	return std::string(buf);
}

inline std::string std_to_string(int32_t n) {
	// FIXME to_string is broken in gcc .. this will have to do for the moment
	char buf[4*sizeof(int32_t)];
#ifdef _MSC_VER
	sprintf_s(buf, 4 * sizeof(int32_t), "%d", n);
#else
	snprintf(buf, 4 * sizeof(int32_t), "%d", n);
#endif

	return std::string(buf);
}


inline std::string std_to_string(float n) {
	// FIXME to_string is broken in gcc .. this will have to do for the moment
	char buf[8*sizeof(float)];
#ifdef _MSC_VER
	sprintf_s(buf, 8 * sizeof(float), "%g", n);
#else
	snprintf(buf, 8 * sizeof(float), "%g", n);
#endif
	return std::string(buf);
}

inline std::string bool_to_string(bool b)
{
	return std::string(b?"true":"false");
}

#ifdef DEBUG_ENABLE
#define DEBUG_OUT(X) (std::cerr << X << std::endl)
#else
#define DEBUG_OUT(X)
#endif
#endif /* COMMONTYPES_H_ */
