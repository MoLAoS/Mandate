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
#include "texture_gl.h"

#include <stdexcept>

#include "opengl.h"
#include "math_util.h"

#include "leak_dumper.h"

namespace Shared { namespace Graphics { namespace Gl {

using namespace Platform;

bool TextureGl::compressTextures = true;

GLint toWrapModeGl(Texture::WrapMode wrapMode) {
	switch (wrapMode) {
		case Texture::wmClamp:
			return GL_CLAMP;
		case Texture::wmRepeat:
			return GL_REPEAT;
		case Texture::wmClampToEdge:
			return GL_CLAMP_TO_EDGE;
		default:
			assert(false);
			return GL_CLAMP;
	}
}

GLint toFormatGl(Texture::Format format, int components) {
	switch (format) {
		case Texture::fAuto:
			switch (components) {
				case 1:
					return GL_LUMINANCE;
				case 3:
					return GL_RGB;
				case 4:
					return GL_RGBA;
				default:
					assert(false);
					return GL_RGBA;
			}
			break;
		case Texture::fLuminance:
			return GL_LUMINANCE;
		case Texture::fAlpha:
			return GL_ALPHA;
		case Texture::fRgb:
			return GL_RGB;
		case Texture::fRgba:
			return GL_RGBA;
		default:
			assert(false);
			return GL_RGB;
	}
}

GLint toInternalFormatGl(Texture::Format format, int components) {
	switch(format){
		case Texture::fAuto:
			switch (components) {
				case 1:
					return GL_COMPRESSED_LUMINANCE;
				case 3:
					return GL_COMPRESSED_RGB;
				case 4:
					return GL_COMPRESSED_RGBA;
				default:
					assert(false);
					return GL_COMPRESSED_RGBA;
			}
			break;
		case Texture::fLuminance:
			return GL_COMPRESSED_LUMINANCE;
		case Texture::fAlpha:
			return GL_COMPRESSED_ALPHA;
		case Texture::fRgb:
			return GL_COMPRESSED_RGB;
		case Texture::fRgba:
			return GL_COMPRESSED_RGBA;
		default:
			assert(false);
			return GL_COMPRESSED_RGB;
	}
}

// =====================================================
//	class Texture1DGl
// =====================================================

void Texture1DGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if (!inited) {
		// texture params
		GLint wrap = toWrapModeGl(wrapMode);
		GLint glFormat = toFormatGl(format, pixmap->getComponents());
		GLint glInternalFormat = toInternalFormatGl(format, pixmap->getComponents());

		// pixel init var
		const uint8* pixels = pixmap->getPixels();

		// gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_1D, handle);

		// wrap params
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, wrap);

		// maxAnisotropy
		if (isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if (mipmap) {
			GLuint glFilter= filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;

			// build mipmaps
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
			int error = gluBuild1DMipmaps(GL_TEXTURE_1D, glInternalFormat, 
						pixmap->getW(), glFormat, GL_UNSIGNED_BYTE, pixels);

			if (error != GL_NO_ERROR) {
				string msg = (char*)gluErrorString(error);
				stringstream ss;
				ss	<< "Error building 1D mipmaps: " << msg << endl 
					<< "Format: " << format << ", components: " << pixmap->getComponents() << endl
					<< "Width: " << pixmap->getW() << endl;
				throw runtime_error(ss.str());
			}
		} else {
			// build single texture
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage1D(GL_TEXTURE_1D, 0, glInternalFormat, 
					pixmap->getW(), 0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error = glGetError();
			if (error != GL_NO_ERROR) {
				string msg = (char*)gluErrorString(error);
				stringstream ss;
				ss	<< "Error sending pixmap data to GL: " << msg << endl 
					<< "Format: " << format << ", components: " << pixmap->getComponents() << endl
					<< "Width: " << pixmap->getW() << endl;
				throw runtime_error(ss.str());
			}
		}
		inited = true;
	}

	assertGl();
}

void Texture1DGl::deletePixmap() {
	delete pixmap;
	pixmap = 0;
}

void Texture1DGl::end() {
	if (inited) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class Texture2DGl
// =====================================================

void Texture2DGl::init(Filter filter, int maxAnisotropy) {
	if (!inited) {
		if (!isPowerOfTwo(pixmap->getW()) || !isPowerOfTwo(pixmap->getH())) {
			throw runtime_error("Texture dimensions are not both a power of two.");
		}
		//params
		GLint wrap = toWrapModeGl(wrapMode);
		GLint glFormat = toFormatGl(format, pixmap->getComponents());
		GLint glInternalFormat = toInternalFormatGl(format, pixmap->getComponents());

		//pixel init var
		const uint8* pixels = pixmap->getPixels();

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

		//maxAnisotropy
		if (isGlExtensionSupported("GL_EXT_texture_filter_anisotropic")) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy);
		}

		if (mipmap) {
			GLuint glFilter = filter == fTrilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;

			// build mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
			int error = gluBuild2DMipmaps(GL_TEXTURE_2D, glInternalFormat,
				pixmap->getW(), pixmap->getH(), glFormat, GL_UNSIGNED_BYTE, pixels);

			if (error != 0) {
				string msg = (char*)gluErrorString(error);
				stringstream ss;
				ss	<< "Error building texture 2D mipmaps: " << msg << endl 
					<< "Format: " << format << ", components: " << pixmap->getComponents() << endl
					<< "Width: " << pixmap->getW() << ", height: " << pixmap->getH() << endl;
				throw runtime_error(ss.str());
			}
		} else {
			// build single texture
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, glInternalFormat,
				pixmap->getW(), pixmap->getH(), 0, glFormat, GL_UNSIGNED_BYTE, pixels);

			GLint error = glGetError();
			if (error != GL_NO_ERROR) {
				string msg = (char*)gluErrorString(error);
				stringstream ss;
				ss	<< "Error sending pixmap data to GL: " << msg << endl 
					<< "Format: " << format << ", components: " << pixmap->getComponents() << endl
					<< "Width: " << pixmap->getW() << ", height: " << pixmap->getH() << endl;
				throw runtime_error(ss.str());
			}
		}
		inited= true;
	}
	assertGl();
}

void Texture2DGl::deletePixmap() {
	delete pixmap;
	pixmap = 0;
}

void Texture2DGl::end() {
	if (inited) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class Texture3DGl
// =====================================================

void Texture3DGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if (!inited) {

		//params
		GLint wrap = toWrapModeGl(wrapMode);
		GLint glFormat = toFormatGl(format, pixmap->getComponents());
		GLint glInternalFormat = toInternalFormatGl(format, pixmap->getComponents());

		//pixel init var
		const uint8* pixels = pixmap->getPixels();

		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_3D, handle);

		//wrap params
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, wrap);

		//build single texture
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
		glTexImage3D(GL_TEXTURE_3D, 0, glInternalFormat, 
			pixmap->getW(), pixmap->getH(), pixmap->getD(), 0, glFormat, GL_UNSIGNED_BYTE, pixels);

		GLint error = glGetError();
		if (error != GL_NO_ERROR) {
			string msg = (char*)gluErrorString(error);
			stringstream ss;
			ss	<< "Error sending pixmap data to GL: " << msg << endl 
				<< "Format: " << format << ", components: " << pixmap->getComponents() << endl
				<< "Width: " << pixmap->getW() << ", height: " << pixmap->getH()
				<< ", Depth: " << pixmap->getD() << endl;
			throw runtime_error(ss.str());
		}
		inited = true;
	}

	assertGl();
}

void Texture3DGl::deletePixmap() {
	delete pixmap;
	pixmap = 0;
}

void Texture3DGl::end() {
	if (inited) {
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

// =====================================================
//	class TextureCubeGl
// =====================================================

void TextureCubeGl::init(Filter filter, int maxAnisotropy) {
	assertGl();

	if (!inited) {
		//gen texture
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

		// wrap
		GLint wrap = toWrapModeGl(wrapMode);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, wrap);

		// filter
		if (mipmap) {
			GLuint glFilter = filter==fTrilinear? GL_LINEAR_MIPMAP_LINEAR: GL_LINEAR_MIPMAP_NEAREST;
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, glFilter);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		} else {
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		for (int i=0; i < 6; ++i) {
			//params
			const Pixmap2D *currentPixmap = pixmap->getFace(i);

			GLint glFormat= toFormatGl(format, currentPixmap->getComponents());
			GLint glInternalFormat= toInternalFormatGl(format, currentPixmap->getComponents());

			//pixel init var
			const uint8* pixels = currentPixmap->getPixels();
			GLenum target= GL_TEXTURE_CUBE_MAP_POSITIVE_X + i;

			if (mipmap) {
				glHint(GL_TEXTURE_COMPRESSION_HINT, GL_FASTEST);
				int error = gluBuild2DMipmaps(target, glInternalFormat,
					currentPixmap->getW(), currentPixmap->getH(), glFormat, GL_UNSIGNED_BYTE, pixels);

				if (error != 0) {
					string msg = (char*)gluErrorString(error);
					stringstream ss;
					ss	<< "Error building TextureCube mipmaps: " << msg << endl 
						<< "Format: " << format << ", components: " << currentPixmap->getComponents() << endl
						<< "Width: " << currentPixmap->getW() << ", Height: " << currentPixmap->getH();
					throw runtime_error(ss.str());
				}
			} else {
				glTexImage2D(target, 0, glInternalFormat,
					currentPixmap->getW(), currentPixmap->getH(), 0, glFormat, GL_UNSIGNED_BYTE, pixels);

				int error = glGetError();
				if (error != GL_NO_ERROR) {
					string msg = (char*)gluErrorString(error);
					stringstream ss;
					ss	<< "Error creating texture cube: " << msg << endl 
						<< "Format: " << format << ", components: " << currentPixmap->getComponents() << endl
						<< "Width: " << currentPixmap->getW() << ", Height: " << currentPixmap->getH();
					throw runtime_error(ss.str());
				}
			}
		}
		inited= true;

	}

	assertGl();
}

void TextureCubeGl::deletePixmap() {
	delete pixmap;
	pixmap = 0;
}

void TextureCubeGl::end(){
	if(inited){
		assertGl();
		glDeleteTextures(1, &handle);
		assertGl();
	}
}

}}}//end namespace
