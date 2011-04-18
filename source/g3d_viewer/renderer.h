#ifndef _G3DVIEWER_RENDERER_H_
#define _G3DVIEWER_RENDERER_H_

#include "model_renderer.h"
#include "texture_manager.h"
#include "model.h"
#include "texture.h"

namespace Shared { namespace G3dViewer {

using namespace Shared::Graphics;

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
	// start window pos and size
	static const int windowX= 100;
	static const int windowY= 75;
	static const int windowW= 800;
	static const int windowH= 600;

private:
	bool wireframe;
	bool normals;
	bool grid;
	int mesh;

	ModelRenderer *modelRenderer;
	TextureManager *textureManager;
	Texture2D *customTextures[8];
	Vec3<uint8> colours[8];
	MeshCallbackTeamColor meshCallbackTeamColor;

	Renderer();


public:
	~Renderer();
	static Renderer *getInstance();

	TextureManager *getTextureManager() { return textureManager; }

	void init();
	void reset(int x, int y, int w, int h, int playerColor);
	void transform(float rotX, float rotY, float zoom);
	void renderGrid();
	void resetTeamTexture(int i, uint8 r, uint8 g, uint8 b);

	bool getNormals() const		{return normals;}
	bool getWireframe() const	{return wireframe;}
	bool getGrid() const		{return grid;}
	int  getMesh() const		{return mesh;}

	void toggleNormals();
	void toggleWireframe();
	void toggleGrid();
	void setMesh(int index = -1) { mesh = index; }

	Vec3<uint8> getTeamColour(int i) const	{ return colours[i]; }

	void loadTheModel(Model *model, string file);
	void renderTheModel(Model *model, float f);
};

}}//end namespace

#endif
