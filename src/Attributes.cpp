/*
 * Attributes.cpp
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#include "UCUpperHeaders.h"

AttributeInitializer::AttributeInitializer() {
}

AttributeInitializer::~AttributeInitializer() {
}

std::string
AttributeInitializer::Serialize() const
{
	std::string serialized = "";
	unsigned i=0;
	for (AttributeInitializer::iterator attr=begin(); attr!=end(); ++attr) {
		unsigned attrSettings = 0;
		attrSettings |= attr->shared ? Attribute::FLAG_SHARED : 0;
		attrSettings |= attr->persistent ?  Attribute::FLAG_PERSISTENT : 0;
		attrSettings |= attr->immutable ?  Attribute::FLAG_IMMUTABLE : 0;
		serialized += attr->name
			  + Token::RS + attr->value
			  + Token::RS
			  + std_to_string(attrSettings);
//			   + std::to_string(attrSettings);

		if (++i < size()) {
			serialized += Token::RS;
		}
	}
	return serialized;
}

/**
 * TODO in changing to stl types some of the semantics have changed. "" is now an invalid/unfound attribute
 * must check ... perhaps return pair or POP
 * TODO consider basing these from the internal notifying Map<K,V> class
 */
AttrMap::AttrMap()
{

}

AttrMap::~AttrMap()
{

}

bool
AttrMap::Contains(const AttrName name) const
{
	auto it = find(name);
	return (it != end());
}

std::vector<AttrName>
AttrMap::GetAttributesNames()  const
{
	std::vector<AttrName> vex;
	for (auto jit=begin(); jit!=end(); jit++) {
		vex.push_back(jit->first);
	}
	return vex;
}

const AttrName
AttrMap::GetValue(const AttrName n) const
{
	return at(n);
}

void
AttrMap::SetValue(const AttrName n, const AttrVal v)
{
	emplace(n, v);
}

AttrMap &
AttrMap::Append(const AttrMap &m)
{
	for (auto it=m.begin(); it!=m.end(); ++it) {
		emplace(it->first, it->second);
	}
	return (*this);
}

int
AttrMap::Length() const
{
	return (int) size();
}

bool
AttrMap::DeleteAttribute(const AttrName name, const ClientRef byClient)
{
	erase(name);
	return true;
}


AttributeCollection::AttributeCollection() {
}

AttributeCollection::~AttributeCollection() {
}

int
AttributeCollection::Length() const
{
	return (int) size();
}

/**
 * @private
 */
bool
AttributeCollection::HasScope(const AttrScope scope) const
{
	auto it = find(scope);
	return it != end();
}

bool
AttributeCollection::SetAttribute(const AttrName name, const AttrVal value, const AttrScope attrScope, ClientRef byClient) {
	bool scopeExists=false;
	bool attrExists=false;
	AttrVal oldVal;

	AttrScope scope = (attrScope == "") ? Token::GLOBAL_ATTR : attrScope; // null scope means global scope
	auto it = find(scope);
	if (it != end()) {	// Check if the scope and attr exist already
		scopeExists = true;
		if (it->second.Contains(name)) {
			attrExists = true;
		}
	}
	if (attrExists) {
		oldVal = it->second.GetValue(name);
		if (oldVal == value) {
			return false;
		}
	}

	if (!scopeExists) {
		emplace(scope, AttrMap());
	}
	at(scope).SetValue(name, value);
	NotifyListeners(Event::UPDATE_ATTR, name, value, scope, oldVal, byClient, UPC::Status::SUCCESS);

	return true;
};

/**
 * @private
 */
bool
AttributeCollection::DeleteAttribute(const AttrName name, const AttrScope scope, const ClientRef byClient) {
//	bool lastAttr = true;
	AttrVal value;

	auto it = find(scope);
	if (it != end() && it->second.Contains(name)) {
		value = at(scope).GetValue(name);
		at(scope).DeleteAttribute(name, byClient);
		if (at(scope).Length() == 0) {
			erase(scope);
		}

//		NotifyListeners(Event::REMOVE, name, value, scope, AttrVal(), byClient, UPC::Status::SUCCESS);
		return true;
	}
	return false;
};

/**
 * @private
 */
void
AttributeCollection::Clear () {
	clear();
};

AttrMap
AttributeCollection::GetByScope(const AttrScope scope) const {
	AttrMap map;

	if (scope == "") {
		for (auto it=begin(); it != end(); ++it) {
			map.Append(it->second);
		}
	} else {
		if (HasScope(scope)) {
			map.Append(at(scope));
		}
	}

	return map;
};

std::vector<AttrName>
AttributeCollection::GetAttributesNamesForScope(const AttrScope scope) const {
	std::vector<AttrName> nex;
	auto it = find(scope);
	if (it != end()) {
		nex = it->second.GetAttributesNames();
	}
	return nex;
};

void
AttrMap::AppendTo(const std::string k, const std::string tag, AttrMap &attrs) const
{
	for (auto jit=begin(); jit!=end(); ++jit) {
		attrs.at(tag+jit->first) = jit->second;
	}
}

AttrMap
AttributeCollection::GetAll() const {
	AttrMap attrs;
	for (auto it=begin(); it != end(); ++it) {
		it->second.AppendTo(it->first, it->first == Token::GLOBAL_ATTR ? "":(it->first + "."), attrs);
	}
	return attrs;
}

AttrVal
AttributeCollection::GetAttribute(const AttrName attrName, const AttrScope attrScope) const {
	AttrScope scope = attrScope;
	if (attrScope == "") {// Use the global scope when no scope is specified
		scope = Token::GLOBAL_ATTR;
	}

	if (Contains(attrName, scope)) {
		return at(scope).GetValue(attrName);
	} else {
		return AttrVal();
	}
};

std::vector<AttrScope>
AttributeCollection::GetScopes() const {
	std::vector<AttrScope> scopes;
	for (auto it=begin(); it!=end(); ++it) {
		scopes.push_back(it->first);
	}
	return scopes;
};

bool
AttributeCollection::Contains(const AttrName name, const AttrScope scope) const {
	auto it = find(scope);
	return (it != end()) ? it->second.Contains(name) : false;
};


/**
 * @private
 */
void
AttributeCollection::Add(const AttributeCollection& collection) {
	std::vector<AttrScope> scopes = collection.GetScopes();
	for (auto jit=scopes.begin(); jit != scopes.end(); ++jit) {
		std::vector<AttrName> names = collection.GetAttributesNamesForScope(*jit);
		for (auto kit=scopes.begin(); kit != scopes.end(); ++kit) {
			SetAttribute(*kit, collection.GetAttribute(*kit, *jit), *jit);
		}
	}
}

/**
 * @private
 */
void
AttributeCollection::SynchronizeScope(const AttrScope scope, const AttributeCollection& set)
{
	std::vector<AttrName> names = GetAttributesNamesForScope(scope);

	for (auto it=names.begin(); it != names.end(); ++it) {
		if (!set.Contains(*it, scope)) {
			DeleteAttribute(*it, scope);
		}
	}

	// Set all new attributes (unchanged attributes are ignored)
	names = set.GetAttributesNamesForScope(scope);
	for (auto it=names.begin(); it != names.end(); ++it) {
		SetAttribute(*it, set.GetAttribute(*it, scope), scope);
	}
}
