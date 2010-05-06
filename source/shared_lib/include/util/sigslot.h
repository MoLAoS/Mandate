// sigslot.h: Signal/Slot classes
// 
// Written by Sarah Thompson (sarah@telergy.com) 2002.
//
// License: Public domain. You are free to use this code however you like, with the proviso that
//          the author takes on no responsibility or liability for any use.
//
// Hacked to pieces by James McCulloch (silnarm at gmail) 2010.
//
#ifndef SIGSLOT_H__
#define SIGSLOT_H__

#include <set>
#include <list>
#include <algorithm>

namespace sigslot {

class has_slots;
template<class SlotClass, typename ArgType> class connection;

class signal_base {
public:
	virtual void slot_disconnect(has_slots* pslot) = 0;
};

template<typename ArgType> class connection_base {
public:
	virtual ~connection_base() { }
	virtual has_slots* getdest() const = 0;
	virtual void emit(ArgType) = 0;
};

template<typename ArgType> class signal : public signal_base {
public:
	typedef typename std::list<connection_base<ArgType> *>  connections_list;
	typedef typename connections_list::const_iterator const_iterator;
	typedef typename connections_list::iterator iterator;

protected:
	connections_list connections;

public:
	signal()	{ ; }
	~signal()	{ disconnect_all(); }

	template<class SlotClass>
	void connect(SlotClass* pclass, void (SlotClass::*pmemfun)(ArgType)) {
		connection<SlotClass, ArgType>* conn = 
			new connection<SlotClass, ArgType>(pclass, pmemfun);
		connections.push_back(conn);
		pclass->signal_connect(this);
	}

	void emit(ArgType a1) {
		const_iterator itEnd = connections.end();
		for (const_iterator it = connections.begin(); it != itEnd; ++it) {
			(*it)->emit(a1);
		}
	}

	void operator()(ArgType a1) {
		const_iterator itEnd = connections.end();
		for (const_iterator it = connections.begin(); it != itEnd; ++it) {
			(*it)->emit(a1);
		}
	}

	void disconnect_all() {
		const_iterator it = connections.begin();
		const_iterator itEnd = connections.end();

		while (it != itEnd) {
			(*it)->getdest()->signal_disconnect(this);
			delete *it;
			++it;
		}
		connections.clear();
	}

	void disconnect(has_slots* pclass) {
		iterator it = connections.begin();
		iterator itEnd = connections.end();

		while (it != itEnd) {
			if ((*it)->getdest() == pclass) {
				delete *it;
				connections.erase(it);
				(*it)->getdest()/*pclass*/->signal_disconnect(this);
				return;
			}
			++it;
		}
	}

	void slot_disconnect(has_slots *slot) {
		iterator it = connections.begin();
		iterator itEnd = connections.end();

		while (it != itEnd) {
			if ((*it)->getdest() == slot) {
				delete *it;
				it = connections.erase(it);
			} else {
				++it;
			}
		}
	}
};


template<class SlotClass, typename ArgType> class connection : public connection_base<ArgType> {
public:
	connection() : slotObject(0), slotMethod(0) { }
	connection(SlotClass* slotObject, void (SlotClass::*slotMethod)(ArgType))
				: slotObject(slotObject), slotMethod(slotMethod) { }

	virtual ~connection() { }

	virtual void emit(ArgType a1) { (slotObject->*slotMethod)(a1); }
	virtual has_slots* getdest() const { return slotObject; }

private:
	SlotClass* slotObject;
	void (SlotClass::*slotMethod)(ArgType);
};

class has_slots {
private:
	typedef std::set<signal_base*> sender_set;
	typedef sender_set::const_iterator const_iterator;

public:
	has_slots() { }
	virtual ~has_slots() { disconnect_all(); }

	void signal_connect(signal_base* sender) {		m_senders.insert(sender); }
	void signal_disconnect(signal_base* sender) {	m_senders.erase(sender); }

	void disconnect_all() {
		const_iterator it = m_senders.begin();
		const_iterator itEnd = m_senders.end();

		while (it != itEnd) {
			(*it)->slot_disconnect(this);
			++it;
		}
		m_senders.clear();
	}

private:
	sender_set m_senders;
};

} // namespace sigslot

#endif // SIGSLOT_H__
