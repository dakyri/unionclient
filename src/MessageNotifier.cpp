/*
 * MessageNotifier.cpp
 *
 *  Created on: May 19, 2014
 *      Author: dak
 */

#include <MessageNotifier.h>

const char* RxMsgBroadcastType::TO_SERVER = "0";
const char* RxMsgBroadcastType::TO_ROOMS = "1";
const char* RxMsgBroadcastType::TO_CLIENTS = "2";

/**
 * @class MessageNotifier MessageNotifier.h
 * @brief a variation on the Notifier<T...> pattern, specialized for clean and matched notifications on UPC messages
 */

MessageNotifier::MessageNotifier() {
}

MessageNotifier::~MessageNotifier() {
}


CBModMsgRef
MessageNotifier::AddMessageListener(const UserMessageID message, const CBModMsg listener) const
{
	return AddMessageListener(message, {}, {}, listener);
}

CBModMsgRef
MessageNotifier::AddMessageListener(const UserMessageID message, const std::vector<ClientID> clients, const std::vector<RoomID> rooms, const CBModMsg listener) const
{
	std::shared_ptr<CBModMsg> x = std::make_shared<CBModMsg>(listener);
	lock.lock();
	auto it = msgListeners.find(message);
	if (it == msgListeners.end()) {
		msgListeners[message].push_back(MsgCBR(message, clients, rooms, x));
	} else {
		it->second.push_back(MsgCBR(message, clients, rooms, x));
	}
	lock.unlock();
	return x;
}

void
MessageNotifier::RemoveMessageListener(const UserMessageID message, const CBModMsgRef listener) const
{
	auto it = msgListeners.find(message);
	if (it == msgListeners.end()) return;
	auto listeners = it->second;
	auto jt = listeners.begin();
	while (jt < listeners.end()) {
		if (!jt->cb.expired()) {
			CBModMsgRef scb = jt->cb.lock();
			if (scb == listener) {
				jt->cb.reset();
			}
		}
		++jt;
	}
}

bool
MessageNotifier::HasMessageListener(const UserMessageID message, const CBModMsgRef listener) const
{
	auto it = msgListeners.find(message);
	if (it == msgListeners.end()) return false;
	auto listeners = it->second;
	auto jt = listeners.begin();
	while (jt < listeners.end()) {
		if (!jt->cb.expired()) {
			CBModMsgRef scb = jt->cb.lock();
			if (scb == listener) {
				return true;
			}
		}
		++jt;
	}
	return false;
}

void
MessageNotifier::NotifyMessageListeners(const UserMessageID message, const std::vector<ClientID> clients, const std::vector<RoomID> rooms, const StringArgs msg) const {
	unsigned i=0;
	auto it = msgListeners.find(message);
	if (it == msgListeners.end()) return;
	auto listeners = it->second;
	while(i<listeners.size()) { // N.B. caution some notifiers might modify the list
		auto iter = listeners[i];
		std::weak_ptr<CBModMsg> p= iter.cb;
		if (p.expired()) {
			lock.lock();
			listeners.erase(listeners.begin() + i);
			lock.unlock();
		} else {
			std::shared_ptr<CBModMsg> s_cb = p.lock(); // create a shared_ptr from the weak_ptr
			if (clients.size() == 0 && rooms.size() == 0) { // message is for everybody ... or system message for self
//				DEBUG_OUT("NotifyMessageListeners room wildcard match");
				(*s_cb)(message, msg);
			} else if (iter.clients.size() == 0 && iter.rooms.size() == 0) { // interested in all messages
				(*s_cb)(message, msg);
			} else {
				bool match = false;
				for (auto it: iter.rooms) {
					for (auto jt: rooms) {
						if (it == jt) {
							match = true;
							break;
						}
					}
				}
				if (!match) {
					for (auto it: iter.clients) {
						for (auto jt: clients) {
							if (it == jt) {
								match = true;
								break;
							}
						}
					}
				}
				if (match) {
					(*s_cb)(message, msg);
				}
			}
			i++;
		}
	}
}



