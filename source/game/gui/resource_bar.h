// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_RESOURCE_BAR_H_
#define _GLEST_GAME_RESOURCE_BAR_H_

#include <string>

#include "framed_widgets.h"
#include "forward_decs.h"

using std::string;
using namespace Shared::Math;

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;
using Glest::ProtoTypes::ResourceType;

// =====================================================
// 	class ResourceBar
//
///	A bar that displays the resources
// =====================================================

class ResourceBar : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
private:
	const Faction *m_faction;
	vector<const ResourceType*> m_resourceTypes;
	vector<string> m_headerStrings;
	bool m_draggingWidget;
	Vec2i m_moveOffset;
	int m_updateCounter;

public:
	ResourceBar(Container *parent); ///@todo (ResourceBarFrame *parent)
	~ResourceBar();

	void init(const Faction *faction, std::set<const ResourceType*> &types);

	virtual void update() override;
	virtual void render() override;
	virtual string descType() const override { return "ResourceBar"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
};

// =====================================================
// 	class ResourceBarFrame
// =====================================================

class ResourceBarFrame : public Frame {
private:
	ResourceBar *m_resourceBar;

	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	ResourceBarFrame();
	ResourceBar * getResourceBar() {return m_resourceBar;}

	void setPinned(bool v);

	virtual void render() override;
};

}}//end namespace

#endif