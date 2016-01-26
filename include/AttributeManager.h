/*
 * AttributeManager.h
 *
 *  Created on: Apr 12, 2014
 *      Author: dak
 */

#ifndef ATTRIBUTEMANAGER_H_
#define ATTRIBUTEMANAGER_H_

#include "CommonTypes.h"

class AttributeManager
	: public NotifyAttrInfo
{
	friend class Client;
	friend class Room;
	friend class Account;
	friend class UnionBridge;
public:
	AttributeManager(/*AttributeOwner& owner, UnionBridge& bridge, ILogger &log*/);
	virtual ~AttributeManager();

	void SendUPC(const UPCMessageID, const StringArgs setRequest) const;

	const AttrMap GetAttributes() const;
	AttrVal GetAttribute(const AttrName attrName, const AttrScope attrScope="") const;
	AttrMap GetAttributesByScope(const AttrScope scope) const;

	void DeleteAttribute(const AttrName attrName, const AttrScope scope) const;
	void SetAttribute(const AttrName attrName, const AttrVal attrValue, const AttrScope attrScope = "",
			const bool isShared = true, const bool isPersistent=false, const bool evaluate = false) const;

	AttributeCollection& GetAttributeCollection() const;

	virtual std::string GetName() const =0;
	virtual UPCMessageID DeleteAttribCommand() const=0;
	virtual UPCMessageID SetAttribCommand() const=0;
	virtual StringArgs DeleteAttribRequest(const AttrName attrName, const AttrScope scope) const=0;
	virtual StringArgs SetAttribRequest(const AttrName attrName, const AttrVal attrValue, const AttrScope scope, const unsigned int options) const=0;
	virtual bool SetLocalAttribs() const= 0;
	virtual void SendUPC(const UPCMessageID, const StringArgs) = 0;

protected:
	void SetAttributeCollection(const AttributeCollection set);
	void SetAttributeLocal(const AttrName attrName, const AttrVal attrVal, const AttrScope attrScope, const ClientRef byClient=nullptr) const;
	void RemoveAttributeLocal(const AttrName attrName, const AttrScope attrScope, const ClientRef byClient=nullptr) const;
	void RemoveAllAttributes();
	void OnSetAttributeResult(AttrName attrName, AttrScope attrScope, UPCStatus status) const;
	void OnDeleteAttributeResult(AttrName attrName, AttrScope attrScope, UPCStatus status) const;

	void RegisterAttributeListeners();
	void UnregisterAttributeListeners();

//	AttributeOwner& owner;
//	UnionBridge& unionBridge;
//	ILogger& log;

	mutable AttributeCollection attributes;

	CBAttrInfoRef updateAttributeListener;
	CBAttrInfoRef deleteAttributeListener;

};

#endif /* ATTRIBUTEMANAGER_H_ */
