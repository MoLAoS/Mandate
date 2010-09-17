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

#include "pch.h"
#include "pixmap.h"

#include <stdexcept>
#include <cstdio>
#include <cassert>

#include "util.h"
#include "math_util.h"
#include "random.h"

#include "leak_dumper.h"
#include "FSFactory.hpp"

#include "profiler.h"

using std::max;
using namespace Shared::Util;

namespace Shared{ namespace Graphics{

using namespace Util;

// =====================================================
//	file structs
// =====================================================

#pragma pack(push, 1)

struct BitmapFileHeader{
	uint8 type1;
	uint8 type2;
	uint32 size;
	uint16 reserved1;
	uint16 reserved2;
	uint32 offsetBits;
};

struct BitmapInfoHeader{
	uint32 size;
	int32 width;
	int32 height;
	uint16 planes;
	uint16 bitCount;
	uint32 compression;
	uint32 sizeImage;
	int32 xPelsPerMeter;
	int32 yPelsPerMeter;
	uint32 clrUsed;
	uint32 clrImportant;
};

struct TargaFileHeader{
	int8 idLength;
	int8 colourMapType;
	int8 dataTypeCode;
	int16 colourMapOrigin;
	int16 colourMapLength;
	int8 colourMapDepth;
	int16 xOrigin;
	int16 yOrigin;
	int16 width;
	int16 height;
	int8 bitsPerPixel;
	int8 imageDescriptor;
};

#pragma pack(pop)

const int tgaUncompressedRgb= 2;
const int tgaUncompressedBw= 3;

// =====================================================
//	class PixmapIoTga
// =====================================================

PixmapIoTga::PixmapIoTga(){
	file= NULL;
}

PixmapIoTga::~PixmapIoTga(){
	if(file!=NULL){
		delete file;
	}
}

void PixmapIoTga::openRead(const string &path){
	file = FSFactory::getInstance()->getFileOps();
	try {
		file->openRead(path.c_str());
	} catch (runtime_error &e) {
		// FIXME: path should really be in game but this is the common load function, maybe have a
		// member for default/missing texture path that's set from game.
		printf("%s\n", e.what());
		// will then throw exception again if even this texture is missing
		file->openRead("data/core/misc_textures/default.tga");
	}

	//read header
	TargaFileHeader fileHeader;
	file->read(&fileHeader, sizeof(TargaFileHeader), 1);

	//check that we can load this tga file
	if(fileHeader.idLength!=0){
		throw runtime_error(path + ": id field is not 0");
	}

	if(fileHeader.dataTypeCode!=tgaUncompressedRgb && fileHeader.dataTypeCode!=tgaUncompressedBw){
		throw runtime_error(path + ": only uncompressed BW and RGB targa images are supported");
	}

	//check bits per pixel
	if(fileHeader.bitsPerPixel!=8 && fileHeader.bitsPerPixel!=24 && fileHeader.bitsPerPixel!=32){
		throw runtime_error(path + ": only 8, 24 and 32 bit targa images are supported");
	}

	h= fileHeader.height;
    w= fileHeader.width;
	components= fileHeader.bitsPerPixel/8;
}

void PixmapIoTga::read(uint8 *pixels){
	read(pixels, components);
}

void PixmapIoTga::read(uint8 *pixels, int components) {
	//_PROFILE_FUNCTION();
	assert(this->components > 0 && components > 0);
	size_t imgDataSize = h * w * this->components;
	size_t pixmapDataSize = h * w * components;
	uint8 *buf = new uint8[imgDataSize];
	file->read(buf, imgDataSize, 1);

	for (int i=0, ndx = 0; i < imgDataSize; i += this->components, ndx += components) {
		uint8 r, g, b, a, l;
		switch(this->components) {
			case 1:
				r = g = b = l = buf[i];
				a = 255U;
				break;
			case 3:
				b = buf[i+0];
				g = buf[i+1];
				r = buf[i+2];
				a = 255U;
				if (components == 1) {
					l = uint8((int(r) + g + b) / 3);
				}
				break;
			case 4:
				b = buf[i+0];
				g = buf[i+1];
				r = buf[i+2];
				a = buf[i+3];
				if (components == 1) {
					l = uint8((int(r) + g + b) / 3);
				}
		}
		switch(components) {
			case 1:
				pixels[ndx+0] = l;
				break;
			case 3:
				pixels[ndx+0] = r;
				pixels[ndx+1] = g;
				pixels[ndx+2] = b;
				break;
			case 4:
				pixels[ndx+0]= r;
				pixels[ndx+1]= g;
				pixels[ndx+2]= b;
				pixels[ndx+3]= a;
				break;
		}
	}
	delete [] buf;
}

void PixmapIoTga::openWrite(const string &path, int w, int h, int components){
    this->w= w;
	this->h= h;
	this->components= components;

	file = FSFactory::getInstance()->getFileOps();
	file->openWrite(path.c_str());

	TargaFileHeader fileHeader;
	memset(&fileHeader, 0, sizeof(TargaFileHeader));
	fileHeader.dataTypeCode= components==1? tgaUncompressedBw: tgaUncompressedRgb;
	fileHeader.bitsPerPixel= components*8;
	fileHeader.width= w;
	fileHeader.height= h;
	fileHeader.imageDescriptor= components==4? 8: 0;

	file->write(&fileHeader, sizeof(TargaFileHeader), 1);
}

void PixmapIoTga::write(uint8 *pixels){
	if(components==1){
		file->write(pixels, h*w, 1);
	}
	else{
		for(int i=0; i<h*w*components; i+=components){
			file->write(&pixels[i+2], 1, 1);
			file->write(&pixels[i+1], 1, 1);
			file->write(&pixels[i], 1, 1);
			if(components==4){
				file->write(&pixels[i+3], 1, 1);
			}
		}
	}
}

// =====================================================
//	class PixmapIoBmp
// =====================================================

PixmapIoBmp::PixmapIoBmp(){
	file = NULL;
}

PixmapIoBmp::~PixmapIoBmp(){
	if (file != NULL) {
		delete file;
	}
}

void PixmapIoBmp::openRead(const string &path){
	file = FSFactory::getInstance()->getFileOps();
	file->openRead(path.c_str());

	//read file header
    BitmapFileHeader fileHeader;
    file->read(&fileHeader, sizeof(BitmapFileHeader), 1);
	if (fileHeader.type1!='B' || fileHeader.type2!='M') {
		throw runtime_error(path +" is not a bitmap");
	}

	//read info header
	BitmapInfoHeader infoHeader;
	file->read(&infoHeader, sizeof(BitmapInfoHeader), 1);
	if (infoHeader.bitCount != 24) {
        throw runtime_error(path+" is not a 24 bit bitmap");
	}

    h = infoHeader.height;
    w = infoHeader.width;
	components= 3;
}

void PixmapIoBmp::read(uint8 *pixels){
	read(pixels, 3);
}

void PixmapIoBmp::read(uint8 *pixels, int components){
	_PROFILE_FUNCTION();
	//const int alignOffset = (w * this->components) % 4;
	const int rowPad = 0;//alignOffset ? 4 - alignOffset : 0;
	const int rowSize = w * this->components + rowPad;
	uint8 *rowBuf = new uint8[rowSize];
	int pix = 0;
	assert(file->bytesRemaining() >= rowSize * h); // some bmps seem to have 2 extra bytes... ?!?

	for (int y=0; y < h; ++y) {
		file->read(rowBuf, rowSize, 1);
		for (int x=0; x < w; ++x) {
			uint8 &b = *(rowBuf + x * this->components + 0);
			uint8 &g = *(rowBuf + x * this->components + 1);
			uint8 &r = *(rowBuf + x * this->components + 2);
			switch (components) {
				case 1:
					pixels[pix++] = (r + g + b) / 3;
					break;
				case 3:
					pixels[pix++] = r;
					pixels[pix++] = g;
					pixels[pix++] = b;
					break;
				case 4:
					pixels[pix++] = r;
					pixels[pix++] = g;
					pixels[pix++] = b;
					pixels[pix++] = 255;
					break;
			}
		}
	}
	assert(pix == components * w * h);
	delete [] rowBuf;
}

void PixmapIoBmp::openWrite(const string &path, int w, int h, int components){
    this->w= w;
	this->h= h;
	this->components= components;

	file = FSFactory::getInstance()->getFileOps();
	file->openWrite(path.c_str());

	BitmapFileHeader fileHeader;
    fileHeader.type1='B';
	fileHeader.type2='M';
	fileHeader.offsetBits=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader);
	fileHeader.size=sizeof(BitmapFileHeader)+sizeof(BitmapInfoHeader)+3*h*w;

    file->write(&fileHeader, sizeof(BitmapFileHeader), 1);

	//info header
	BitmapInfoHeader infoHeader;
	infoHeader.bitCount=24;
	infoHeader.clrImportant=0;
	infoHeader.clrUsed=0;
	infoHeader.compression=0;
	infoHeader.height= h;
	infoHeader.planes=1;
	infoHeader.size=sizeof(BitmapInfoHeader);
	infoHeader.sizeImage=0;
	infoHeader.width= w;
	infoHeader.xPelsPerMeter= 0;
	infoHeader.yPelsPerMeter= 0;

	file->write(&infoHeader, sizeof(BitmapInfoHeader), 1);
}

void PixmapIoBmp::write(uint8 *pixels){
    for (int i=0; i<h*w*components; i+=components){
        file->write(&pixels[i+2], 1, 1);
		file->write(&pixels[i+1], 1, 1);
		file->write(&pixels[i], 1, 1);
    }
}

// =====================================================
//	class PixmapIoPng
// =====================================================

// Callback for PNG-decompression
static void user_read_data(png_structp read_ptr, png_bytep data, png_size_t length) {
	FileOps *f = ((FileOps*)png_get_io_ptr(read_ptr));
	if (f->read((void*)data, length, 1) != 1) {
		png_error(read_ptr,"Could not read from png-file");
	}
}

void PixmapIoPng::openRead(const string &path) {
	file = FSFactory::getInstance()->getFileOps();
	file->openRead(path.c_str());

	int length = file->fileSize();
	uint8 *buffer = new uint8[8];
	file->read(buffer, 8, 1);

	if (png_sig_cmp(buffer, 0, 8) != 0) {
		delete [] buffer;
		throw runtime_error("magic cookie mismatch, file is not PNG");
	}
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		delete [] buffer;
		throw runtime_error("png_create_read_struct() failed.");
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL,(png_infopp)NULL);
		delete [] buffer;
		throw runtime_error("png_create_info_struct() failed.");
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp)NULL);
		delete [] buffer;
		throw runtime_error("error during init_io");
	}

	png_set_read_fn(png_ptr, file, user_read_data);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	
	w = info_ptr->width;
	h = info_ptr->height;
	int color_type = info_ptr->color_type;
	int bit_depth = info_ptr->bit_depth;

	// We want RGB, 24 bit
	if (color_type == PNG_COLOR_TYPE_PALETTE || (color_type == PNG_COLOR_TYPE_GRAY && info_ptr->bit_depth < 8) || (info_ptr->valid & PNG_INFO_tRNS)) {
		png_set_expand(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(png_ptr);
	}
	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	components = info_ptr->rowbytes / info_ptr->width;

	delete [] buffer;
}

void PixmapIoPng::read(uint8 *pixels) {
	read(pixels, 3);
}

void PixmapIoPng::read(uint8 *pixels, int components) {
	png_bytep* row_pointers = new png_bytep[h];

	if (setjmp(png_jmpbuf(png_ptr))) {
		delete[] row_pointers;
		throw runtime_error("error during read_image");
	}
	for (int y = 0; y < h; ++y) {
		row_pointers[y] = new png_byte[info_ptr->rowbytes];
	}
	png_read_image(png_ptr, row_pointers);

	// Copy image
	const int rowbytes = info_ptr->rowbytes;
	int location = 0;
	for (int y = h - 1; y >= 0; --y) { // you have to somehow invert the lines
		if (components == this->components) {
			memcpy(pixels + location, row_pointers[y], rowbytes);
		} else {
			int r,g,b,a,l;
			for (int xPic = 0, xFile = 0; xFile < rowbytes; xPic += components, xFile += this->components) {
				switch (this->components) {
					case 1:
						r = g = b = l = row_pointers[y][xFile];
						a = 255;
						break;
					case 3:
						r = row_pointers[y][xFile];
						g = row_pointers[y][xFile+1];
						b = row_pointers[y][xFile+2];
						l = (r+g+b+2)/3;
						a = 255;
						break;
					case 4:
						r = row_pointers[y][xFile];
						g = row_pointers[y][xFile+1];
						b = row_pointers[y][xFile+2];
						l = (r+g+b+2)/3;
						a = row_pointers[y][xFile+3];	
						break;
					default:
						throw runtime_error("PNG file has invalid number of colour channels");
				}
				switch (components) {
					case 1:
						pixels[location+xPic] = l;
						break;
					case 4:
						pixels[location+xPic+3] = a; //Next case
					case 3:
						pixels[location+xPic] = r;
						pixels[location+xPic+1] = g;
						pixels[location+xPic+2] = b;
						break;
					default:
						throw runtime_error("PixmapIoPng request for invalid number of colour channels");
				}
			}
		}
		location += components * w;
	}
	for (int y = 0; y < h; ++y) {
		delete [] row_pointers[y];
	}
	delete[] row_pointers;
	png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
}

// =====================================================
//	class Pixmap1D
// =====================================================


// ===================== PUBLIC ========================

Pixmap1D::Pixmap1D() {
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap1D::Pixmap1D(int components){
	init(components);
}

Pixmap1D::Pixmap1D(int w, int components){
	init(w, components);
}

void Pixmap1D::init(int components){
	this->w= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap1D::init(int w, int components){
	this->w= w;
	this->components= components;
	pixels= new uint8[w*components];
}

Pixmap1D::~Pixmap1D(){
	delete [] pixels;
}

void Pixmap1D::load(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		loadBmp(path);
	}
	else if(extension=="tga"){
		loadTga(path);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap1D::loadBmp(const string &path){
	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	if(plb.getH()==1){
		w= plb.getW();
	}
	else if(plb.getW()==1){
		w= plb.getH();
	}
	else{
		throw runtime_error("One of the texture dimensions must be 1");
	}

	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[w*components];
	}

	//data
	plb.read(pixels, components);
}

void Pixmap1D::loadTga(const string &path){

	PixmapIoTga plt;
	plt.openRead(path);

	//init
	if(plt.getH()==1){
		w= plt.getW();
	}
	else if(plt.getW()==1){
		w= plt.getH();
	}
	else{
		throw runtime_error("One of the texture dimensions must be 1");
	}

	int fileComponents= plt.getComponents();

	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[w*components];
	}

	//read data
	plt.read(pixels, components);
}

// =====================================================
//	class Pixmap2D
// =====================================================

// ===================== PUBLIC ========================

Pixmap2D::Pixmap2D(){
    h= -1;
    w= -1;
	components= -1;
    pixels= NULL;
}

Pixmap2D::Pixmap2D(int components){
	init(components);
}

Pixmap2D::Pixmap2D(int w, int h, int components){
	init(w, h, components);
}

void Pixmap2D::init(int components){
	this->w= -1;
	this->h= -1;
	this->components= components;
	pixels= NULL;
}

void Pixmap2D::init(int w, int h, int components){
	this->w= w;
	this->h= h;
	this->components= components;
	pixels= new uint8[h*w*components];
}

Pixmap2D::~Pixmap2D(){
	delete[] pixels;
}

void Pixmap2D::load(const string &path){
	string extension = toLower(path.substr(path.find_last_of('.') + 1));	
	if (extension == "bmp") {
		loadBmp(path);
	} else if (extension == "tga") {
		loadTga(path);
	} else if (extension == "png") {
		loadPng(path);
	} else {
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap2D::loadBmp(const string &path){
	PixmapIoBmp pib;
	pib.openRead(path);

	// init
	w = pib.getW();
	h = pib.getH();
	if (components == -1) {
		components = 3;
	}
	if (pixels == NULL) {
		pixels = new uint8[w * h * components];
	}

	//data
	pib.read(pixels, components);
}

void Pixmap2D::loadTga(const string &path){

	PixmapIoTga pit;
	pit.openRead(path);
	w = pit.getW();
	h = pit.getH();

	// header
	int fileComponents = pit.getComponents();

	// init
	if (components == -1) {
		components = fileComponents;
	}
	if (pixels == NULL) {
		pixels = new uint8[w * h * components];
	}

	// read data
	pit.read(pixels, components);
}

void Pixmap2D::loadPng(const string &path) {
	PixmapIoPng pip;
	pip.openRead(path);
	w = pip.getW();
	h = pip.getH();

	int fileComponents = pip.getComponents();

	//init
	if (components == -1) {
		components = fileComponents;
	}
	if (pixels == NULL) {
		pixels = new uint8[w * h * components];
	}

	// read data
	pip.read(pixels, components);
}

void Pixmap2D::save(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if (extension == "bmp") {
		saveBmp(path);
	} else if(extension == "tga") {
		saveTga(path);
	} else {
		throw runtime_error("Unknown pixmap extension: " + extension);
	}
}

void Pixmap2D::saveBmp(const string &path){
	PixmapIoBmp psb;
	psb.openWrite(path, w, h, components);
	psb.write(pixels);
}

void Pixmap2D::saveTga(const string &path){
	PixmapIoTga pst;
	pst.openWrite(path, w, h, components);
	pst.write(pixels);
}

void Pixmap2D::getPixel(int x, int y, uint8 *value) const{
	for(int i=0; i<components; ++i){
		value[i]= pixels[(w*y+x)*components+i];
	}
}

void Pixmap2D::getPixel(int x, int y, float32 *value) const{
	for(int i=0; i<components; ++i){
		value[i]= pixels[(w*y+x)*components+i]/255.f;
	}
}

//vector get
Vec4f Pixmap2D::getPixel4f(int x, int y) const{
	Vec4f v(0.f);
	for(int i=0; i<components && i<4; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

Vec3f Pixmap2D::getPixel3f(int x, int y) const{
	Vec3f v(0.f);
	for(int i=0; i<components && i<3; ++i){
		v.ptr()[i]= pixels[(w*y+x)*components+i]/255.f;
	}
	return v;
}

float Pixmap2D::getComponentf(int x, int y, int component) const{
	float c;
	getComponent(x, y, component, c);
	return c;
}

void Pixmap2D::setPixel(int x, int y, const uint8 *value){
	for(int i=0; i<components; ++i){
		pixels[(w*y+x)*components+i]= value[i];
	}
}

void Pixmap2D::setPixel(int x, int y, const float32 *value){
	for(int i=0; i<components; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(value[i]*255.f);
	}
}

//vector set
void Pixmap2D::setPixel(int x, int y, const Vec3f &p){
	for(int i=0; i<components  && i<3; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}

void Pixmap2D::setPixel(int x, int y, const Vec4f &p){
	for(int i=0; i<components && i<4; ++i){
		pixels[(w*y+x)*components+i]= static_cast<uint8>(p.ptr()[i]*255.f);
	}
}


void Pixmap2D::setPixels(const uint8 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}

void Pixmap2D::setPixels(const float32 *value){
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, value);
		}
	}
}

void Pixmap2D::setComponents(int component, uint8 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
}

void Pixmap2D::setComponents(int component, float32 value){
	assert(component<components);
	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setComponent(i, j, component, value);
		}
	}
}

float splatDist(Vec2i a, Vec2i b){
	return (max(abs(a.x-b.x),abs(a.y- b.y)) + 3.f*a.dist(b))/4.f;
}

void Pixmap2D::splat(const Pixmap2D *leftUp, const Pixmap2D *rightUp, const Pixmap2D *leftDown, const Pixmap2D *rightDown){

	Random random;

	assert(components==3 || components==4);

	if(
		!doDimensionsAgree(leftUp) ||
		!doDimensionsAgree(rightUp) ||
		!doDimensionsAgree(leftDown) ||
		!doDimensionsAgree(rightDown))
	{
		throw runtime_error("Pixmap2D::splat: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){

			float avg= (w+h)/2.f;

			float distLu= splatDist(Vec2i(i, j), Vec2i(0, 0));
			float distRu= splatDist(Vec2i(i, j), Vec2i(w, 0));
			float distLd= splatDist(Vec2i(i, j), Vec2i(0, h));
			float distRd= splatDist(Vec2i(i, j), Vec2i(w, h));

			const float powFactor= 2.0f;
			distLu= pow(distLu, powFactor);
			distRu= pow(distRu, powFactor);
			distLd= pow(distLd, powFactor);
			distRd= pow(distRd, powFactor);
			avg= pow(avg, powFactor);

			float lu= distLu>avg? 0: ((avg-distLu))*random.randRange(0.5f, 1.0f);
			float ru= distRu>avg? 0: ((avg-distRu))*random.randRange(0.5f, 1.0f);
			float ld= distLd>avg? 0: ((avg-distLd))*random.randRange(0.5f, 1.0f);
			float rd= distRd>avg? 0: ((avg-distRd))*random.randRange(0.5f, 1.0f);

			float total= lu+ru+ld+rd;

			Vec4f pix= (leftUp->getPixel4f(i, j)*lu+
				rightUp->getPixel4f(i, j)*ru+
				leftDown->getPixel4f(i, j)*ld+
				rightDown->getPixel4f(i, j)*rd)*(1.0f/total);

			setPixel(i, j, pix);
		}
	}
}

void Pixmap2D::lerp(float t, const Pixmap2D *pixmap1, const Pixmap2D *pixmap2){
	if(
		!doDimensionsAgree(pixmap1) ||
		!doDimensionsAgree(pixmap2))
	{
		throw runtime_error("Pixmap2D::lerp: pixmap dimensions don't agree");
	}

	for(int i=0; i<w; ++i){
		for(int j=0; j<h; ++j){
			setPixel(i, j, pixmap1->getPixel4f(i, j).lerp(t, pixmap2->getPixel4f(i, j)));
		}
	}
}

void Pixmap2D::copy(const Pixmap2D *sourcePixmap){

	assert(components==sourcePixmap->getComponents());

	if(w!=sourcePixmap->getW() || h!=sourcePixmap->getH()){
		throw runtime_error("Pixmap2D::copy() dimensions must agree");
	}
	memcpy(pixels, sourcePixmap->getPixels(), w*h*sourcePixmap->getComponents());
}

void Pixmap2D::subCopy(int x, int y, const Pixmap2D *sourcePixmap){
	assert(components==sourcePixmap->getComponents());

	if(w<sourcePixmap->getW() && h<sourcePixmap->getH()){
		throw runtime_error("Pixmap2D::subCopy(), bad dimensions");
	}

	uint8 *pixel= new uint8[components];

	for(int i=0; i<sourcePixmap->getW(); ++i){
		for(int j=0; j<sourcePixmap->getH(); ++j){
			sourcePixmap->getPixel(i, j, pixel);
			setPixel(i+x, j+y, pixel);
		}
	}

	delete pixel;
}

bool Pixmap2D::doDimensionsAgree(const Pixmap2D *pixmap){
	return pixmap->getW() == w && pixmap->getH() == h;
}

// =====================================================
//	class Pixmap3D
// =====================================================

Pixmap3D::Pixmap3D(){
	w= -1;
	h= -1;
	d= -1;
	components= -1;
}

Pixmap3D::Pixmap3D(int w, int h, int d, int components){
	init(w, h, d, components);
}

Pixmap3D::Pixmap3D(int d, int components){
	init(d, components);
}

void Pixmap3D::init(int w, int h, int d, int components){
	this->w= w;
	this->h= h;
	this->d= d;
	this->components= components;
	pixels= new uint8[h*w*d*components];;
}

void Pixmap3D::init(int d, int components){
	this->w= -1;
	this->h= -1;
	this->d= d;
	this->components= components;
	pixels= NULL;
}

Pixmap3D::~Pixmap3D(){
	delete[] pixels;
}

void Pixmap3D::loadSlice(const string &path, int slice){
	string extension= path.substr(path.find_last_of('.')+1);
	if(extension=="bmp"){
		loadSliceBmp(path, slice);
	}
	else if(extension=="tga"){
		loadSliceTga(path, slice);
	}
	else{
		throw runtime_error("Unknown pixmap extension: "+extension);
	}
}

void Pixmap3D::loadSliceBmp(const string &path, int slice){

	PixmapIoBmp plb;
	plb.openRead(path);

	//init
	w= plb.getW();
	h= plb.getH();
	if(components==-1){
		components= 3;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*d*components];
	}

	//data
	plb.read(&pixels[slice*w*h*components], components);
}

void Pixmap3D::loadSliceTga(const string &path, int slice){
	PixmapIoTga plt;
	plt.openRead(path);

	//header
	int fileComponents= plt.getComponents();

	//init
	w= plt.getW();
	h= plt.getH();
	if(components==-1){
		components= fileComponents;
	}
	if(pixels==NULL){
		pixels= new uint8[w*h*d*components];
	}

	//read data
	plt.read(&pixels[slice*w*h*components], components);
}

// =====================================================
//	class PixmapCube
// =====================================================

void PixmapCube::init(int w, int h, int components){
	for(int i=0; i<6; ++i){
		faces[i].init(w, h, components);
	}
}

	//load & save
void PixmapCube::loadFace(const string &path, int face){
	faces[face].load(path);
}

void PixmapCube::loadFaceBmp(const string &path, int face){
	faces[face].loadBmp(path);
}

void PixmapCube::loadFaceTga(const string &path, int face){
	faces[face].loadTga(path);
}

}}//end namespace
