/*
 * ClientManifest.h
 *
 *  Created on: Apr 17, 2014
 *      Author: dak
 */

#ifndef CLIENTMANIFEST_H_
#define CLIENTMANIFEST_H_

#include "CommonTypes.h"

class ClientManifest {
public:
	ClientManifest();
	virtual ~ClientManifest();

	void Deserialize(ClientID clientID, UserID userID, std::string serializedOccupiedRoomIDs, std::string serializedObservedRoomIDs,
			std::string globalAttrs, std::vector<std::string> roomAttrs);
	void DeserializeAttributesByScope(AttrScope scope, std::string serializedAttributes);
	void DeserializeObservedRoomIDs(std::string roomIDs);
	void DeserializeOccupiedRoomIDs(std::string roomIDs);

    UserID userID;
    ClientID clientID;
	AttributeCollection transientAttributes;
    AttributeCollection persistentAttributes;
    std::vector<RoomID> observedRoomIDs;
    std::vector<RoomID> occupiedRoomIDs;
};

#endif /* CLIENTMANIFEST_H_ */
