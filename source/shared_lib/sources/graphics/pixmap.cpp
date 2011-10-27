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

#ifndef Z_DEFAULT_COMPRESSION
#   include "zlib.h"
#endif

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
void PixmapIoTga::openRead(FileOps *f) {
	file = f;

	//read header
	TargaFileHeader fileHeader;
	file->read(&fileHeader, sizeof(TargaFileHeader), 1);

	//check that we can load this tga file
	if(fileHeader.idLength!=0){
		throw runtime_error("id field is not 0");
	}

	if(fileHeader.dataTypeCode!=tgaUncompressedRgb && fileHeader.dataTypeCode!=tgaUncompressedBw){
		throw runtime_error("only uncompressed BW and RGB targa images are supported");
	}

	//check bits per pixel
	if(fileHeader.bitsPerPixel!=8 && fileHeader.bitsPerPixel!=24 && fileHeader.bitsPerPixel!=32){
		throw runtime_error("only 8, 24 and 32 bit targa images are supported");
	}

	h= fileHeader.height;
	w= fileHeader.width;
	components= fileHeader.bitsPerPixel/8;
}

void PixmapIoTga::openRead(const string &path){
	file = FSFactory::getInstance()->getFileOps();
	file->openRead(path.c_str());

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
	} else {
		char *buffer = new char[h*w*components];
		for(int i=0; i<h*w*components; i+=components){
			buffer[i] = pixels[i+2];
			buffer[i+1] = pixels[i+1];
			buffer[i+2] = pixels[i];
			if(components==4){
				buffer[i+3] = pixels[i+3];
			}
		}
		file->write(buffer, h * w * components, 1);
		delete [] buffer;
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
	//_PROFILE_FUNCTION();
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

static void user_write_data(png_structp write_ptr, png_bytep data, png_size_t length) {
	FileOps *f = ((FileOps*)png_get_io_ptr(write_ptr));
	if (f->write((void*)data, length, 1) != 1) {
		png_error(write_ptr,"Could not write to png-file");
	}
}

static void png_flush(png_structp png_ptr) {
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

	w = png_get_image_width(png_ptr, info_ptr);
	h = png_get_image_height(png_ptr, info_ptr);
	int color_type = png_get_color_type(png_ptr, info_ptr);
	int bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	// We want RGB, 24 bit
	if (color_type == PNG_COLOR_TYPE_PALETTE || (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
	|| png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) {
		png_set_expand(png_ptr);
	}

	if (color_type == PNG_COLOR_TYPE_GRAY) {
		png_set_gray_to_rgb(png_ptr);
	}
	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	components = png_get_rowbytes(png_ptr, info_ptr) / w;

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
	const int rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	for (int y = 0; y < h; ++y) {
		row_pointers[y] = new png_byte[rowbytes];
	}
	png_read_image(png_ptr, row_pointers);

	// Copy image
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

void PixmapIoPng::openWrite(const string &path, int w, int h, int components) {
    //this->path = path;
	this->w= w;
	this->h= h;
	this->components= components;

	//file= fopen(path.c_str(),"wb");
	file = FSFactory::getInstance()->getFileOps();
	file->openWrite(path.c_str());
}

void PixmapIoPng::write(uint8 *pixels) {
	// Thanks to MegaGlest for most of this implementation.

	// Allocate write & info structures
   png_structp png_write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if(!png_write_ptr) {
	 //fclose(file); ///@todo replace with physfs equivalent
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - out of memory creating write structure");
   }

   png_infop info_ptr = png_create_info_struct(png_write_ptr);
   if(!info_ptr) {
	 png_destroy_write_struct(&png_write_ptr,
							  (png_infopp)NULL);
	 //fclose(file); ///@todo replace with physfs equivalent
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - out of memery creating info structure");
   }

   // setjmp() must be called in every function that calls a PNG-writing
   // libpng function, unless an alternate error handler was installed--
   // but compatible error handlers must either use longjmp() themselves
   // (as in this program) or exit immediately, so here we go:

   if(setjmp(png_jmpbuf(png_write_ptr))) {
	 png_destroy_write_struct(&png_write_ptr, &info_ptr);
	 //fclose(file); ///@todo replace with physfs equivalent
	 throw runtime_error("OpenGlDevice::saveImageAsPNG() - setjmp problem");
   }

   png_set_write_fn(png_write_ptr, file, user_write_data, png_flush);

   // make sure outfile is (re)opened in BINARY mode
   //png_init_io(png_ptr, file); // using custom write through PhysFS instead

   // set the compression levels--in general, always want to leave filtering
   // turned on (except for palette images) and allow all of the filters,
   // which is the default; want 32K zlib window, unless entire image buffer
   // is 16K or smaller (unknown here)--also the default; usually want max
   // compression (NOT the default); and remaining compression flags should
   // be left alone

   //png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
   png_set_compression_level(png_write_ptr, Z_DEFAULT_COMPRESSION);

   //
   // this is default for no filtering; Z_FILTERED is default otherwise:
   // png_set_compression_strategy(png_ptr, Z_DEFAULT_STRATEGY);
   //  these are all defaults:
   //   png_set_compression_mem_level(png_ptr, 8);
   //   png_set_compression_window_bits(png_ptr, 15);
   //   png_set_compression_method(png_ptr, 8);


   // Set some options: color_type, interlace_type
   int color_type=0, interlace_type=0, numChannels=0;

   //  color_type = PNG_COLOR_TYPE_GRAY;
   //  color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
   color_type = PNG_COLOR_TYPE_RGB;
   numChannels = 3;
   // color_type = PNG_COLOR_TYPE_RGB_ALPHA;

   interlace_type =  PNG_INTERLACE_NONE;
   // interlace_type = PNG_INTERLACE_ADAM7;

   int bit_depth = 8;
   png_set_IHDR(png_write_ptr, info_ptr, this->w, this->h, bit_depth,
				color_type,
				interlace_type,
				PNG_COMPRESSION_TYPE_BASE,
				PNG_FILTER_TYPE_BASE);

   // Optional gamma chunk is strongly suggested if you have any guess
   // as to the correct gamma of the image. (we don't have a guess)
   //
   // png_set_gAMA(png_write_ptr, info_ptr, image_gamma);

   // write all chunks up to (but not including) first IDAT
   png_write_info(png_write_ptr, info_ptr);

   // set up the row pointers for the image so we can use png_write_image

   png_bytep* row_pointers = new png_bytep[this->h];
   if (row_pointers == 0) {
	 png_destroy_write_struct(&png_write_ptr, &info_ptr);
	 //fclose(file); ///@todo replace with physfs equivalent
	 throw runtime_error("OpenGlDevice::failed to allocate memory for row pointers");
   }

   unsigned int row_stride = this->w * numChannels;
   unsigned char *rowptr = (unsigned char*) pixels;
   for (int row = this->h-1; row >=0 ; row--) {
	 row_pointers[row] = rowptr;
	 rowptr += row_stride;
   }

   // now we just write the whole image; libpng takes care of interlacing for us
   png_write_image(png_write_ptr, row_pointers);

   // since that's it, we also close out the end of the PNG file now--if we
   // had any text or time info to write after the IDATs, second argument
   // would be info_ptr, but we optimize slightly by sending NULL pointer: */

   png_write_end(png_write_ptr, info_ptr);

   //
   // clean up after the write
   //    free any memory allocated & close the file
   //
   png_destroy_write_struct(&png_write_ptr, &info_ptr);

   delete [] row_pointers;
	//fclose(file);
}

// =====================================================
//	class PixmapIoJpg
// =====================================================

// *** Methods used for JPG-Decompression (jpeglib) ***

static void init_source(j_decompress_ptr cinfo) {
	// already initialized
}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
	// already initialized
	return true;
}

static void skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	if (num_bytes > 0) {
		jpeg_source_mgr* This = cinfo->src;
		This->bytes_in_buffer-= num_bytes;
		This->next_input_byte+= num_bytes;
	}
}

static void term_source(j_decompress_ptr cinfo) {
}

// ***

void PixmapIoJpg::openRead(const string &path) {
	file = FSFactory::getInstance()->getFileOps();
	file->openRead(path.c_str());

	size_t length = file->fileSize();
	buffer = new uint8[length];
	file->read(buffer, length, 1);

	//Check buffer (weak jpeg check)
	// Proper header check found from: http://www.fastgraph.com/help/jpeg_header_format.html
	if (buffer[0] != 0xFF || buffer[1] != 0xD8) {
	    std::cout << "0 = [" << std::hex << (int)buffer[0] << "] 1 = [" << std::hex << (int)buffer[1] << "]" << std::endl;
		delete[] buffer;
		throw runtime_error("JPG header check failed");
	}

	cinfo.err = jpeg_std_error(&jerr); //Standard error handler
	jpeg_create_decompress(&cinfo); //Create decompressing structure
	
	jmp_buf error_buffer; //Used for saving/restoring context

	if (setjmp(error_buffer)) { //Longjump was called --> an exception was thrown
		delete[] buffer;
		jpeg_destroy_decompress(&cinfo);
		throw runtime_error("JPG failed setjump");
	}

	// Set up data pointer
	source.bytes_in_buffer = length;
	source.next_input_byte = (JOCTET*)buffer;
	source.init_source = init_source;
	source.fill_input_buffer = fill_input_buffer;
	source.resync_to_restart = jpeg_resync_to_restart;
	source.skip_input_data = skip_input_data;
	source.term_source = term_source;

	cinfo.src = &source;

	/* reading the image header which contains image information */
	if (jpeg_read_header(&cinfo, TRUE) != JPEG_HEADER_OK) {
		delete[] buffer;
		jpeg_destroy_decompress(&cinfo);
		throw runtime_error("JPG failed reading header");
	}

	components = cinfo.num_components;
	w = cinfo.image_width;
	h = cinfo.image_height;
}

void PixmapIoJpg::read(uint8 *pixels) {
	read(pixels, 3);
}

void PixmapIoJpg::read(uint8 *pixels, int components) { 
	//TODO: Irrlicht has some special CMYK-handling - maybe needed too?

	/* Start decompression jpeg here */
	jpeg_start_decompress(&cinfo);
	
	/* now actually read the jpeg into the raw buffer */
	JSAMPROW row_pointer[1];
	row_pointer[0] = new unsigned char[cinfo.output_width * this->components];
	size_t location = 0; //Current pixel
	
	/* read one scan line at a time */
	/* Again you need to invert the lines unfortunately*/
	while (cinfo.output_scanline < h) {
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		location = (cinfo.output_height - cinfo.output_scanline) * cinfo.output_width * components;

		if (components == this->components) {
			memcpy(pixels + location, row_pointer[0], cinfo.output_width * components);
		} else {
			int r,g,b,a,l;
			for (int xPic = 0, xFile = 0; xFile < cinfo.output_width * components; xPic += components, xFile += this->components) {
				switch(this->components) {
					case 1:
						r = g = b = l = row_pointer[0][xFile];
						a = 255;
						break;
					case 3:
						r = row_pointer[0][xFile];
						g = row_pointer[0][xFile+1];
						b = row_pointer[0][xFile+2];
						l = (r+g+b+2)/3;
						a = 255;
						break;
					case 4:
						r = row_pointer[0][xFile];
						g = row_pointer[0][xFile+1];
						b = row_pointer[0][xFile+2];
						l = (r+g+b+2)/3;
						a = row_pointer[0][xFile+3];
						break;
					default:
						throw runtime_error("JPG file has invalid number of colour channels");
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
						throw runtime_error("PixmapIoJpg request for invalid number of colour channels");
				}
			}
		}
	}

	/* wrap up decompression, destroy objects, free pointers and close open files */
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	delete[] row_pointer[0];
	delete[] buffer;
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

void Pixmap1D::dispose() {
	delete [] pixels;
	pixels = 0;
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

Pixmap2D::Pixmap2D() : h(-1), w(-1), components(-1), pixels(0) {
}

Pixmap2D::Pixmap2D(int components) : h(-1), w(-1), components(-1), pixels(0) {
	init(components);
}

Pixmap2D::Pixmap2D(int w, int h, int components) : h(-1), w(-1), components(-1), pixels(0) {
	init(w, h, components);
}

void Pixmap2D::init(int components){
	if (pixels) {
		delete [] pixels; // in case init is called twice
	}
	this->w = -1;
	this->h = -1;
	this->components = components;
	pixels = 0;
}

void Pixmap2D::init(int w, int h, int components){
	if (pixels) {
		delete[] pixels; // in case init is called twice
	}
	this->w= w;
	this->h= h;
	this->components= components;
	pixels = new uint8[h*w*components];
}

void Pixmap2D::dispose() {
	delete [] pixels;
	pixels = 0;
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
	} else if (extension == "jpg") {
		loadJpg(path);
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

void Pixmap2D::loadTga(FileOps *f) {
	PixmapIoTga pit;
	pit.openRead(f);
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

void Pixmap2D::loadJpg(const string &path) {
	PixmapIoJpg pij;
	pij.openRead(path);
	w = pij.getW();
	h = pij.getH();

	int fileComponents = pij.getComponents();

	//init
	if (components == -1) {
		components = fileComponents;
	}
	if (pixels == NULL) {
		pixels = new uint8[w * h * components];
	}

	// read data
	pij.read(pixels, components);
}

void Pixmap2D::save(const string &path){
	string extension= path.substr(path.find_last_of('.')+1);
	if (extension == "bmp") {
		saveBmp(path);
	} else if(extension == "tga") {
		saveTga(path);
	} else if(extension == "png") {
		savePng(path);
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

void Pixmap2D::savePng(const string &path){
	PixmapIoPng psp;
	psp.openWrite(path, w, h, components);
	psp.write(pixels);
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

/** Copy a piece of the source pixmap to become this pixmap
  * @param x x position of top left corner
  * @param y y position of top left corner
  * @param width width of a square
  * @param height height of a square
  * @param source the source pixmap
  */
void Pixmap2D::copy(int x, int y, int width, int height, const Pixmap2D *source) {
	assert(source != this);
	// check piece is inside source pixmap
	if(source->getW() < (x + width) || source->getH() < (y + height) || x < 0 || y < 0) {
		throw runtime_error("Pixmap2D::getPiece(), bad dimensions");
	}

	// resize this pixmap to fit the dimensions of the piece
	init(width, height, source->getComponents());

	///@note there might be potential for optimisation in the iteration order - hailstone 30Dec2010
	// copy pixels
	uint8 *pixel = new uint8[source->getComponents()];
	for (int j = 0; j < h; ++j) {
		for (int i = 0; i < w; ++i) {
			source->getPixel(i+x, j+y, pixel);
			setPixel(i, j, pixel);
		}
	}

	delete [] pixel;
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

	delete [] pixel;
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

void Pixmap3D::dispose() {
	delete [] pixels;
	pixels = 0;
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

void PixmapCube::dispose() {
	for(int i=0; i<6; ++i){
		faces[i].dispose();
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
