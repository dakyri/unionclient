/*
 * AttributeManager.cpp
 *
 *  Created on: Apr 12, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

/**
 * @class AttributeManager AttributeManager.h
 * base class to handle the attribute management of any object that can have attributes, in particular, rooms and clients. this base is the main interface for
 * server attribute operations on these objects and maintains a local cache of attribute values.
 * Abstract and requires several virtuals to be implemented by its inheritors
 */
AttributeManager::AttributeManager(/*AttributeOwner&owner, UnionBridge& unionBridge, ILogger& log*/)
//	: owner(owner)
//	, unionBridge(unionBridge)
//	, log(log)
{
	DEBUG_OUT("AttributeManager::AttributeManager()");

	updateAttributeListener = std::make_shared<CBAttrInfo>([this](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
		NotifyListeners(t, n, v, s, ov, c, status);
	});
	deleteAttributeListener = std::make_shared<CBAttrInfo>([this](EventType t, AttrName n, AttrVal v, AttrScope s, AttrVal ov, ClientRef c, UPCStatus status) {
		NotifyListeners(t, n, v, s, ov, c, status);
	});
	RegisterAttributeListeners();
}

AttributeManager::~AttributeManager() {
	UnregisterAttributeListeners();
	DEBUG_OUT("AttributeManager::~AttributeManager()");
	attributes.Clear();
}

/**
 * sends a server delete for the given attribute in the given scope
 */
void
AttributeManager::DeleteAttribute(const AttrName attrName, const AttrScope attrScope) const
{
	StringArgs req = DeleteAttribRequest(attrName, attrScope);
	if (attrScope != "") req.push_back(attrScope);
	const_cast<AttributeManager*>(this)->SendUPC(DeleteAttribCommand(), req);

}

/**
 * sends a server set request for the given attribute in the given scope, with the given value and properties
 */
void
AttributeManager::SetAttribute(const AttrName attrName, const AttrVal attrValue, const AttrScope attrScope,
		const bool isShared, const bool isPersistent, const bool evaluate) const
{
	// Create an integer to hold the attribute options.
	unsigned int attrOptions = 0;
	if (isShared) attrOptions |= Attribute::FLAG_SHARED;
	if (evaluate) attrOptions |= Attribute::FLAG_EVALUATE;
	if (isPersistent) attrOptions |= Attribute::FLAG_PERSISTENT;

	AttrScope scope=attrScope;
	if (scope == "") {
		scope = Token::GLOBAL_ATTR;
	} else if (!UPCUtils::IsValidAttributeScope(scope)) {
//		log.Error("Cannot set attribute. Illegal scope. Illegal attribute is: " + attrName + "=" + attrValue);
	}

	StringArgs req = SetAttribRequest(attrName, attrValue, scope, attrOptions);
	const_cast<AttributeManager*>(this)->SendUPC(SetAttribCommand(), req);
	if (SetLocalAttribs() && !evaluate) {
		SetAttributeLocal(attrName, attrValue, scope);
	}
}

/**
 * @return reference to the main attribute collections
 */
AttributeCollection&
AttributeManager::GetAttributeCollection() const{
	return attributes;
};

/**
 * sets the main attribute collection
 */
void
AttributeManager::SetAttributeCollection(const AttributeCollection set) {
	UnregisterAttributeListeners();
	attributes = set;
	RegisterAttributeListeners();
};

/**
 * @return the value of the given attribute
 */
AttrVal
AttributeManager::GetAttribute(const AttrName attrName, const AttrScope attrScope) const {
	// Quit if there are no attrbutes.
	if (attributes.Length()==0) {
		return AttrVal();
	} else {
		return attributes.GetAttribute(attrName, attrScope);
	}
};

/**
 * returns a flattened version of the main attribute collection (merging scopes)
 */
const AttrMap
AttributeManager::GetAttributes() const {
  return attributes.GetAll();
}

/**
 * returns a flattened version of the main attribute connection matching a particular scope
 */
AttrMap
AttributeManager::GetAttributesByScope(const AttrScope scope) const {
  return attributes.GetByScope(scope);
};

/**
 * sets an attribute in the given local map
 */
void
AttributeManager::SetAttributeLocal(
		const AttrName attrName,
		const AttrVal attrVal,
		const AttrScope attrScope,
		const ClientRef byClient) const {
	bool changed = attributes.SetAttribute(attrName, attrVal, attrScope, byClient);
	if (!changed) {
//		log.Info(owner.GetName() + " New attribute value for [" + attrName + "] matches old value. Not changed.");
	}
};

/**
 * removes an attribute from the local map
 */
void
AttributeManager::RemoveAttributeLocal(
		const AttrName attrName,
		const AttrScope attrScope,
		const ClientRef byClient) const {
	bool deleted = attributes.DeleteAttribute(attrName, attrScope, byClient);
	if (!deleted) {
//		log.Info(owner.GetName() + " Delete attribute failed for [" + attrName + "]. No such attribute.");
	}
};

/**
 * clears all attributes
 */
void
AttributeManager::RemoveAllAttributes() {
	attributes.Clear();
}

/**
 * start listening to changes in the collection
 */
void
AttributeManager::RegisterAttributeListeners() {
	attributes.AddListener(Event::UPDATE_ATTR, updateAttributeListener);
	attributes.AddListener(Event::REMOVE_ATTR, deleteAttributeListener);
}

/**
 * stop listening to changes in the collection
 */
void
AttributeManager::UnregisterAttributeListeners() {
	attributes.RemoveListener(Event::UPDATE_ATTR, updateAttributeListener);
	attributes.RemoveListener(Event::REMOVE_ATTR, deleteAttributeListener);
}


/**
 * call back for when we have a successful attribute change
 */
void
AttributeManager::OnSetAttributeResult(AttrName attrName, AttrScope attrScope, UPCStatus status) const {
	const_cast<AttributeManager*>(this)->NotifyListeners(Event::UPDATE_ATTR_RESULT, attrName, "", attrScope, "", ClientRef(), status);
}

/**
 * call back for when we have a successful attribute removal
 */
void
AttributeManager::OnDeleteAttributeResult(AttrName attrName, AttrScope attrScope, UPCStatus status) const {
	const_cast<AttributeManager*>(this)->NotifyListeners(Event::REMOVE_ATTR_RESULT, attrName, "", attrScope, "", ClientRef(), status);
}

