/*
 * RoomAttributes.h
 *
 *  Created on: Apr 10, 2014
 *      Author: dak
 */

#ifndef ROOMATTRIBUTES_H_
#define ROOMATTRIBUTES_H_

#include "CommonTypes.h"

// TODO storage of Attribute attributes?

class Attribute {
public:
	static const int FLAG_SHARED     = 1 << 2;
	static const int FLAG_PERSISTENT = 1 << 3;
	static const int FLAG_IMMUTABLE  = 1 << 5;
	static const int FLAG_EVALUATE   = 1 << 8;

	Attribute(AttrName name, AttrVal value, bool shared=false, bool persistent=false, bool immutable=false);
	virtual ~Attribute();

	AttrName name;
	AttrVal value;
	AttrVal oldValue;
	AttrScope scope;
	ClientRef client;

	bool shared;
	bool persistent;
	bool immutable;
};

class AttributeInitializer: public std::initializer_list<Attribute> {
public:
	AttributeInitializer();
	virtual ~AttributeInitializer();

	std::string Serialize() const;
};

class AttrMap: private std::unordered_map<AttrName, AttrVal> {
public:
	AttrMap();
	~AttrMap();

	int Length() const;
	bool Contains(const AttrName name) const;
	void SetAttribute(const AttrName name, const AttrVal value, const ClientRef byClient);
	bool DeleteAttribute(const AttrName name, const ClientRef byClient);
	const AttrVal GetValue(const AttrName names) const;
	void SetValue(const AttrName name, const AttrVal v);
	AttrMap &Append(const AttrMap &m);
	void AppendTo(const std::string base, const std::string x, AttrMap &m) const;
	std::vector<AttrName> GetAttributesNames() const;
};

class AttributeCollection: private std::unordered_map<AttrScope, AttrMap>, public NXAttrInfo {
public:
	AttributeCollection();
	~AttributeCollection();

	void Clear();
	int Length() const;
	std::vector<AttrScope> GetScopes() const;
	void Add(const AttributeCollection& collection);
	bool Contains(const AttrName name, const AttrScope scope) const;
	bool HasScope(const AttrScope scope) const;
	bool DeleteAttribute(const AttrName name, const AttrScope scope, const ClientRef byClient=nullptr);
	std::vector<AttrName> GetAttributesNamesForScope(const AttrScope scope) const;
	void SynchronizeScope(const AttrScope scope, const AttributeCollection& set);
	bool SetAttribute(const AttrName name, const AttrVal value, const AttrScope scope, const ClientRef byClient=nullptr);
	AttrVal GetAttribute(const AttrName attrName, const AttrScope attrScope) const;
	AttrMap GetAll() const;
	AttrMap GetByScope(const AttrScope scope) const;
};

#endif /* ROOMATTRIBUTES_H_ */
