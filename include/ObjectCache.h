/*
 * ObjectCache.h
 *
 *  Created on: Apr 17, 2014
 *      Author: dak
 */

#ifndef OBJECTCACHE_H_
#define OBJECTCACHE_H_

#include "Map.h"

/**
 * a cache for objects referenced by a key. at the moment, not really a cache, just a simple unordered map, but this is
 * the locus for doing something more sophisticated in that direction
 */
template <typename K, typename V> class Cache: public Map<K, V> {
public:
	Cache(std::function<K(V)> map, std::function<bool(V)> validate): Cache<K,V>::Map(map,validate) {}
	virtual ~Cache() {}

};

#endif /* OBJECTCACHE_H_ */
