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

#ifndef _SHARED_GRAPHICS_GRAPHICSFACTORY_H_
#define _SHARED_GRAPHICS_GRAPHICSFACTORY_H_

#include <cstdlib>

namespace Shared{ namespace Graphics{

class Context;

class TextureManager;
class Texture1D;
class Texture2D;
class Texture3D;
class TextureCube;
	
class ModelManager;
class ModelRenderer;
class Model;

class FontManager;
class TextRenderer;
class TextRenderer3D;
class Font;
class Font3D;

class ParticleManager;
class ParticleRenderer;

// =====================================================
//	class GraphicsFactory
// =====================================================

class GraphicsFactory{
public:
	virtual ~GraphicsFactory(){}

	//context
	virtual Context *newContext()					{return NULL;}

	//textures
	virtual TextureManager *newTextureManager()		{return NULL;}
	virtual Texture1D *newTexture1D()				{return NULL;}
	virtual Texture2D *newTexture2D()				{return NULL;}
	virtual Texture3D *newTexture3D()				{return NULL;}
	virtual TextureCube *newTextureCube()			{return NULL;}
	
	//models
	virtual ModelManager *newModelManager()			{return NULL;}
	virtual ModelRenderer *newModelRenderer()		{return NULL;}
	virtual Model *newModel()						{return NULL;}

	//text
	virtual FontManager *newFontManager()			{return NULL;}
	virtual TextRenderer *newTextRendererFT()		{return NULL;}
	virtual Font *newFreeTypeFont()					{return NULL;}

	//particles
	virtual ParticleManager *newParticleManager()	{return NULL;}
	virtual ParticleRenderer *newParticleRenderer()	{return NULL;}
};

}}//end namespace

#endif
