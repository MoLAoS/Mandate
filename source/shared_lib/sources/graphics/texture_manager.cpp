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
#include "texture_manager.h"

#include <cstdlib>

#include "graphics_interface.h"
#include "graphics_factory.h"
#include "util.h"
#include "opengl.h"
#include "leak_dumper.h"

namespace Shared{ namespace Graphics{

using Gl::_assertGl;
using Util::cleanPath;
using Util::mediaErrorLog;

// =====================================================
//	class TextureManager
// =====================================================

TextureManager::TextureManager(){
	textureFilter= Texture::fBilinear;
	maxAnisotropy= 1;
}

TextureManager::~TextureManager(){
	end();
}

void TextureManager::init(){
	for (int i=0; i < TextureType::COUNT; ++i) {
		foreach (TextureContainer, it, textures[i]) {
			(*it)->init(textureFilter, maxAnisotropy);
		}
	}
}

void TextureManager::end(){
	for (int i=0; i < TextureType::COUNT; ++i) {
		foreach (TextureContainer, it, textures[i]) {
			(*it)->end();
			delete *it;
		}
		textures[i].clear();
	}
}

void TextureManager::setFilter(Texture::Filter textureFilter){
	this->textureFilter= textureFilter;
}

void TextureManager::setMaxAnisotropy(int maxAnisotropy){
	this->maxAnisotropy= maxAnisotropy;
}

Texture2D *TextureManager::getTexture(const string &path) {
	string cleanedPath = cleanPath(path);
	for (int i=0; i < textures[TextureType::TWO_D].size(); ++i) {
		if (textures[TextureType::TWO_D][i]->getPath() == cleanedPath) {
			return static_cast<Texture2D*>(textures[TextureType::TWO_D][i]);
		}
	}
	Texture2D *tex = GraphicsInterface::getInstance().getFactory()->newTexture2D();
	try {
		tex->load(cleanedPath);
		tex->init(textureFilter, maxAnisotropy);
		assertGl();
		tex->deletePixmap();
	} catch (runtime_error &e) {
		delete tex;
		mediaErrorLog.add(e.what(), path);
		return Texture2D::defaultTexture;
	}
	textures[TextureType::TWO_D].push_back(tex);
	return tex;
}

Texture1D *TextureManager::newTexture1D(){
	Texture1D *texture1D= GraphicsInterface::getInstance().getFactory()->newTexture1D();
	textures[TextureType::ONE_D].push_back(texture1D);

	return texture1D;
}

Texture2D *TextureManager::newTexture2D(){
	Texture2D *texture2D= GraphicsInterface::getInstance().getFactory()->newTexture2D();
	textures[TextureType::TWO_D].push_back(texture2D);

	return texture2D;
}

Texture3D *TextureManager::newTexture3D(){
	Texture3D *texture3D= GraphicsInterface::getInstance().getFactory()->newTexture3D();
	textures[TextureType::THREE_D].push_back(texture3D);

	return texture3D;
}

TextureCube *TextureManager::newTextureCube(){
	TextureCube *textureCube= GraphicsInterface::getInstance().getFactory()->newTextureCube();
	textures[TextureType::CUBE_MAP].push_back(textureCube);

	return textureCube;
}

bool TextureManager::deleteTexture2D(Texture2D *tex) {
	foreach (TextureContainer, it, textures[TextureType::TWO_D]) {
		if (*it == tex) {
			tex->end();
			delete tex;
			textures[TextureType::TWO_D].erase(it);
			return true;
		}
	}
	return false;
}

}}//end namespace
