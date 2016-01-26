/*
 * MessageNotifier.h
 *
 *  Created on: May 19, 2014
 *      Author: dak
 */

#ifndef MESSAGENOTIFIER_H_
#define MESSAGENOTIFIER_H_

#include "CommonTypes.h"

class RxMsgBroadcastType
{
public:
	static const char*TO_SERVER;
	static const char*TO_ROOMS;
	static const char*TO_CLIENTS;
};

typedef std::function<void(UserMessageID, StringArgs)> CBModMsg;
typedef std::shared_ptr<CBModMsg> CBModMsgRef;

struct MsgCBR
{
	MsgCBR(UserMessageID message, std::vector<ClientID> clients, std::vector<RoomID> rooms, CBModMsgRef cb)
		: message(message)
		, rooms(rooms)
		, cb(cb) {	}
	UserMessageID message;
	std::vector<RoomID> rooms;
	std::vector<ClientID> clients;
	std::weak_ptr<CBModMsg> cb;
};

class MessageNotifier {
public:
	MessageNotifier();
	virtual ~MessageNotifier();

	CBModMsgRef AddMessageListener(const UserMessageID message, const CBModMsg listener) const;
	CBModMsgRef AddMessageListener(const UserMessageID message, const std::vector<ClientID> clients, const std::vector<RoomID> rooms, const CBModMsg listener) const;
	void RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const;
	bool HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const;

protected:
	void NotifyMessageListeners(const UserMessageID message, const std::vector<ClientID> clients, const std::vector<RoomID> rooms, const StringArgs msg) const;

	std::mutex mutable lock;
	std::unordered_map<UserMessageID, std::vector<MsgCBR>> mutable msgListeners;
};

#endif /* MESSAGENOTIFIER_H_ */
