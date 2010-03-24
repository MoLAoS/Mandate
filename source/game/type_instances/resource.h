// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================
#ifndef _GLEST_GAME_RESOURCE_H_
#define _GLEST_GAME_RESOURCE_H_

#include <string>

#include "vec.h"
#include "xml_parser.h"
#include "sigslot.h"

using std::string;
using Shared::Xml::XmlNode;

namespace Glest{ namespace Game{

using Shared::Math::Vec2i;

class ResourceType;
class TechTree;

// =====================================================
// 	class Resource
//
/// Amount of a given ResourceType
// =====================================================

class Resource {
private:
    int amount;
    const ResourceType *type;
	Vec2i pos;
	int balance;

public:
	void init(const XmlNode *node, const TechTree *tt);
    void init(const ResourceType *rt, int amount);
    void init(const ResourceType *rt, const Vec2i &pos);

	int getAmount() const					{return amount;}
	const ResourceType * getType() const	{return type;}
	Vec2i getPos() const					{return pos;}
	int getBalance() const					{return balance;}
	string getDescription() const;

	void setAmount(int amount)				{
		if (this->amount == amount) return;
		this->amount = amount;
		AmountChanged(amount);
	}
	void setBalance(int balance)			{
		if (this->balance == balance) return;
		this->balance = balance;
		BalanceChanged(balance);
	}

    bool decAmount(int i);
	void save(XmlNode *node) const;

	sigslot::signal1<Vec2i>		Depleted;
	sigslot::signal1<int>		AmountChanged;
	sigslot::signal1<int>		BalanceChanged;

};

}}// end namespace

#endif
