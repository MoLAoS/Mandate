// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_FLAGS_H_
#define _GLEST_GAME_FLAGS_H_

#include <cassert>
#include <stdexcept>
#include "xml_parser.h"

using std::runtime_error;
using Shared::Xml::XmlNode;

namespace Glest{ namespace Game{

class TechTree;
class FactionType;

// ===============================
// 	class template Flags
// ===============================

/**
 * A generic flag management template class.  Flags has been added to manage a
 * generic set of flags and is used to replace bool arrays for fields (air,
 * land, etc.) and unit properties (burnable, etc), in addition to new flag-like
 * data where there are a given set of values that can be true or false.  The
 * template accepts an enum type E, a max enum value (usually the xCount enum
 * value) and an optional storage type T, defaulting to int.  Flags is very
 * lightweight, consiting of only the value of type T.
 */
template<class E, size_t max, class T =unsigned int> class Flags {
public:
	T flags;

	Flags() {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
	}

	Flags(T flags) {
		assert(max <= sizeof(flags) * 8);
		this->flags = flags;
	}

	Flags(E flag1, bool val1 = true) {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
		set(flag1, val1);
	}

	Flags(E flag1, bool val1, E flag2, bool val2) {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
		set(flag1, val1);
		set(flag2, val2);
	}

	Flags(E flag1, E flag2) {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
		set(flag1, true);
		set(flag2, true);
	}

	Flags(E flag1, E flag2, E flag3) {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
		set(flag1, true);
		set(flag2, true);
		set(flag3, true);
	}

	Flags(E flag1, E flag2, E flag3, E flag4) {
		assert(max <= sizeof(flags) * 8);
		flags = 0;
		set(flag1, true);
		set(flag2, true);
		set(flag3, true);
		set(flag4, true);
	}

	/** Sets a boolean flag. */
	void set(E flag, bool value) {
		assert(flag < sizeof(flags) * 8);
		assert(flag < max);
		flags = value ? flags | (1 << flag) : flags & ~(1 << flag);
	}

	/** Retrieve the value of a boolean flag. */
	bool get(E flag) const {
		assert(flag < sizeof(flags) * 8);
		assert(flag < max);
		return flags & (1 << flag);
	}

	/** Assignment operator.  Copies the flags from the supplied T value. */
	Flags &operator =(const T &f) {
		flags = f;
		return flags;
	}

	/** Cast operator to return the flags of type T. */
	operator T() {
		return flags;
	}

	/** Clears all flags. */
	void reset() {
		flags = 0;
	}
};

// ===============================
// 	class template XmlBasedFlags
// ===============================

/**
 * XmlBasedFlags is an extension of flags which manages loading from an xml node
 * based upon an child node name and char*[] of valid values that is expected to
 * be max size. Each name is then associated with their respective values in enum
 * E.
 */
template<class E, size_t max, class T = unsigned int> class XmlBasedFlags : public Flags<E, max, T> {
public:
	virtual ~XmlBasedFlags(){}
	/**
	 * Called to initialize the Flags object from an xml node.  The derived
	 * class should in turn call the protected load method passing the
	 * appropriate values for childNodeName and flagNames.
	 */
	virtual void load(const XmlNode *node, const string &dir,
			const TechTree *tt, const FactionType *ft) = 0;

protected:
	/**
	 * Default method to initialize from an XmlNode object. This function should
	 * be called by the subclass, specifying the appropriate values for
	 * childNodeName and flagNames. The baseNode is expected to contain zero or
	 * more child nodes either with the name specified by childNodeName, or a
	 * string matching one of the values in flagNames. In the case of the
	 * former, the child node is expected to have an attribute named "value"
	 * which contains a string matching one of the values in flagNames. In the
	 * case of the later, the name of the child node alone will set the
	 * corresponding flag.
	 *
	 * @param baseNode the XmlNode object that optionally contains the flags to
	 *            set
	 * @param dir directory name of the xml file
	 * @param tt
	 * @param ft
	 * @param childNodeName the name of a single flag
	 * @param flagNames an array of pointers of size max that associates an enum
	 *            value to a string.
	 *
	 */
	void load(const XmlNode *baseNode, const string &dir, const TechTree *tt,
			const FactionType *ft, const char* childNodeName,
			const char**flagNames) {
		string nodeName;
		string flagName;
		const XmlNode *node;
		int f;

		for (int i = 0; i < baseNode->getChildCount(); ++i) {
			node = baseNode->getChild(i);
			nodeName = node->getName();
			if (nodeName == childNodeName) {
				flagName = node->getAttribute("value")->getRestrictedValue();
			} else {
				flagName = nodeName;
			}
			for (f = 0; f < max; ++f) {
				if (flagName == flagNames[f]) {
					this->set((E)f, true);
					break;
				}
			}
			if (f == max) {
				throw runtime_error(string() + "Invalid " + childNodeName + ": " + flagName + ": " + dir);
			}
		}
	}
};

}}//end namespace

#endif
