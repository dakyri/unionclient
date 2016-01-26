/*
 * Map
 *
 *  Created on: Apr 18, 2014
 *      Author: dak
 */

#ifndef MAP_
#define MAP_

#include "Notifier.h"

template <typename K, typename V> class Map:
			public std::unordered_map<K, V>/*, public NXR<V>*/ {
public:
	Map(std::function<K(V)> map, std::function<bool(V)> valid)
			: map(map)
			, valid(valid){

	}
	~Map() {}

	std::function<K(V)> map;
	std::function<bool(V)> valid;

	void ApplyToAll(std::function<void(V)> f) const
	{
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			if (valid(it->second)) f(it->second);
		}
	}

	bool ApplyBool(std::function<bool(V)> f) const
	{
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			if (valid(it->second) && !f(it->second)) {
				return false;
			}
		}
		return true;
	}

	bool Add(const K k, const V v)
	{
		if (!valid(v)) return false;
		if (Map<K,V>::find(k) != Map<K,V>::end()) {
			return false;
		}
//		NotifyAddItem(v);
		this->emplace(k, v);
		return true;
	}

	bool Add(const V v)
	{
		if (!valid(v)) return false;
		K k = map(v);
		if (Map<K,V>::find(k) != Map<K,V>::end()) {
			return false;
		}
//		NotifyAddItem(v);
		this->emplace(k, v);
		return true;
	}


	int Length() const
	{
		return (int) Map<K,V>::size();
	}

	const Map<K,V> &GetAll() const
	{
		return *this;
	}


	V Get(const K id) const
	{
		if (Contains(id)) {
			return Map<K,V>::at(id);
		}
		return V();
	}

	V Remove(const K id)
	{
		V c;
		auto it=Map<K,V>::find(id);
		if (it != Map<K,V>::end()) {
			c = it->second;
//			NotifyRemoveItem(c);
			Map<K,V>::erase(id);
		}
		return c;
	}


	void RemoveAll()
	{
		auto it=Map<K,V>::begin();
		while (it != Map<K,V>::end()) {
			V c = it->second;
//			NotifyRemoveItem(c);
			it = Map<K,V>::erase(it);
		}
	}

	void RemoveAllApplying(std::function<void(V)> f)
	{
		auto it=Map<K,V>::begin();
		while (it != Map<K,V>::end()) {
			V c = it->second;
			it = Map<K,V>::erase(it);
//			NotifyRemoveItem(c);
			f(c);
		}
	}

	bool Contains(const K id) const
	{
		auto it = Map<K,V>::find(id);
		return it!=Map<K,V>::end();
	}

	bool Contains(const V v) const
	{
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			if (v == it->second) {
				return true;
			}
		}
		return false;
	}

	void Append(const std::vector<V> s)
	{
		for (auto it=s.begin(); it!=s.end(); ++it) {
			Add(*it);
		}
	}

	void Append(const Map<K,V> &s)
	{
		for (auto it=s.begin(); it!=s.end(); ++it) {
			Add(it->first, it->second);
		}
	}


	void AppendTo(std::vector<V> list) const
	{
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			V r = it->second;
			bool inList = false;
			for (auto jt=list.begin(); jt!=list.end(); ++jt) {
				if (r == *jt) {
					inList = true;
					break;
				}
			}
			if (!inList) {
				list.push_back(r);
			}
		}
	}

	void AppendTo(Map<K,V> list) const
	{
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			V r = it->second;
			if (list.find(it->first) == list.end()) {
				list[it->first] = it->second;
			}
		}
	}

	typename std::vector<K> GetKeys() const
	{
		std::vector<K> kl;
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			kl.push_back(it->first);
		}
		return kl;
	}

	typename std::vector<V> GetValues() const
	{
		std::vector<V> kl;
		for (auto it=Map<K,V>::begin(); it!=Map<K,V>::end(); it++) {
			kl.push_back(it->second);
		}
		return kl;
	}
/*
	void NotifyAddItem(V item) {
		NXR<V>::NotifyListeners(CollectionEvent::ADD_ITEM, item);
	}

	void NotifyRemoveItem(V item) {
		NXR<V>::NotifyListeners(CollectionEvent::REMOVE_ITEM, item);
	}
*/
};



#endif /* MAP_ */
