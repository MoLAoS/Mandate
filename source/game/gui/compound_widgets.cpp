// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"

#include "compound_widgets.h"
#include "core_data.h"
#include "lang.h"

namespace Glest { namespace Widgets {
using Shared::Util::intToStr;
using Global::CoreData;
using Global::Lang;
using Sim::ControlTypeNames;

PlayerSlotWidget::PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size) {
	Panel::setAutoLayout(false);
	Widget::setBorderParams(BorderStyle::SOLID, 2, Vec3f(0.f), 0.4f);
	Widget::setPadding(0);
	assert(size.x > 200);
	float size_x = float(size.x - 25);
	float fwidths[] = { size_x * 20.f / 100.f, size_x * 35.f / 100.f, size_x * 35.f / 100.f, size_x * 10.f / 100.f};
	int widths[4];
	for (int i=0; i < 4; ++i) {
		widths[i] = int(fwidths[i]);
	}
	Vec2i cpos(5, 2);
	
	CoreData &coreData = CoreData::getInstance();

	stLabel = new StaticText(this, cpos, Vec2i(widths[0], 30));
	stLabel->setTextParams("Player #", Vec4f(1.f), coreData.getfreeTypeMenuFont(), true);
	
	cpos.x += widths[0] + 5;
	dlControl = new DropList(this, cpos, Vec2i(widths[1], 30));
	dlControl->setBorderSize(0);
	foreach_enum (ControlType, ct) {
		dlControl->addItem(theLang.get(ControlTypeNames[ct]));
	}
	
	cpos.x += widths[1] + 5;	
	dlFaction = new DropList(this, cpos, Vec2i(widths[2], 30));
	dlFaction->setBorderSize(0);

	cpos.x += widths[2] + 5;	
	dlTeam = new DropList(this, cpos, Vec2i(widths[3], 30));
	for (int i=1; i <= GameConstants::maxPlayers; ++i) {
		dlTeam->addItem(intToStr(i));
	}
	dlTeam->setBorderSize(0);

	dlControl->SelectionChanged.connect(this, &PlayerSlotWidget::onControlChanged);
	dlFaction->SelectionChanged.connect(this, &PlayerSlotWidget::onFactionChanged);
	dlTeam->SelectionChanged.connect(this, &PlayerSlotWidget::onTeamChanged);
}

}}
