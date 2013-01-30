// ==============================================================
//	This file is part of The Mandate Engine
//
//	Copyright (C) 2012	Matt Shafer-skelton <taomastercu@yahoo.com>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_GAME_MAPDISPLAY_H_
#define _GLEST_GAME_MAPDISPLAY_H_

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
#include "map_command.h"

using std::string;

using Shared::Graphics::Texture2D;
using namespace Shared::Math;
using Shared::Util::replaceBy;
using Glest::Global::Metrics;
using Glest::ProtoTypes::TechTree;

namespace Glest { namespace Gui_Mandate {

using namespace Widgets;
using namespace ProtoTypes;
using namespace Gui;

WRAPPED_ENUM( MapDisplaySection, COMMANDS )

struct MapDisplayButton {
	MapDisplaySection	        m_section;
	int				        m_index;

	MapDisplayButton(MapDisplaySection s, int ndx) : m_section(s), m_index(ndx) {}

	MapDisplayButton& operator=(const MapDisplayButton &that) {
		this->m_section = that.m_section;
		this->m_index = that.m_index;
		return *this;
	}

	bool operator==(const MapDisplayButton &that) const {
		return (this->m_section == that.m_section && this->m_index == that.m_index);
	}

	bool operator!=(const MapDisplayButton &that) const {
		return (this->m_section != that.m_section || this->m_index != that.m_index);
	}
};

// =====================================================
// 	class MapDisplay
//
///	Display for unit commands, and unit selection
// =====================================================

class MapDisplay : public Widget, public MouseWidget, public ImageWidget, public TextWidget {
public:
	static const int cellWidthCount = 6;
	static const int cellHeightCount = 5;

	static const int commandCellCount = cellWidthCount * cellHeightCount;

	static const int invalidIndex = -1;

private:
    Tileset* m_tileset;

    int   m_iconSize;
	bool  m_draggingWidget;

	UserInterface *m_ui;
	bool downLighted[commandCellCount];
	int index[commandCellCount];
	int m_selectedCommandIndex;
	int m_logo;

	int m_imageSize;

	struct SizeCollection {
		Vec2i logoSize;
		Vec2i commandSize;
	};
	SizeCollection m_sizes;
	FuzzySize m_fuzzySize;
	Vec2i	m_commandOffset;
	MapDisplayButton	m_hoverBtn,
                        m_pressedBtn;
	const FontMetrics *m_fontMetrics;
	CommandTip  *m_toolTip;
    vector<MapBuild> m_mapBuilds;
private:
	void layout();
	void renderProgressBar();

public:
	MapDisplay(Container *parent, UserInterface *ui, Vec2i pos);

    bool building;
    MapBuild currentMapBuild;

	//get
	int getBuildCount() const                       {return m_mapBuilds.size();}
	MapBuild getMapBuild(int i) const               {return m_mapBuilds[i];}
	FuzzySize getFuzzySize() const                  {return m_fuzzySize;}
	int getIndex(int i)	                            {return index[i];}
	bool getDownLighted(int index) const			{return downLighted[index];}

	//set
	void setSize();
	void setFuzzySize(FuzzySize fSize);
	void setToolTipText2(const string &hdr, const string &tip, MapDisplaySection i_section = MapDisplaySection::COMMANDS);

	void setDownImage(int i, const Texture2D *image)	 {setImage(image, i);}
	void setDownLighted(int i, bool lighted)			 {downLighted[i]= lighted;}
	void setIndex(int i, int value)						 {index[i] = value;}
	void setSelectedCommandPos(int i);

	//misc
	void init(Tileset *tileset);

	void computeBuildInfo(int posDisplay);
	void onFirstTierSelect(int posBuild);
    void buildButtonPressed(int posDisplay);

	void clear();
	void resetTipPos(Vec2i i_offset);
	void resetTipPos() {resetTipPos(m_commandOffset);}
	MapDisplayButton computeIndex(Vec2i pos, bool screenPos = false);
	MapDisplayButton getHoverButton() const { return m_hoverBtn; }
	void persist();
	void reset();

	virtual void render() override;
	virtual string descType() const override { return "MapPanel"; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos) override;
	virtual bool mouseUp(MouseButton btn, Vec2i pos) override;
	virtual bool mouseMove(Vec2i pos) override;
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos) override;
	virtual void mouseOut() override;

	public:
    void computeMapCommandPanel();
    void computeBuildPanel();


};

// =====================================================
//  class MapDisplayFrame
// =====================================================

class MapDisplayFrame : public Frame {
private:
	MapDisplay *m_mapDisplay;
	UserInterface *m_ui;

public:
	void onExpand(Widget*);
	void onShrink(Widget*);

public:
	MapDisplayFrame(UserInterface *ui, Vec2i pos);
	MapDisplay* getMapDisplay() {return m_mapDisplay;}

	void resetSize();

	virtual void render() override;
	void setPinned(bool v);



};

}}//end namespace

#endif
