// ====================================================================
//	This file is part of the Mandate Engine (www.lordofthedawn.com)
//
//	Copyright (C) 2012 Matt Shafer-Skelton <taomastercu@yahoo.com>
//
//	It is released under the terms of the GNU General Public License 3
//
// ====================================================================

#ifndef _GLEST_GAME_FORMATIONS_H_
#define _GLEST_GAME_FORMATIONS_H_

#include "forward_decs.h"
#include "xml_parser.h"
#include "element_type.h"

namespace Glest { namespace Hierarchy {
using namespace ProtoTypes;
using namespace Shared::Xml;
using namespace Shared::Util;
using Gui::Clicks;

// ===============================
// 	class Formation
// ===============================

typedef vector<Vec2i> Positions;

class Line {
public:
    string name;
    Positions line;
};

typedef vector<Line> Lines;

class Formation : public DisplayableType {
public:
    string title;
    Lines lines;

public:
    bool load(const XmlNode *formationNode, const string &dir);

};

// =====================================================
// 	FormationDescriptor
// =====================================================

class FormationDescriptor {
public:
	virtual void setHeader(const string &header) = 0;
	virtual void setTipText(const string &mainText) = 0;
	virtual void addElement(const string &msg) = 0;
	virtual void addItem(const DisplayableType *dt, const string &msg) = 0;
	virtual void addReq(bool ok, const DisplayableType *dt, const string &msg) = 0;
};

}}//end namespace

#endif
