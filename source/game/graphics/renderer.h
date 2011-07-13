// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_RENDERER_H_
#define _GLEST_GAME_RENDERER_H_

// shared_lib
#include "vec.h"
#include "math_util.h"
#include "model.h"
#include "particle.h"
#include "pixmap.h"
#include "font.h"
#include "matrix.h"
#include "texture.h"
#include "model_manager.h"
#include "graphics_factory_gl.h"
#include "font_manager.h"
#include "camera.h"
#include "profiler.h"

// game
#include "selection.h"
#include "components.h"

#include "scene_culler.h"
#include "terrain_renderer.h"

using namespace Shared::Math;
using namespace Shared::Graphics;
using namespace Glest::Global;
using namespace Glest::Gui;
using namespace Glest::Menu;

namespace Glest {
	namespace Entities { class MapObject; }

namespace Graphics {

WRAPPED_ENUM( ResourceScope, GLOBAL, MENU, GAME );
WRAPPED_ENUM( ShadowMode, DISABLED, PROJECTED, MAPPED );
WRAPPED_ENUM( TeamColourMode, DISABLED, OUTLINE, TINT, BOTH );

// ===========================================================
// 	class Renderer
//
///	OpenGL renderer, uses the shared library
// ===========================================================

class Renderer{
	IF_DEBUG_EDITION( friend class Script::ScriptManager; )
public:
	//progress bar
	static const int maxProgressBar;
	static const Vec4f progressBarBack1;
	static const Vec4f progressBarBack2;
	static const Vec4f progressBarFront1;
	static const Vec4f progressBarFront2;

	//sun and moon
	static const float sunDist;
	static const float moonDist;
	static const float lightAmbFactor;

	//mouse
	static const int maxMouse2dAnim;

	//texture units
	static const GLenum baseTexUnit;
	static const GLenum fowTexUnit;
	static const GLenum shadowTexUnit;

	//selection
	static const float selectionCircleRadius;
	static const float magicCircleRadius;

	//default values
	static const float ambFactor;
	static const Vec4f defSpecularColor;
	static const Vec4f defDiffuseColor;
	static const Vec4f defAmbientColor;
	static const Vec4f defColor;
	static const Vec4f fowColor;

	//light
	static const float maxLightDist;

private:
	// config
	int maxLights;
    bool photoMode;
	int shadowTextureSize;
	int shadowFrameSkip;
	float shadowAlpha;
	bool focusArrows;
	bool textures3D;
	ShadowMode m_shadowMode;

	// game
	const GameState *game;
	MainMenu *m_mainMenu;

	// misc
	int triangleCount;
	int pointCount;
	Vec4f nearestLightPos;
	TeamColourMode m_teamColourMode;

	// renderers
	ModelRenderer *modelRenderer;
	TextRenderer *textRendererFT;
	ParticleRenderer *particleRenderer;

	// texture managers
	ModelManager *modelManager[ResourceScope::COUNT];
	TextureManager *textureManager[ResourceScope::COUNT];
	FontManager *fontManager[ResourceScope::COUNT];
	ParticleManager *particleManager[ResourceScope::COUNT];

	// display list handles
	GLuint list3d;
	GLuint list2d;
	GLuint list3dMenu;
	GLuint list3dGLSL;

	// shadows
	GLuint shadowMapHandle;
	Matrix4f shadowMapMatrix;
	int shadowMapFrame;

	// water
	float waterAnim;
	
	// perspective values
	float perspFov;
	float perspNearPlane;
	float perspFarPlane;

	// helper object, determines visible scene
	SceneCuller culler;

	TerrainRenderer *m_terrainRenderer;

private:
	Renderer();
	~Renderer();

public:
	static Renderer &getInstance();

    //init
	bool init();
	void initGame(GameState *game);
	void initMenu(MainMenu *mm);
	void reset3d();
	void reset2d();
	void reset3dMenu();
	void resetGlLists();

	// end
	void end();
	void endMenu();
	void endGame();

	// get
	int getTriangleCount() const	{return triangleCount;}
	int getPointCount() const		{return pointCount;}
	ShadowMode getShadowMode() const {return m_shadowMode;}
	GLuint getShadowMapHandle() const { return shadowMapHandle;}

	// inc tri/point counters
	void incTriangleCount(int n=1) { triangleCount += n;}
	void incPointCount(int n=1) { pointCount += n;}

	const SceneCuller& getCuller() const { return culler; }
	void setFarClip(float clip) { perspFarPlane = clip; }

	// misc
	void reloadResources();

	void changeShader(const string &name);
	void cycleShaders();

	TeamColourMode getTeamColourMode() const { return m_teamColourMode; }
	void setTeamColourMode(TeamColourMode v) { m_teamColourMode = v; }

	// engine interface
	Model *newModel(ResourceScope rs);
	Texture2D *getTexture2D(ResourceScope rs, const string &path);
	Texture2D *newTexture2D(ResourceScope rs);
	Texture3D *newTexture3D(ResourceScope rs);

	void deleteTexture2D(Texture2D *tex, ResourceScope rs);

	//Font *newFont(ResourceScope rs);
	Font *newFreeTypeFont(ResourceScope rs);
	
	//TextRenderer *getTextRenderer() const	{return textRenderer;}
	TextRenderer *getFreeTypeRenderer() const	{return textRendererFT;}

	TerrainRenderer* getTerrainRenderer() { return m_terrainRenderer; }

	void manageParticleSystem(ParticleSystem *particleSystem, ResourceScope rs);
	void updateParticleManager(ResourceScope rs);
	void renderParticleManager(ResourceScope rs);
	void swapBuffers();

	ParticleManager* getParticleManager() { return particleManager[ResourceScope::GAME]; }

	//lights and camera
	void setupLighting();
	void loadGameCameraMatrix();
	void loadCameraMatrix(const Camera *camera);
	
	void computeVisibleArea();

    //basic rendering
    void renderMouse3d();
    void renderBackground(const Texture2D *texture);
	void renderTextureQuad(int x, int y, int w, int h, const Texture2D *texture, float alpha=1.f);
	void renderSelectionQuad();
	//void renderText(const string &text, const Font *font, float alpha, int x, int y, bool centered= false);
	//void renderText(const string &text, const Font *font, const Vec3f &color, int x, int y, bool centered= false);
	//void renderTextShadow(const string &text, const Font *font, int x, int y, bool centered = false, Vec3f colour = Vec3f(1.f));

	void renderText(const string &text, const Font *font, const Vec4f &color, int x, int y);
	void renderProgressBar(int size, int x, int y, int w, int h, const Font *font);

    //complex rendering
	void renderSurface()	{m_terrainRenderer->render(culler);}
	void renderObjects();
	void renderWater();
    void renderUnits();
	void renderSelectionEffects();
	void renderWaterEffects();
	void renderMenuBackground(const MenuBackground *menuBackground);

	//computing
    bool computePosition(const Vec2i &screenPos, Vec2i &worldPos);
	void computeSelected(UnitVector &units, const MapObject *&obj, const Vec2i &posDown, const Vec2i &posUp);

    //gl wrap
	string getGlInfo();
	string getGlMoreInfo();
	string getGlMoreInfo2();
	void autoConfig();

	//clear
    void clearBuffers();
	void clearZBuffer();

	//shadows
	void renderShadowsToTexture();

	//misc
	void loadConfig();
	void saveScreen(const string &path);

	void loadProjectionMatrix();
	void enableProjectiveTexturing();

	//static
	static ShadowMode strToShadows(const string &s);
	static string shadowsToStr(ShadowMode shadows);

private:
	//private misc
	float computeSunAngle(float time);
	float computeMoonAngle(float time);
	Vec4f computeSunPos(float time);
	Vec4f computeMoonPos(float time);
	Vec3f computeLightColor(float time);
	Vec4f computeWaterColor(float waterLevel, float cellHeight);
	void checkExtension(const string &extension, const string &msg);

	// selection or shadows render
	void renderObjectsFast(bool shadows = false);
	void renderUnitsFast(bool renderingShadows = false);

	//gl requirements
	void checkGlCaps();
	void checkGlOptionalCaps();

	//gl init
	void init3dList();
	void init3dListGLSL();
    void init2dList();
	void init3dListMenu(MainMenu *mm);

	//private aux drawing
	void renderSelectionCircle(Vec3f v, int size, float radius);
	void renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width);
	void renderTile(const Vec2i &pos);
	void renderQuad(int x, int y, int w, int h, const Texture2D *texture);

public:
	//static
    static Texture2D::Filter strToTextureFilter(const string &s);
};

}} //end namespace

#endif
