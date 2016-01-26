/*
 * Notifier.h
 *
 *  Created on: Apr 9, 2014
 *	  Author: dak
 */

#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include "CommonTypes.h"

/**
 * @class NXR<CBP...> Notifier.h
 *  simple and lightweight notifier patter, storing weak pointer to std:function callbacks. The function<CBP..> has a type NXR<CBP...>::CB and is passed in
 *  a shared pointer.
 *
 * about as thread safe as std::vector ... which is not especially, at least on push_back ... reading is ok
 *
 * FIXME ... investigate .. should we _maybe_ get a shared pointer to the called object as well as the callback ... ? if the object gets deleted
 * between time of grabbing the lock and the time of actually calling the callback ... unlikely but not impossible ... or is it... likely the callback
 * is bound to 'this' in the listening object and will use members.
 */
template <typename ... CBP> class NXR {
public:
	/** type for our callback */
	typedef typename std::function<void(EventType, CBP ...)> CB;

	/** type for a tuple of our params */
	typedef typename std::tuple<CBP ...> CP;

	/** structure for holding our callback and corresponding event */
	struct NR
	{
		NR(EventType eventType, std::shared_ptr<CB> cb)
			: eventType(eventType)
			, cb(cb) {	}
		EventType eventType;
		std::weak_ptr<CB> cb;
	};

	/**
	 * adds a listener for an event type, converting the passed shared pointer to a weak one
	 * TODO perhaps it would be good to add a virtual to verify that the notifier does actually support a notification on this event with these
	 * parameters. atm keeping it simple and light so that it can be added as a functionality to a lot of objects
	 */
	bool AddListener(const EventType eventType, std::shared_ptr<CB> listener) const {
		listeners.push_back(NR(eventType, listener));
		return true;
	}

	/**
	 * removes a lister if found by resetting its weak pointer, so it will be cleared on a subsequent notification. removing the record directly in here
	 * may cause threading issues. listeners may be removed just by reseting the shared pointer, but there may be occasions where something more
	 * precise is needed
	 */
	void RemoveListener(const EventType eventType, std::shared_ptr<CB> callback) const{
		size_t i = 0;
		while (i < listeners.size()) {
			NR iter = listeners[i];
			if (!iter.cb.expired()) {
				std::shared_ptr<CB> scb = iter.cb.lock();
				if (iter.eventType == eventType && scb == callback) {
					iter.cb.reset();
					break;
				}
			}
			++i;
		}
	}

	bool HasListener(const EventType eventType, std::shared_ptr<CB> callback) const{
		int i=0;
		while (i < listeners.size()) {
			NR iter = listeners[i];
			if (!iter.cb.expired()) {
				std::shared_ptr<CB> scb = iter.cb.lock();
				if ((eventType == -1 || iter.eventType == eventType) && scb == callback) {
					return true;
				}
			}
			++i;
		}
		return false;
	}


	virtual ~NXR() {

	}
protected:
	/**
	 *  In most cases, this default implementation is what's wanted ... but if we have multiple event types etc etc maybe we want to
	 * adjust the parameters ... basic error checking or .... perhaps a virtual
	 */
	void Notify(EventType eventType, CB*listener, CBP...args) {
		(*listener)(eventType, args...);
	};

public:
	/**
	 * tell any listeners on a particular event that we've found something interesting
	 * FIXME ... protecting the notifylisteners causes accessibility issues in multiple inheritors
	 */
	void NotifyListeners(EventType eventType, CBP...args) {
		size_t i = 0;
		while (i < listeners.size()) {
			NR iter = listeners[i]; // xxx critical also ... what happens when the undelying data structure is realloc'd
			std::weak_ptr<CB> p= iter.cb;
			if (p.expired()) {
				// xxx critical, seriously so
				listeners.erase(listeners.begin()+i);
			} else {
				if (iter.eventType == eventType) {
					std::shared_ptr<CB> s_cb = p.lock(); // create a shared_ptr from the weak_ptr
					CB* listener = s_cb.get();
					Notify(eventType, listener, args...);
				}
				i++;
			}
		}
	}
	std::vector<NR> mutable listeners;
};

/**
 * @class Notifier<CBP...> Notifier.h
 * a slightly less safe, less light, easier to use version of NXR<CBP...>. The notifier stores a shared pointer to a function passed, under the assumption
 * that it is likely this would be being used by a class that has a longer shelf life than the client library .. so any shared pointer I keep here is
 * likely to remain valid.
 */
template <typename ... CBP> class Notifier: public NXR<CBP...> {
public:
	/**
	 * a slightly more convenient way of dealing with the notifier. the shared pointer is returned and the listener will disappear when this goes
	 * out of context. NB: the caller must store a copy of the handle so returned or it will be automatically deleted
	 * @param a callback function, which could (conveniently) be a bound lambda
	 * @return a reference to the created call back which can be deleted using RemoveListener in the base class
	 */
	std::shared_ptr<std::function<void(EventType, CBP ...)>> AddEventListener(const EventType eventType, std::function<void(EventType, CBP ...)> rawListener) const {
		std::shared_ptr<std::function<void(EventType, CBP ...)>> x =
				std::make_shared<std::function<void(EventType, CBP ...)>>(rawListener);
		this->AddListener(eventType, x);
		return x;
	}
};

/**
 * unpack a variadic tuple into a variadic member function ...
 * http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
 *
 * totally awesome solution ... if you're reading this, check the discussion and send jurgen a beer
 */
template<int ...> struct seq { };
template<int N, int ...S>
	struct gens : gens<N-1, N-1, S...> { };
template<int ...S>
	struct gens<0, S...> {
		typedef seq<S...> type;
	};


/**
 * @class Dispatcher<CBP ...> Notifier.h
 * a variation on Notifier<CBP ...> which separates the activity and the call of the notification, so suited for threading the event notification callbacks
 * atm relying on thread safety on std::vector, which is I don't think sufficient for this purpose .. and c++11 threads and mutexes are inconsistent between
 * platforms a big xxx on this, as we for the moment rely on the event loop to provide locking ...
 */
template <typename ... CBP> class Dispatcher: public Notifier<CBP...> {
public:
	typedef std::function<void(EventType, CBP ...)> ECB;
	struct ER {
		ER(EventType eventType, std::weak_ptr<ECB> cb, CBP...params)
			: eventType(eventType)
			, cb(cb)
			, params(params...) { }
		EventType eventType;
		std::weak_ptr<ECB> cb;
		std::tuple<CBP...> params;
	};
	/**
	 * pure genius. this is the guts of the unpacking
	 */
	template<int ...S>
		void CallFunc(seq<S...>, ECB* listener, EventType eventType, std::tuple<CBP...> params) {
			(*listener)(eventType, std::get<S>(params) ...);
		}
	/**
	 * dispatch an event ... ie push a call and its parameters onto a queue for execution in another thread ...
	 * so ... there should be no way that the calling thread will be held up by dodgy callbacks
	 */
	void DispatchEvent(const EventType eventType, CBP...params) {
		int i=0;
		while(i<this->listeners.size()) { // N.B. caution some notifiers might modify the list
			auto iter = this->listeners[i];
			std::weak_ptr<ECB> p= iter.cb;
			if (p.expired()) {
				this->listeners.erase(this->listeners.begin()+i);
			} else {
				if (iter.eventType == eventType) {
//					std::weak_ptr<ECB> listener = p.lock(); // create a shared_ptr from the weak_ptr
//					ECB* listener = s_cb.get();
					pendingLock.lock();
					pending.push_back(ER(eventType, iter.cb, params...));
					pendingLock.unlock();
				}
				++i;
			}
		}
	}

	/**
	 * process the array of pending calls
	 * pending array should be thread safe in the manner we require
	 */
	void ProcessDispatches()
	{
		int i=((int)pending.size())-1;
		while (i>=0) {
			ER it = pending[i];
			std::weak_ptr<ECB> p= it.cb;
			if (!p.expired()) {
				std::shared_ptr<ECB> s_cb = p.lock(); // create a shared_ptr from the weak_ptr
				ECB* listener = s_cb.get();
				CallFunc(typename gens<sizeof...(CBP)>::type(), listener, it.eventType, it.params);
//				(*listener)(it.eventType, it.params);
			}
			pendingLock.lock();
			pending.erase(pending.begin()+i);
			pendingLock.unlock();
			--i;
		}
	}
	std::mutex pendingLock;
	std::vector<ER> mutable pending;
};

#endif /* NOTIFIER_H_ */
