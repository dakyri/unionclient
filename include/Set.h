/*
 * Set.h
 *
 *  Created on: Apr 19, 2014
 *      Author: dak
 */

#ifndef SET_H_
#define SET_H_


#include "Notifier.h"

template <typename V> class Set:
			protected std::vector<V>, public NXR<V> {
public:
	Set() {}
	~Set() {}

	void Add(const V v)
	{
		if (!Contains(v)) {
			NotifyAddItem(v);
			Set<V>::push_back(v);
		}
	}

	bool Contains(const V v) const
	{
		for (auto it=Set<V>::begin(); it!=Set<V>::end(); ++it) {
			if (*it == v) {
				return true;
			}
		}
		return false;
	}

	typename std::vector<V> Elements() const
	{
		typename std::vector<V> el;
		for (auto it=Set<V>::begin(); it!=Set<V>::end(); ++it) {
			el.push_back(*it);
		}
		return el;
	}

	typename Set<V>::iterator Remove(const V v)
	{
		for (auto it=Set<V>::begin(); it!=Set<V>::end(); ++it) {
			if (*it == v) {
				NotifyRemoveItem(v);
				return Set<V>::erase(it);
			}
		}
		return Set<V>::end();
	}

	void Synchronize(const std::vector<V> newv) {
		auto it=Set<V>::begin();
		while (it!=Set<V>::end()) {
			V v = *it;
			bool found = false;
			for (auto jt=newv.begin(); jt!=newv.begin(); ++jt) {
				if (v == *jt) {
					found = true;
					break;
				}
			}
			if (!found) {
				it = Set<V>::erase(it);
				NotifyRemoveItem(v);
			} else {
				++it;
			}
		}
		for (auto it=newv.begin(); it!=newv.end(); ++it) {
			Add(*it);
		}
	}

	void NotifyAddItem(V item) {
		NXR<V>::NotifyListeners(Event::ADD_ITEM, item);
	}

	void NotifyRemoveItem(V item) {
		NXR<V>::NotifyListeners(Event::REMOVE_ITEM, item);
	}
};

#endif /* SET_H_ */
