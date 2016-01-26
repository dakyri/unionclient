/*
 * Manager.h
 *
 *  Created on: Apr 20, 2014
 *      Author: dak
 */

#ifndef MANAGER_H_
#define MANAGER_H_

#include "CommonTypes.h"
#include "Logger.h"

/**
 * @class Manager<V,K> Manager.h
 * @brief base class for an object which manages a cache of objects of type V, indexed primarily by key K.
 */
template <typename V, typename K> class Manager {
public:
	typedef std::shared_ptr<const V> Ref;

	/**
	 * @private
	 * constructor, is passed small functionals which control the mapping between V and K
	 * @param map functional which maps a reference to a V onto a key
	 * @param validate functional indicating that an object reference is valid (typically that the shared pointer is non null)
	 * @param log a logger ... what it says
	 */
	Manager(std::function<K(Ref)> map, std::function<bool(Ref)> validate, ILogger&log)
		: log(log)
		, cache(map, validate) {}
	virtual ~Manager() {}

	/**
	 * the object managers name for itself ... mainly for logging purposes
	 */
	virtual std::string GetName() const=0;

	/**
	 * implemented by subclasses to find an object by any means necessary, including but not necessarily
	 * limited to the cache
	 */
	virtual Ref Get(const K k) const=0;

	/**
	 * implemented by subclasses to indicate that a key is valid
	 */
	virtual bool Valid(const K k) const=0;

	/**
	 * try to find a reference to the object, and if none found, make a new object and add it to the cache
	 */
	Ref Request(const K k) {
		if (!Valid(k)) {
			log.Warn(GetName()+" Request() failed. Supplied ID '"+k+"' was invalid.");
			return Ref();
		}

		Ref r = Get(k);
		if (r) {
			return r;
		} else {
			log.Debug(GetName()+" Request() creating new object for id [" + k + "]");
			return CreateCached(k);
		}
	}

	ILogger& GetLog()
	{
		return log;
	}

	void SetLog(ILogger &l) {
		log = l;
	}

protected:
	/**
	 * finds object with key k in the cache
	 */
	Ref GetCached(const K k) const
	{
		return cache.Get(k);
	}

	/**
	 * adds a fresh object to the cache, first checking that there is no key clash
	 */
	void AddCached(const Ref vr) const
	{
		Ref cached = cache.Get(cache.map(vr));
		if (cached.use_count() == 0) {
			log.Debug(GetName()+" adding cached object: [" + cache.map(vr) + "]");
			cache.Add(vr);
		}
	}

	/**
	 * adds a fresh object to the cache, first checking that there is no key clash
	 */
	Ref CreateCached(const K k)
	{
		Ref cached = cache.Get(k);
		if (cached.use_count() == 0) {
			log.Debug(GetName()+" creating object to be cached: [" + k + "]");
			V* v = Make(k);
			if (v != nullptr) {
				Ref r = std::shared_ptr<V>(v);
				cache.Add(r);
				return r;
			} else {
				log.Debug(GetName()+" AddCached(). Make() returns null while making object: [" + k + "]");
			}
		} else {
		}
		return cached;
	}

	/**
	 * what it says!
	 */
	bool HasCached(const K k) const
	{
		return cache.Contains(k);
	}

	/**
	 * what it says!
	 */
	Ref RemoveCached(const K k) const
	{
		if (cache.Contains(k)) {
			return cache.Remove(k);
		} else {
			log.Error(GetName()+" could not remove cached room: [" + k + "]." + " Room not found.");
		}
		return Ref();
	}

	/**
	 * what it says!
	 */
	void ClearCache()
	{
		cache.RemoveAll();
	}

	/**
	 * appends every element in the cache to the given vector
	 */
	void AppendCached(std::vector<Ref> va) const
	{
		cache.AppendTo(va);
	}

	/**
	 * appends every element in the cache to the given vector
	 */
	void AppendCached(Map<K,Ref> ma) const
	{
		cache.AppendTo(ma);
	}


	void ApplyToCached(std::function<void(Ref)> f) const
	{
		cache.ApplyToAll(f);
	}

	void ApplyBoolCached(std::function<bool(Ref)> f) const
	{
		cache.ApplyBool(f);
	}

	/**
	 * implemented by the subcclasses to make an object ... it returns a POP, so that we
	 * can leave the implentation of a reference and storage internal to this class
	 */
	virtual V* Make(const K k)=0;

	ILogger& log;
private:
	Cache<K,Ref> mutable cache;
};

#endif /* MANAGER_H_ */
