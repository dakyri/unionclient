/*
 * ClientManifest.cpp
 *
 *  Created on: Apr 17, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class ClientManifest ClientManifest.h
 * class representing the client serialized info passed back from the server by the u54 call, among others
 */

ClientManifest::ClientManifest() {
}

ClientManifest::~ClientManifest() {
}

/**
 * main deserialization hook
 */
void
ClientManifest::Deserialize(ClientID _clientID, UserID _userID, std::string serializedOccupiedRoomIDs, std::string serializedObservedRoomIDs,
		std::string globalAttrs, std::vector<std::string> roomAttrs) {
	clientID = _clientID;
	userID = _userID;
	DeserializeOccupiedRoomIDs(serializedOccupiedRoomIDs);
	DeserializeObservedRoomIDs(serializedObservedRoomIDs);
	DeserializeAttributesByScope(Token::GLOBAL_ATTR, globalAttrs);

	auto it=roomAttrs.begin();
	while (it!=roomAttrs.end()) {
		std::string cid(*it);
		++it;
		DeserializeAttributesByScope(cid, *(it));
		++it;
	}
};

/**
 * deserialize the room id list passed as a '|' separated string into a list of occupied rooms
 */
void
ClientManifest::DeserializeOccupiedRoomIDs(std::string roomIDs) {
	if (roomIDs == "") {
		occupiedRoomIDs = {};
		return;
	}
	std::stringstream idss(roomIDs);
	std::string item;
	char c = *Token::RS;
	auto it=occupiedRoomIDs.begin();
	while (std::getline(idss, item, c)) {
		occupiedRoomIDs.insert(it,item);
	}
};

/**
 * deserialize the room id list passed as a '|' separated string into a list of observed rooms
 */
void
ClientManifest::DeserializeObservedRoomIDs(std::string roomIDs) {
	if (roomIDs == "") {
		occupiedRoomIDs = {};
		return;
	}
	std::stringstream idss(roomIDs);
	std::string item;
	char c = *Token::RS;
	auto it=occupiedRoomIDs.begin();
	while (std::getline(idss, item, c)) {
		observedRoomIDs.insert(it,item);
	}
};

/**
 * deserialize the passed string attribute list
 */
void
ClientManifest::DeserializeAttributesByScope(AttrScope scope, std::string serializedAttributes) {
	if (serializedAttributes == "") {
		return;
	}
	std::vector<ClientID> attrList;
	std::stringstream idss(serializedAttributes);
	std::string item;
	char c = *Token::RS;
	while (std::getline(idss, item, c)) {
		attrList.push_back(item);
	}

	for (int i = (int)attrList.size()-3; i >= 0; i -=3) {
		if (atoi(attrList[i+2].c_str()) & Attribute::FLAG_PERSISTENT) {
			persistentAttributes.SetAttribute(attrList[i], attrList[i+1], scope);
		} else {
			transientAttributes.SetAttribute(attrList[i], attrList[i+1], scope);
		}
	}
};
