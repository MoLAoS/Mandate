// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_GRAPHICS_PIXMAP_H_
#define _SHARED_GRAPHICS_PIXMAP_H_

#include <string>
#include <cassert>
#include <stdexcept>

#include "png.h"

#include "vec.h"
#include "types.h"

#include "FileOps.hpp"

namespace Shared { namespace Graphics {
using std::runtime_error;
using std::string;
using namespace Platform;
using namespace Math;
using namespace PhysFS;

// =====================================================
//	class PixmapIo
// =====================================================

class PixmapIo{
protected:
	int w;
	int h;
	int components;

public:
	virtual ~PixmapIo(){}

	int getW() const			{return w;}
	int getH() const			{return h;}
	int getComponents() const	{return components;}

	virtual void openRead(const string &path)= 0;
	virtual void read(uint8 *pixels)= 0;
	virtual void read(uint8 *pixels, int components)= 0;

	virtual void openWrite(const string &path, int w, int h, int components)= 0;
	virtual void write(uint8 *pixels)= 0;
};

// =====================================================
//	class PixmapIoTga
// =====================================================

class PixmapIoTga: public PixmapIo{
private:
	FileOps *file;

public:
	PixmapIoTga();
	virtual ~PixmapIoTga();

	virtual void openRead(const string &path);
	void openRead(FileOps *f);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

// =====================================================
//	class PixmapIoBmp
// =====================================================

class PixmapIoBmp: public PixmapIo{
private:
	FileOps *file;

public:
	PixmapIoBmp();
	virtual ~PixmapIoBmp();

	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components);
	virtual void write(uint8 *pixels);
};

// =====================================================
//	class PixmapIoPng
// =====================================================

class PixmapIoPng: public PixmapIo {
private:
	FileOps *file;
	png_structp png_ptr;
	png_infop info_ptr;

public:
	PixmapIoPng(): file(0) {}
	virtual ~PixmapIoPng() {delete file;}

	virtual void openRead(const string &path);
	virtual void read(uint8 *pixels);
	virtual void read(uint8 *pixels, int components);

	virtual void openWrite(const string &path, int w, int h, int components) {
		throw runtime_error("Can't write PNG.");
	}

	virtual void write(uint8 *pixels) {
		throw runtime_error("Can't write PNG.");
	}
};



// =====================================================
//	class Pixmap1D
// =====================================================

class Pixmap1D{
protected:
	int w;
	int components;
	uint8 *pixels;

public:
	//constructor & destructor
	Pixmap1D();
	Pixmap1D(int components);
	Pixmap1D(int w, int components);
	void init(int components);
	void init(int w, int components);
	~Pixmap1D();

	//load & save
	void load(const string &path);
	void loadTga(const string &path);
	void loadBmp(const string &path);

	//get
	int getW() const			{return w;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}
};

// =====================================================
//	class Pixmap2D
// =====================================================

class Pixmap2D{
protected:
	int h;
	int w;
	int components;
	uint8 *pixels;

public:
	//constructor & destructor
	Pixmap2D();
	Pixmap2D(int components);
	Pixmap2D(int w, int h, int components);
	void init(int components);
	void init(int w, int h, int components);
	~Pixmap2D();

	//load & save
	void load(const string &path);
	void loadTga(FileOps *f);
	void loadTga(const string &path);
	void loadBmp(const string &path);
	void loadPng(const string &path);

	void save(const string &path);
	void saveBmp(const string &path);
	void saveTga(const string &path);

	//get
	int getW() const			{return w;}
	int getH() const			{return h;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}

	//get data
	void getPixel(int x, int y, uint8 *value) const;
	void getPixel(int x, int y, float32 *value) const;
	void getComponent(int x, int y, int component, uint8 &value) const	{value= pixels[(w*y+x)*components+component];}
	void getComponent(int x, int y, int component, float32 &value) const{value= pixels[(w*y+x)*components+component]/255.f;}

	//vector get
	Vec4f getPixel4f(int x, int y) const;
	Vec3f getPixel3f(int x, int y) const;
	float getPixelf(int x, int y) const								{return pixels[(w*y+x)*components]/255.f;}
	float getComponentf(int x, int y, int component) const;

	//set data
	void setPixel(int x, int y, const uint8 *value);
	void setPixel(int x, int y, const float32 *value);
	void setComponent(int x, int y, int component, uint8 value)		{pixels[(w*y+x)*components+component]= value;}
	void setComponent(int x, int y, int component, float32 value)	{pixels[(w*y+x)*components+component]= static_cast<uint8>(value*255.f);}

	//vector set
	void setPixel(const Vec2i pos, const Colour &c) {
		assert(components == 4);
		const int ndx = (w * pos.y + pos.x) * 4;
		pixels[ndx + 0] = c.r;
		pixels[ndx + 1] = c.g;
		pixels[ndx + 2] = c.b;
		pixels[ndx + 3] = c.a;
	}
	void setPixel(int x, int y, const Vec3f &p);
	void setPixel(int x, int y, const Vec4f &p);
	void setPixel(int x, int y, float p)		{pixels[(w*y+x)*components]= static_cast<uint8>(p*255.f);}

	//mass set
	void setPixels(const uint8 *value);
	void setPixels(const float32 *value);
	void setComponents(int component, uint8 value);
	void setComponents(int component, float32 value);

	//operations
	void splat(const Pixmap2D *leftUp, const Pixmap2D *rightUp, const Pixmap2D *leftDown, const Pixmap2D *rightDown);
	void lerp(float t, const Pixmap2D *pixmap1, const Pixmap2D *pixmap2);
	void copy(const Pixmap2D *sourcePixmap);
	void subCopy(int x, int y, const Pixmap2D *sourcePixmap);

private:
	bool doDimensionsAgree(const Pixmap2D *pixmap);
};

// =====================================================
//	class Pixmap3D
// =====================================================

class Pixmap3D{
protected:
	int h;
	int w;
	int d;
	int components;
	uint8 *pixels;

public:
	//constructor & destructor
	Pixmap3D();
	Pixmap3D(int w, int h, int d, int components);
	Pixmap3D(int d, int components);
	void init(int w, int h, int d, int components);
	void init(int d, int components);
	~Pixmap3D();

	//load & save
	void loadSlice(const string &path, int slice);
	void loadSliceBmp(const string &path, int slice);
	void loadSliceTga(const string &path, int slice);

	//get
	int getW() const			{return w;}
	int getH() const			{return h;}
	int getD() const			{return d;}
	int getComponents() const	{return components;}
	uint8 *getPixels() const	{return pixels;}
};

// =====================================================
//	class PixmapCube
// =====================================================

class PixmapCube{
public:
	enum Face{
		fPositiveX,
		fNegativeX,
		fPositiveY,
		fNegativeY,
		fPositiveZ,
		fNegativeZ
	};

protected:
	Pixmap2D faces[6];

public:
	//init
	void init(int w, int h, int components);

	//load & save
	void loadFace(const string &path, int face);
	void loadFaceBmp(const string &path, int face);
	void loadFaceTga(const string &path, int face);

	//get
	Pixmap2D *getFace(int face)				{return &faces[face];}
	const Pixmap2D *getFace(int face) const	{return &faces[face];}
};

}}//end namespace

#endif
