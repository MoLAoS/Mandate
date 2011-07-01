// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_FONT_H_
#define _SHARED_GRAPHICS_FONT_H_

#include <string>

#include "vec.h"

using std::string;
using namespace Shared::Math;

namespace Shared { namespace Graphics {

// =====================================================
//	class FontMetrics
// =====================================================

class FontMetrics {
private:
	float *widths;
	float height;

	bool freeType;
	float maxAscent;
	float maxDescent;

public:
	FontMetrics();
	~FontMetrics();

	void setWidth(int i, float width)	{widths[i] = width;}
	void setHeight(float height)		{this->height = height;}

	void setMaxAscent(float ascent)		{maxAscent = ascent;}
	void setMaxDescent(float descent)	{maxDescent = descent;}

	void setFreeType(bool val)			{freeType = val;}

	Vec2f getTextDiminsions(const string &str) const;
	void wrapText(string &io_text, unsigned i_maxWidth) const;
	bool isFreeType() const { return freeType; }
	float getHeight() const;
	float getMaxAscent() const { return maxAscent; }
	float getMaxDescent() const { return maxDescent; }
};

// =====================================================
//	class Font
// =====================================================

class Font {
public:
	static const int charCount;

public:
	enum Width {
		wNormal = 400,
		wBold = 700
	};

protected:
	string type;
	int width;
	int size;
	bool inited;
	FontMetrics metrics;

public:
	//constructor & destructor
	Font();
	virtual ~Font(){};
	virtual void init() = 0;
	virtual void reInit() = 0;
	virtual void end() = 0;

	//get
	string getType() const			{return type;}
	int getWidth() const			{return width;}
	int getSize() const				{return size;}
	void setSize(int size)			{this->size= size;}

	const FontMetrics *getMetrics() const	{return &metrics;}

	//set
	void setType(string type)		{this->type= type;}
	void setWidth(int width)		{this->width= width;}
};

}}//end namespace

#endif
