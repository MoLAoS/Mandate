// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_PRODUCTION_DISPLAY_H_
#define _GLEST_GAME_PRODUCTION_DISPLAY_H_

#include <string>
#include "texture.h"
#include "util.h"
#include "game_util.h"
#include "metrics.h"
#include "widgets.h"
#include "forward_decs.h"
#include "selection.h"
#include "framed_widgets.h"
#include "tool_tips.h"

using std::string;

using Shared::Graphics::Texture2D;
using namespace Shared::Math;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;
using namespace Glest::Entities;

namespace Glest { namespace Gui {

using namespace Widgets;
using namespace ProtoTypes;
using Entities::Faction;

class UserInterface;

WRAPPED_ENUM( ProductionDisplaySection, RESOURCES, ITEMS, PROCESSES, UNITS )

struct ProductionDisplayButton {
	ProductionDisplaySection m_section;
	int m_index;

	ProductionDisplayButton(ProductionDisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	ProductionDisplayButton& operator=(const ProductionDisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const ProductionDisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const ProductionDisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class ProductionWindow
//
///	Display for production
// =====================================================

class ProductionWindow : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 4;
	static const int resourceCellCount = cellWidthCount * cellHeightCount;
	static const int itemCellCount = cellWidthCount * cellHeightCount;
	static const int processCellCount = cellWidthCount * cellHeightCount;
	static const int unitCellCount = cellWidthCount * cellHeightCount;
	static const int invalidIndex = -1;
private:
	UserInterface *m_ui;
	//bool downLighted[0];
	int index[1];
	int m_logo;
	int m_imageSize;
	struct SizeItemCollection {
		Vec2i logoSize;
		Vec2i displaySize;
	};
	SizeItemCollection m_sizes;
	FuzzySize m_fuzzySize;
	Vec2i m_resourceOffset,
	m_itemOffset,
	m_processOffset,
	m_unitOffset;
	ProductionDisplayButton m_hoverBtn,
                      m_pressedBtn;
	CommandTip *m_toolTip;
	const FontMetrics *m_fontMetrics;
private:
	void layout();
public:
	ProductionWindow(Container *parent, UserInterface *ui, Vec2i pos);

	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	int getIndex(int i)								{return index[i];}
	//bool getDownLighted(int index) const			{return downLighted[index];}
	CommandTip* getCommandTip()                     {return m_toolTip;}

	void setSize();
	void setFuzzySize(FuzzySize fSize);

	void setUpImage(int i, const Texture2D *image) 		        {setImage(image, i);}
	void setDownImage(int i, const Texture2D *image)	        {setImage(image, i);}
	//void setDownLighted(int i, bool lighted)			        {downLighted[i] = lighted;}
	void setIndex(int i, int value)						        {index[i] = value;}

	void ProductionWindow::tick();

	void computeResourcesPanel();
	void computeResourcesInfo(int posDisplay);
	void computeItemPanel();
	void computeItemInfo(int posDisplay);
	void computeProcessPanel();
	void computeProcessInfo(int posDisplay);
	void computeUnitPanel();
	void computeUnitInfo(int posDisplay);

	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_resourceOffset);}
	ProductionDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	ProductionDisplayButton getHoverButton() const { return m_hoverBtn; }
	void persist();
	void reset();

	void setToolTipText2(const string &hdr, const string &tip, ProductionDisplaySection i_section);

	virtual void render() override;
	virtual string descType() const override { return "DisplayPanel"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;
};

// =====================================================
//  class ItemDisplayFrame
// =====================================================

class ProductionDisplayFrame : public Frame {
private:
	ProductionWindow *m_display;
	UserInterface *m_ui;

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	ProductionDisplayFrame(UserInterface *ui, Vec2i pos);
	ProductionWindow* getProductionDisplay() {return m_display;}

	void resetSize();

	virtual void render() override;
	void setPinned(bool v);
};

}}//end namespace

#endif
