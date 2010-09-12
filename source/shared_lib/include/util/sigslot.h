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

// =====================================================
//  class signal_base
// =====================================================
/// base class for signals, so they can be treated generically by slot classes
class signal_base {
public:
	virtual void slot_disconnect(has_slots* pslot) = 0;
};

// =====================================================
//  class connection_base
// =====================================================
/// template base classes for connections, so they can be treated generically by signals
template<typename ArgType> class connection_base {
public:
	virtual ~connection_base() { }
	virtual has_slots* getdest() const = 0;
	virtual void emit(ArgType) = 0;
};

// =====================================================
//  class has_slots
// =====================================================
/// slot class, signal listeners all need to inherit this
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

// =====================================================
//  class signal
// =====================================================
/// The workhorse. Can be connected to any number of SlotClass::slotMethod 'listeners'
/// [note: uses std::list to keep connection.end() valid when erasing]
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
		const_iterator it = connections.begin();
		while (it != itEnd) {
			(*it)->emit(a1);
			++it;
		}
	}

	void operator()(ArgType a1) {
		const_iterator itEnd = connections.end();
		const_iterator it = connections.begin();
		while (it != itEnd) {
			(*it)->emit(a1);
			++it;
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
				pclass->signal_disconnect(this);
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

// =====================================================
//  class connection
// =====================================================
/// concrete connection, to SlotClass::slotMethod(ArgType a1);
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

} // namespace sigslot

#endif // SIGSLOT_H__
