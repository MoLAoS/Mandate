#ifndef _SHADER_G3DVIEWER_RENDERER_H_
#define _SHADER_G3DVIEWER_RENDERER_H_

#include "model_renderer.h"
#include "texture_manager.h"
#include "model.h"
#include "texture.h"

using Shared::Graphics::ModelRenderer;
using Shared::Graphics::TextureManager;
using Shared::Graphics::Model;
using Shared::Graphics::Texture2D;

#include "model_renderer.h"

using Shared::Graphics::MeshCallback;
using Shared::Graphics::Mesh;
using Shared::Graphics::Texture;

namespace Shared{ namespace G3dViewer{

// ===============================================
// 	class MeshCallbackTeamColor
// ===============================================

class MeshCallbackTeamColor: public MeshCallback{
private:
	const Texture *teamTexture;

public:
	void setTeamTexture(const Texture *teamTexture)	{this->teamTexture= teamTexture;}
	virtual void execute(const Mesh *mesh);
};

// ===============================
// 	class Renderer  
// ===============================

class Renderer{
public:
	static const int windowX= 100;
	static const int windowY= 75;
	static const int windowW= 800;
	static const int windowH= 600;

private:
	bool wireframe;
	bool normals;
	bool grid;

	ModelRenderer *modelRenderer;
	TextureManager *textureManager;
	Texture2D *customTextures[4];
	MeshCallbackTeamColor meshCallbackTeamColor;

	Renderer();


public:
	~Renderer();
	static Renderer *getInstance();

	void init();
	void reset(int x, int y, int w, int h, int playerColor);
	void transform(float rotX, float rotY, float zoom);
	void renderGrid();
	void resetTeamTexture(int i, uint8 r, uint8 g, uint8 b);

	bool getNormals() const		{return normals;}
	bool getWireframe() const	{return wireframe;}
	bool getGrid() const		{return grid;}

	void toggleNormals();
	void toggleWireframe();
	void toggleGrid();

	void loadTheModel(Model *model, string file);
	void renderTheModel(Model *model, float f);

	uint8 colours[4*3];
};

}}//end namespace

#endif
