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

#include "pch.h"
#include "renderer.h"

#include "shader.h"

#include "texture_gl.h"
#include "main_menu.h"
#include "config.h"
#include "components.h"
#include "time_flow.h"
#include "graphics_interface.h"
#include "object.h"
#include "core_data.h"
#include "game.h"
#include "metrics.h"
#include "opengl.h"
#include "faction.h"
#include "factory_repository.h"
#include "sim_interface.h"
#include "debug_stats.h"
#include "program.h"
#include "util.h"
#include "leak_dumper.h"

#if _GAE_DEBUG_EDITION_
#	include "debug_renderer.h"
#endif

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Graphics {

// =====================================================
// 	class MeshCallbackTeamColor
// =====================================================

class MeshCallbackTeamColor: public MeshCallback{
private:
	const Texture *teamTexture;

public:
	MeshCallbackTeamColor() : teamTexture(0) {}
	void setTeamTexture(const Texture *teamTexture)	{this->teamTexture= teamTexture;}
	virtual void execute(const Mesh *mesh);
};

void MeshCallbackTeamColor::execute(const Mesh *mesh){

	//team color
	if(mesh->usesTeamTexture() && teamTexture!=NULL){
		//texture 0
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE); // Interpolate RGB components
		// Arg0: source colour 1
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);	   // from this tex (baseTexUnit),
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);  // use rgb from sampled colour
		// Arg1: source colour 2
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE1);	   // from team texture (in unit 1),
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);  // use rgb from sampled colour
		// Arg2: 't' (interpolation factor)
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);     // 't' from this tex,
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);  // use alpha-channel from sampled colour

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);   // Replace alpha component
		// Arg0: value
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);   // alpha from texture. wtf?
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);// meh... this is overwritten below...

		//texture 1
		glActiveTexture(GL_TEXTURE1);
		glMultiTexCoord2f(GL_TEXTURE1, 0.f, 0.f);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(teamTexture)->getHandle());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);        // Modulate RGB components

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);   // Arg0: Mesh Colour
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);      // use RGB

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);        // Arg1: Result from above
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);      // use RGB

		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);       // Replace alpha component
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR); // from mesh colour,
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);    // use alpha component

		glActiveTexture(GL_TEXTURE0);
	}
	else{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

// ===========================================================
//	class Renderer
// ===========================================================

// ===================== PUBLIC ========================

const int   Renderer::maxProgressBar    = 100;
const Vec4f Renderer::progressBarBack1  = Vec4f(0.7f, 0.7f, 0.7f, 0.7f);
const Vec4f Renderer::progressBarBack2  = Vec4f(0.7f, 0.7f, 0.7f, 1.f);
const Vec4f Renderer::progressBarFront1 = Vec4f(0.f, 0.5f, 0.f, 1.f);
const Vec4f Renderer::progressBarFront2 = Vec4f(0.f, 0.1f, 0.f, 1.f);

const int   Renderer::maxMouse2dAnim        = 100;

const float Renderer::selectionCircleRadius = 0.7f;
const float Renderer::magicCircleRadius     = 1.f;

const float Renderer::sunDist           = 10e6;
const float Renderer::moonDist          = 10e6;
const float Renderer::lightAmbFactor    = 0.4f;
const float Renderer::maxLightDist      = 50.f;

const GLenum Renderer::baseTexUnit      = GL_TEXTURE0;
const GLenum Renderer::fowTexUnit       = GL_TEXTURE1;
const GLenum Renderer::shadowTexUnit    = GL_TEXTURE2;

const float Renderer::ambFactor         = 0.7f;
const Vec4f Renderer::fowColor          = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
const Vec4f Renderer::defSpecularColor  = Vec4f(0.8f, 0.8f, 0.8f, 1.f);
const Vec4f Renderer::defDiffuseColor   = Vec4f(1.f, 1.f, 1.f, 1.f);
const Vec4f Renderer::defAmbientColor   = Vec4f(1.f * ambFactor, 1.f * ambFactor, 1.f * ambFactor, 1.f);
const Vec4f Renderer::defColor          = Vec4f(1.f, 1.f, 1.f, 1.f);

// ==================== constructor and destructor ====================

Renderer::Renderer() {
	GraphicsInterface &gi= GraphicsInterface::getInstance();
	FactoryRepository &fr= FactoryRepository::getInstance();
	Config &config= Config::getInstance();

	gi.setFactory(fr.getGraphicsFactory("OpenGL"));
	GraphicsFactory *graphicsFactory= GraphicsInterface::getInstance().getFactory();

	modelRenderer= graphicsFactory->newModelRenderer();
//	textRenderer= graphicsFactory->newTextRendererBM();
	textRendererFT = graphicsFactory->newTextRendererFT();
	particleRenderer= graphicsFactory->newParticleRenderer();

	//resources
	for(int i=0; i<ResourceScope::COUNT; ++i){
		modelManager[i]= graphicsFactory->newModelManager();
		textureManager[i]= graphicsFactory->newTextureManager();
		modelManager[i]->setTextureManager(textureManager[i]);
		particleManager[i]= graphicsFactory->newParticleManager();
		fontManager[i]= graphicsFactory->newFontManager();
	}
	perspFov = config.getRenderFov();
	perspNearPlane = config.getRenderDistanceMin();
	perspFarPlane = config.getRenderDistanceMax();
	game = 0;
	m_mainMenu = 0;
	m_teamColourMode = TeamColourMode::DISABLED;
}

Renderer::~Renderer(){
	delete modelRenderer;
	delete textRendererFT;
	delete particleRenderer;

	//resources
	for(int i=0; i<ResourceScope::COUNT; ++i){
		delete modelManager[i];
		delete textureManager[i];
		delete particleManager[i];
		delete fontManager[i];
	}
}

Renderer &Renderer::getInstance(){
	static Renderer renderer;
	return renderer;
}


// ==================== init ====================

bool Renderer::init() {

	{	ONE_TIME_TIMER(Renderer_Load_Config, cout);
		// config
		g_logger.logProgramEvent("Initialising renderer.");
		Config &config= Config::getInstance();
		loadConfig();
		if (config.getRenderCheckGlCaps()) {
			checkGlCaps();
		}
		if (config.getMiscFirstTime()) {
			config.setMiscFirstTime(false);
			autoConfig();
			config.save();
		}
	}
	{	ONE_TIME_TIMER(Renderer_Init_Resources, cout);
		// init resource managers
		g_logger.logProgramEvent("\tinit core data.");
		try {
			modelManager[ResourceScope::GLOBAL]->init();
			textureManager[ResourceScope::GLOBAL]->init();
			fontManager[ResourceScope::GLOBAL]->init();
		} catch (runtime_error &e) {
			g_logger.logProgramEvent(
				string("\tError while initialising data in ResourceScope::GLOBAL.\n\t") + e.what());
			return false;
		}
	}
	// load shader code (todo ?: do this in initGame(), so a shader-set can be selected in menu)
	if (g_config.getRenderTestingShaders()) {
		ONE_TIME_TIMER(Renderer_Load_Test_Shaders, cout);
		// some hacky stuff so we can test easier, get a list of shader 'sets' to load
		string names = g_config.getRenderModelTestShaders();
		g_logger.logProgramEvent("\t'renderTestShaders' is set, loading model shaders: " + names + ".");
		char *tmp = new char[names.size() + 1];
		strcpy(tmp, names.c_str());
		vector<string> programNames;
		char *tok = strtok(tmp, ",");
		while (tok) {
			string name = tok;
			trimString(name);
			programNames.push_back(name);
			tok = strtok(0, ",");
		}
		delete [] tmp;
		static_cast<ModelRendererGl*>(modelRenderer)->loadShaders(programNames);
		while (mediaErrorLog.hasError()) {
			MediaErrorLog::ErrorRecord rec = mediaErrorLog.popError();
			g_logger.logError(rec.path, rec.msg);
		}
	// load single model shader
	} else if (g_config.getRenderUseShaders()) {
		ONE_TIME_TIMER(Renderer_Load_Shader, cout);
		bool bump = g_config.getRenderEnableBumpMapping();
		bool spec = g_config.getRenderEnableSpecMapping();
		if (spec) {
			if (!fileExists("gae/shaders/spec.vs") || !fileExists("gae/shaders/bump_spec.vs")) {
				g_config.setRenderEnableSpecMapping(false);
				spec = false;
			}
		}
		string name;
		if (bump && spec) {
			name = "bump_spec";
		} else if (bump) {
			name = "bump";
		} else if (spec) {
			name = "spec";
		} else {
			name = "basic";
		}
		g_logger.logProgramEvent("Loading model shader: " + name + ".");
		static_cast<ModelRendererGl*>(modelRenderer)->setShader(name);
		while (mediaErrorLog.hasError()) {
			MediaErrorLog::ErrorRecord rec = mediaErrorLog.popError();
			g_logger.logError(rec.path, rec.msg);
		}
	}
	g_logger.logProgramEvent("\tinit 2d display lists.");
	init2dList();
	return true;
}

void Renderer::resetGlLists() {
	init2dList();

	if (m_mainMenu) {
		init3dListMenu(m_mainMenu);
	}
	if (game) {
		init3dList();
		init3dListGLSL();
	}
}

void Renderer::changeShader(const string &name) {
	if (name.empty()) {
		g_logger.logProgramEvent("Deleting model shader.");
	} else {
		g_logger.logProgramEvent("Loading model shader: " + name + ".");
	}
	static_cast<ModelRendererGl*>(modelRenderer)->setShader(name);
	while (mediaErrorLog.hasError()) {
		MediaErrorLog::ErrorRecord rec = mediaErrorLog.popError();
		g_logger.logError(rec.path, rec.msg);
	}
}

void Renderer::cycleShaders() {
	if (g_config.getRenderUseShaders()) {
		ModelRendererGl *mr = static_cast<ModelRendererGl*>(modelRenderer);
		mr->cycleShaderSet();
		if (World::isConstructed()) {
			g_console.addLine("Shader changed: " + mr->getShaderName());
		}
	}
}

void Renderer::initGame(GameState *game) {
	ONE_TIME_TIMER(Renderer_Init_Game, cout);
	this->game= game;
	m_mainMenu = 0;

	// check gl caps
	checkGlOptionalCaps();

	// vars
	shadowMapFrame= 0;
	waterAnim= 0;

	// terrain renderer
	if (g_config.getRenderTerrainRenderer() == 2) {
		//try {
			m_terrainRenderer = new TerrainRenderer2();
			m_terrainRenderer->init(g_world.getMap(), g_world.getTileset());
		//} catch (runtime_error &e) {
		//	g_logger.logError(string(e.what()) + "\nTerrainRenderer2 disabled.");
		//	m_terrainRenderer = new TerrainRendererGlest();
		//	m_terrainRenderer->init(g_world.getMap(), g_world.getTileset());
		//}
	} else {
		m_terrainRenderer = new TerrainRendererGlest();
		m_terrainRenderer->init(g_world.getMap(), g_world.getTileset());
	}

	// shadows
	if (m_shadowMode == ShadowMode::PROJECTED || m_shadowMode == ShadowMode::MAPPED) {
		if (g_metrics.getScreenH() < shadowTextureSize || g_metrics.getScreenH() < shadowTextureSize) {
			throw runtime_error("Shadow texture size must be smaller than display resolution.");
		}

		static_cast<ModelRendererGl*>(modelRenderer)->setSecondaryTexCoordUnit(2);

		glGenTextures(1, &shadowMapHandle);
		glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (m_shadowMode == ShadowMode::MAPPED) {
			// shadow mapping
			glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
				shadowTextureSize, shadowTextureSize,
				0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
		} else {
			// projected
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
				shadowTextureSize, shadowTextureSize,
				0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);
		}
		shadowMapFrame = -1;
	}

	IF_DEBUG_EDITION(
		Debug::getDebugRenderer().init(); 
	)

	// texture init
	modelManager[ResourceScope::GAME]->init();
	textureManager[ResourceScope::GAME]->init();
	fontManager[ResourceScope::GAME]->init();

	init3dList();
	init3dListGLSL();
}

void Renderer::initMenu(MainMenu *mm){
	ONE_TIME_TIMER(Renderer_Init_Menu, cout);

	modelManager[ResourceScope::MENU]->init();
	textureManager[ResourceScope::MENU]->init();
	fontManager[ResourceScope::MENU]->init();
	//modelRenderer->setCustomTexture(CoreData::getInstance().getCustomTexture());
	modelRenderer->setLightCount(1);

	m_mainMenu = mm;
	init3dListMenu(mm);
}

void Renderer::reset3d(){
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	loadProjectionMatrix();
	if (static_cast<ModelRendererGl*>(modelRenderer)->isUsingShaders()) {
		glCallList(list3dGLSL);
	} else {
		glCallList(list3d);
	}
	pointCount= 0;
	triangleCount= 0;
	assertGl();
}

void Renderer::reset2d() {
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	glCallList(list2d);
	assertGl();
}

void Renderer::reset3dMenu() {
	assertGl();
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	glCallList(list3dMenu);
	assertGl();
}

// ==================== end ====================

void Renderer::end() {

	//delete resources
	modelManager[ResourceScope::GLOBAL]->end();
	textureManager[ResourceScope::GLOBAL]->end();
	fontManager[ResourceScope::GLOBAL]->end();
	particleManager[ResourceScope::GLOBAL]->end();

	//delete 2d list
	glDeleteLists(list2d, 1);
	list2d = 0;
}

void Renderer::endGame() {
	game = 0;
	delete m_terrainRenderer;
	m_terrainRenderer = 0;

	// delete resources
	modelManager[ResourceScope::GAME]->end();
	textureManager[ResourceScope::GAME]->end();
	fontManager[ResourceScope::GAME]->end();
	particleManager[ResourceScope::GAME]->end();

	if (m_shadowMode == ShadowMode::PROJECTED || m_shadowMode == ShadowMode::MAPPED) {
		glDeleteTextures(1, &shadowMapHandle);
	}

	glDeleteLists(list3d, 1);
	glDeleteLists(list3dGLSL, 1);
	list3d = list3dGLSL = 0;
}

void Renderer::endMenu() {
	// delete resources
	modelManager[ResourceScope::MENU]->end();
	textureManager[ResourceScope::MENU]->end();
	fontManager[ResourceScope::MENU]->end();
	particleManager[ResourceScope::MENU]->end();

	glDeleteLists(list3dMenu, 1);
	list3dMenu = 0;
}

void Renderer::reloadResources() {
	for (int i=0; i < ResourceScope::COUNT; ++i) {
		modelManager[i]->end();
		textureManager[i]->end();
		fontManager[i]->end();
	}
	for (int i=0; i < ResourceScope::COUNT; ++i) {
		modelManager[i]->init();
		textureManager[i]->init();
		fontManager[i]->init();
	}
}


// ==================== engine interface ====================

Model* Renderer::newModel(ResourceScope rs) {
	return modelManager[rs]->newModel();
}

Texture2D* Renderer::getTexture2D(ResourceScope rs, const string &path) {
	return textureManager[rs]->getTexture(path);
}

Texture2D* Renderer::newTexture2D(ResourceScope rs) {
	return textureManager[rs]->newTexture2D();
}

Texture3D* Renderer::newTexture3D(ResourceScope rs) {
	return textureManager[rs]->newTexture3D();
}

Font* Renderer::newFreeTypeFont(ResourceScope rs) {
	return fontManager[rs]->newFreeTypeFont();
}

void Renderer::deleteTexture2D(Texture2D *tex, ResourceScope rs) {
	bool res = textureManager[rs]->deleteTexture2D(tex);
	assert(res);
}

void Renderer::manageParticleSystem(ParticleSystem *particleSystem, ResourceScope rs){
	particleManager[rs]->manage(particleSystem);
}

void Renderer::updateParticleManager(ResourceScope rs) {
	//_PROFILE_FUNCTION();
	particleManager[rs]->update();
}

void Renderer::renderParticleManager(ResourceScope rs){
	//_PROFILE_FUNCTION();
	glPushAttrib(GL_DEPTH_BUFFER_BIT  | GL_STENCIL_BUFFER_BIT);
	glDepthFunc(GL_LESS);
	particleRenderer->renderManager(particleManager[rs], modelRenderer);
	glPopAttrib();
}

void Renderer::swapBuffers() {
	//_PROFILE_FUNCTION();
	glFlush();
	GraphicsInterface::getInstance().getCurrentContext()->swapBuffers();
}

// ==================== lighting ====================

//places all the opengl lights
void Renderer::setupLighting() {

	int lightCount = 0;
	const World *world = &g_world;
	const Faction *thisFaction = world->getThisFaction();
	const GameCamera *gameCamera = game->getGameCamera();
	const TimeFlow *timeFlow = world->getTimeFlow();
	float time = timeFlow->getTime();

	assertGl();

	// sun/moon light
	Vec3f lightColour = computeLightColor(time);
	Vec3f fogColor = world->getTileset()->getFogColor();
	Vec4f lightPos = timeFlow->isDay()? computeSunPos(time): computeMoonPos(time);
	nearestLightPos = lightPos;

	glLightfv(GL_LIGHT0, GL_POSITION, lightPos.ptr());
	glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4f(lightColour * lightAmbFactor, 1.f).ptr());
	glLightfv(GL_LIGHT0, GL_DIFFUSE, Vec4f(lightColour, 1.f).ptr());

	///@todo add some spec!
	glLightfv(GL_LIGHT0, GL_SPECULAR, Vec4f(0.0f, 0.0f, 0.f, 1.f).ptr());

	glFogfv(GL_FOG_COLOR, Vec4f(fogColor * lightColour, 1.f).ptr());

	++lightCount;

	// disable all secondary lights
	for (int i= 1; i < maxLights; ++i) {
		glDisable(GL_LIGHT0 + i);
	}

	//unit lights (not projectiles)
	if (timeFlow->isTotalNight()) {
		for (int i=0; i < world->getFactionCount() && lightCount < maxLights; ++i) {
			for (int j=0; j < world->getFaction(i)->getUnitCount() && lightCount < maxLights; ++j) {
				Unit *unit = world->getFaction(i)->getUnit(j);
				if (thisFaction->canSee(unit) && !unit->isCarried()
				&& unit->getType()->getLight() && unit->isOperative()
				&& unit->getCurrVector().dist(gameCamera->getPos()) < maxLightDist) {
					Vec4f pos = Vec4f(unit->getCurrVector());
					pos.y += 4.f;
					GLenum lightEnum = GL_LIGHT0 + lightCount;

					glEnable(lightEnum);
					glLightfv(lightEnum, GL_POSITION, pos.ptr());
					glLightfv(lightEnum, GL_AMBIENT, Vec4f(unit->getType()->getLightColour()).ptr());
					glLightfv(lightEnum, GL_DIFFUSE, Vec4f(unit->getType()->getLightColour()).ptr());
					glLightfv(lightEnum, GL_SPECULAR, Vec4f(unit->getType()->getLightColour() * 0.3f).ptr());
					glLightf(lightEnum, GL_QUADRATIC_ATTENUATION, 0.05f);

					++lightCount;

					const GameCamera *gameCamera= game->getGameCamera();

					if (Vec3f(pos).dist(gameCamera->getPos()) < Vec3f(nearestLightPos).dist(gameCamera->getPos())) {
						nearestLightPos = pos;
					}
				}
			}
		}
	}
	modelRenderer->setLightCount(lightCount);
	assertGl();
}

void Renderer::loadGameCameraMatrix() {
	const GameCamera *gameCamera = game->getGameCamera();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glRotatef(gameCamera->getVAng(), -1, 0, 0);
	glRotatef(gameCamera->getHAng(), 0, 1, 0);
	glTranslatef(-gameCamera->getPos().x, -gameCamera->getPos().y, -gameCamera->getPos().z);
}

void Renderer::loadCameraMatrix(const Camera *camera) {
	Vec3f position = camera->getPosition();
	Quaternion orientation = camera->getOrientation().conjugate();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(orientation.toMatrix4().ptr());
	glTranslatef(-position.x, -position.y, -position.z);
}

void Renderer::computeVisibleArea() {
	culler.establishScene();
	IF_DEBUG_EDITION( Debug::getDebugRenderer().sceneEstablished(culler); )
}

// =======================================
// basic rendering
// =======================================

void Renderer::renderMouse3d() {
	const UserInterface *gui = game->getGui();
	const Mouse3d *mouse3d = gui->getMouse3d();
	const Map *map = g_world.getMap();

	GLUquadricObj *cilQuadric;
	Vec4f color;

	assertGl();

	if ((mouse3d->isEnabled() || gui->isPlacingBuilding()) && gui->isValidPosObjWorld()) {

		if (gui->isPlacingBuilding()) {
			const UserInterface::BuildPositions &bp = gui->getBuildPositions();
			const UnitType *building = gui->getBuilding();
			const UnitVector &units = gui->getSelection()->getUnits();

			// selection building emplacement
			float offset = building->getSize() / 2.f;
			Field buildField = building->getField();
			float rotation = gui->getBuildingFacing() * 90.f;

			foreach_const(UserInterface::BuildPositions, i, bp) {
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
				glEnable(GL_BLEND);
				modelRenderer->setAlphaThreshold(0.f);
				glEnable(GL_COLOR_MATERIAL);
				glDepthMask(GL_FALSE);

				Vec3f pos3f = Vec3f(float((*i).x), map->getCell(*i)->getHeight(), float((*i).y));

				glTranslatef(pos3f.x + offset, pos3f.y, pos3f.z + offset);
				glRotatef(rotation, 0.f, 1.f, 0.f);

				// choose color
				if (map->areFreeCellsOrHaveUnits(*i, building->getSize(), buildField, units)) {
					color = Vec4f(1.f, 1.f, 1.f, 0.5f);
				} else {
					color = Vec4f(1.f, 0.f, 0.f, 0.5f);
				}
				// use of RenderMode::OBJECTS is not an error, we don't want mesh colour being set here.
				modelRenderer->begin(RenderMode::OBJECTS, g_world.getTileset()->getFog());
				glColor4fv(color.ptr());
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.ptr());
				const Model *buildingModel = building->getIdleAnimation();
				buildingModel->updateInterpolationData(0.f, false);
				modelRenderer->render(buildingModel);
				glDisable(GL_COLOR_MATERIAL);
				modelRenderer->end();

				glPopAttrib();
				glPopMatrix();
			}
		} else {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glPushAttrib(GL_CURRENT_BIT | GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_BLEND);
			glDisable(GL_STENCIL_TEST);
			glDepthFunc(GL_LESS);
			glEnable(GL_COLOR_MATERIAL);
			glDepthMask(GL_FALSE);

			Vec2i pos = mouse3d->getPos();
			Vec3f pos3f = Vec3f(float(pos.x), map->getCell(pos)->getHeight(), float(pos.y));

			//standard mouse
			glDisable(GL_TEXTURE_2D);
			glDisable(GL_CULL_FACE);
			color= Vec4f(1.f, 0.f, 0.f, 1.f - mouse3d->getFade());
			glColor4fv(color.ptr());
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, color.ptr());

			glTranslatef(pos3f.x, pos3f.y + 2.f, pos3f.z);
			glRotatef(90.f, 1.f, 0.f, 0.f);
			glRotatef(float(mouse3d->getRot()), 0.f, 0.f, 1.f);

			cilQuadric = gluNewQuadric();
			gluQuadricDrawStyle(cilQuadric, GLU_FILL);
			gluCylinder(cilQuadric, 0.5f, 0.f, 2.f, 4, 1);
			gluCylinder(cilQuadric, 0.5f, 0.f, 0.f, 4, 1);
			glTranslatef(0.f, 0.f, 1.f);
			gluCylinder(cilQuadric, 0.7f, 0.f, 1.f, 4, 1);
			gluCylinder(cilQuadric, 0.7f, 0.f, 0.f, 4, 1);
			gluDeleteQuadric(cilQuadric);

			glPopAttrib();
			glPopMatrix();
		}
	}
}

void Renderer::renderBackground(const Texture2D *texture){
	assertGl();
	glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.f, 1.f, 1.f, 1.f);
		renderQuad(0, 0, g_config.getDisplayWidth(), g_config.getDisplayHeight(), texture);
	glPopAttrib();
	assertGl();
}

void Renderer::renderTextureQuad(int x, int y, int w, int h, const Texture2D *texture, float alpha) {
	assertGl();
	glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glColor4f(1.f, 1.f, 1.f, alpha);
		renderQuad(x, y, w, h, texture);
	glPopAttrib();
	assertGl();
}

void Renderer::renderSelectionQuad() {
	const UserInterface *gui = game->getGui();
	const SelectionQuad *sq = gui->getSelectionQuad();

	Vec2i down = sq->getPosDown();
	Vec2i up = sq->getPosUp();

	if (gui->isSelecting()) {
		glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT);
			glColor3f(0.f, 1.f, 0.f);
			glBegin(GL_LINE_LOOP);
				glVertex2i(down.x, down.y);
				glVertex2i(up.x, down.y);
				glVertex2i(up.x, up.y);
				glVertex2i(down.x, up.y);
			glEnd();
		glPopAttrib();
	}
}

// ==================== complex rendering ====================

void Renderer::renderObjects() {
	SECTION_TIMER(RENDER_OBJECTS);
	SECTION_TIMER(RENDER_MODELS);
	const World *world= &g_world;
	const Map *map= world->getMap();

	assertGl();
	const Texture2D *fowTex= g_userInterface.getMinimap()->getFowTexture();

	const Tileset *tileset = g_world.getTileset();
	Vec3f baseFogColor = tileset->getFog() ? tileset->getFogColor() : Vec3f(1.f);
	baseFogColor = baseFogColor * computeLightColor(world->getTimeFlow()->getTime());

	glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);

	if(m_shadowMode == ShadowMode::MAPPED){
		glActiveTexture(shadowTexUnit);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
		enableProjectiveTexturing();
	}

	glActiveTexture(baseTexUnit);

	glEnable(GL_COLOR_MATERIAL);

	modelRenderer->begin(RenderMode::OBJECTS, g_world.getTileset()->getFog());
	modelRenderer->setAlphaThreshold(0.5f);

	int thisTeamIndex = world->getThisTeamIndex();

	SceneCuller::iterator it = culler.tile_begin();
	for ( ; it != culler.tile_end(); ++it ) {
		const Vec2i &pos = *it;
		if (!map->isInsideTile(pos)) continue;
		Tile *sc= map->getTile(pos);
		MapObject *o= sc->getObject();
		if(o && sc->isExplored(thisTeamIndex)) {

			const Model *objModel= sc->getObject()->getModel();
			Vec3f v= o->getPos();

			// ambient and diffuse color is taken from tile pos on FoW tex (ie, shades of grey)
			float fowFactor= fowTex->getPixmap()->getPixelf(pos.x, pos.y);
			Vec4f color= Vec4f(Vec3f(fowFactor), 1.f);
			glColor4fv(color.ptr());
			Vec4f matColour = color * ambFactor;
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matColour.ptr());
			Vec4f fogColour = Vec4f(baseFogColor * fowFactor, 1.f);
			glFogfv(GL_FOG_COLOR, fogColour.ptr());

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glTranslatef(v.x, v.y, v.z);
				glRotatef(o->getRotation(), 0.f, 1.f, 0.f);

				objModel->updateInterpolationData(0.f, true);
				modelRenderer->render(objModel);

				triangleCount+= objModel->getTriangleCount();
				pointCount+= objModel->getVertexCount();
			glPopMatrix();

		}
	}
	modelRenderer->end();

	//restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
	glPopAttrib();
}

void Renderer::renderWater(){
	SECTION_TIMER(RENDER_WATER);

	bool closed= false;
	World &world = g_world;
	Map *map = world.getMap();

	float waterAnim = world.getWaterEffects()->getAmin();

	//assert
	assertGl();

	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT);

	// water texture unit
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_ALPHA_TEST);

	glEnable(GL_BLEND);
	if(textures3D){
		Texture3D *waterTex = world.getTileset()->getWaterTex();
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, static_cast<Texture3DGl*>(waterTex)->getHandle());
	}
	else{
		glEnable(GL_COLOR_MATERIAL);
		glColor4f(0.5f, 0.5f, 1.0f, 0.5f);
		glBindTexture(GL_TEXTURE_3D, 0);
	}

	assertGl();

	//fog of War texture Unit
	const Texture2D *fowTex = g_userInterface.getMinimap()->getFowTexture();
	glActiveTexture(fowTexUnit);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(fowTex)->getHandle());
	glActiveTexture(baseTexUnit);

	assertGl();

	//Rect2i boundingRect= visibleQuad.computeBoundingRect();
	//Rect2i scaledRect= boundingRect/Map::cellScale;
	Rect2i scaledRect = culler.getBoundingRectTile();
	scaledRect.clamp(0, 0, map->getTileW()-1, map->getTileH()-1);
	MapVertexData &mapData = *map->getVertexData();

	int thisTeamIndex = world.getThisTeamIndex();
	glNormal3f(0.f, 1.f, 0.f);

	float waterLevel = map->getWaterLevel();
	for(int j = scaledRect.p[0].y; j < scaledRect.p[1].y; ++j) {
		glBegin(GL_TRIANGLE_STRIP);

		for(int i = scaledRect.p[0].x; i <= scaledRect.p[1].x; ++i) {
			Vec2i tilePos0(i, j);
			Vec2i tilePos1(i, j + 1);
			Tile *tc0= map->getTile(i, j);
			Tile *tc1= map->getTile(i, j+1);

			if(tc0->getNearSubmerged() && (tc0->isExplored(thisTeamIndex) || tc1->isExplored(thisTeamIndex))){
				closed= false;

				triangleCount+= 2;
				pointCount+= 2;

				//vertex 1
				glMaterialfv(
					GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					computeWaterColor(waterLevel, map->getTileHeight(tilePos1)).ptr());
				glMultiTexCoord2fv(GL_TEXTURE1, mapData.get(tilePos1).fowTexCoord().ptr());
				glTexCoord3f( (float)i, 1.f, waterAnim );
				glVertex3f(
					float(i) * GameConstants::mapScale,
					waterLevel,
					float(j + 1) * GameConstants::mapScale);

				//vertex 2
				glMaterialfv(
					GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
					computeWaterColor(waterLevel, map->getTileHeight(tilePos0)).ptr());
				glMultiTexCoord2fv(GL_TEXTURE1, mapData.get(tilePos0).fowTexCoord().ptr());
				glTexCoord3f( (float)i, 0.f, waterAnim );
				glVertex3f(
					float(i) * GameConstants::mapScale,
					waterLevel,
					float(j) * GameConstants::mapScale);

			} else {
				if ( !closed ) {
					pointCount+= 2;
					//vertex 1
					glMaterialfv(
						GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
						computeWaterColor(waterLevel, map->getTileHeight(tilePos1)).ptr());
					glMultiTexCoord2fv(GL_TEXTURE1, mapData.get(tilePos1).fowTexCoord().ptr());
					glTexCoord3f( (float)i, 1.f, waterAnim );
					glVertex3f(
						float(i) * GameConstants::mapScale,
						waterLevel,
						float(j + 1) * GameConstants::mapScale);

					//vertex 2
					glMaterialfv(
						GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,
						computeWaterColor(waterLevel, map->getTileHeight(tilePos0)).ptr());
					glMultiTexCoord2fv(GL_TEXTURE1, mapData.get(tilePos0).fowTexCoord().ptr());
					glTexCoord3f( (float)i, 0.f, waterAnim );
					glVertex3f(
						float(i) * GameConstants::mapScale,
						waterLevel,
						float(j) * GameConstants::mapScale);

					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					closed= true;
				}
			}
		}
		glEnd();
	}

	//restore
	glPopAttrib();

	assertGl();
}

#if _GAE_DEBUG_EDITION_

} // end namespace Graphics

namespace Debug {

bool reportRenderUnitsFlag = false;

} // end namespace Debug

namespace Graphics {

void reportRenderUnits(vector<const Unit*> *unitLists) {
	cout << "renderUnitsReport ::\n";
	for (int i=0; i < GameConstants::maxPlayers + 1; ++i) {
		foreach_const (vector<const Unit*>, it, unitLists[i]) {
			string unitType = (*it)->getType()->getName();
			string factionType = i ? (*it)->getFaction()->getType()->getName() : "gaia";
			cout << "\tUnit: " << unitType << " of faction " << (i - 1) << " (" << factionType << ")\n";
		}
	}
}

#endif

void Renderer::renderUnits() {
	SECTION_TIMER(RENDER_UNITS);
	SECTION_TIMER(RENDER_MODELS);
	const Unit *unit;
	const World *world= &g_world;
	const Faction *thisFaction = world->getThisFaction();
	MeshCallbackTeamColor meshCallbackTeamColor;

	assertGl();

	glPushAttrib(GL_ENABLE_BIT | GL_FOG_BIT | GL_LIGHTING_BIT | GL_TEXTURE_BIT);
	glEnable(GL_COLOR_MATERIAL);

	if(m_shadowMode == ShadowMode::MAPPED){
		glActiveTexture(shadowTexUnit);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, shadowMapHandle);

		static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
		enableProjectiveTexturing();
	}

	glActiveTexture(baseTexUnit);

	modelRenderer->begin(RenderMode::UNITS, g_world.getTileset()->getFog(), &meshCallbackTeamColor);

	vector<const Unit*> toRender[GameConstants::maxPlayers + 1];

	///@todo Just use cells to determine the alive ones, then check all dead ones
	for (int i=0; i < world->getFactionCount(); ++i) { 
		for (int j=0; j < world->getFaction(i)->getUnitCount(); ++j) {
			unit = world->getFaction(i)->getUnit(j);
			if (thisFaction->canSee(unit) && culler.isInside(unit->getCenteredPos())) {
				toRender[i + 1].push_back(unit);
			}
		}
	}
	for (int i=0; i < world->getGlestimals()->getUnitCount(); ++i) {
			unit = world->getGlestimals()->getUnit(i);
			if (thisFaction->canSee(unit) && culler.isInside(unit->getPos())) {
				toRender[0].push_back(unit);
			}
	}
	IF_DEBUG_EDITION(
		if (Debug::reportRenderUnitsFlag) {
			reportRenderUnits(toRender);
			Debug::reportRenderUnitsFlag = false;
		}
	)
	const int frame = g_world.getFrameCount();
	for (int i=0; i < GameConstants::maxPlayers + 1; ++i) {
		if (toRender[i].empty()) continue;

		if (i) {
			meshCallbackTeamColor.setTeamTexture(world->getFaction(i - 1)->getTexture());
			int ndx = g_simInterface.getGameSettings().getColourIndex(i - 1);
			modelRenderer->setTeamColour(getFactionColour(ndx));
		} else {
			meshCallbackTeamColor.setTeamTexture(0);
			Vec3f black(0.f);
			modelRenderer->setTeamColour(black);
		}

		glMatrixMode(GL_MODELVIEW);
		vector<const Unit *>::iterator it = toRender[i].begin();
		for ( ; it != toRender[i].end(); ++it) {
			unit = *it;
			if (unit->isCarried()) {
				continue;
			}
			RUNTIME_CHECK(unit->getPos().x >= 0 && unit->getPos().y >= 0);
			RUNTIME_CHECK(unit->getPos().x < world->getMap()->getW() && unit->getPos().y < world->getMap()->getH());
			float alphaThreshold = 0.5f;
			ShaderProgram *shader = 0;
			const int id = unit->getId();

			// get model, lerp to animProgess
			const Model *model = unit->getCurrentModel();
			bool cycleAnim = unit->isAlive() && !unit->getCurrSkill()->isStretchyAnim();
			model->updateInterpolationData(unit->getAnimProgress(), cycleAnim);

			// push model-view matrix
			glPushMatrix();
			Vec3f currVec = unit->getCurrVectorSink();

			// translate
			glTranslatef(currVec.x, currVec.y, currVec.z);

			// rotate
			glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);
			//glRotatef(unit->getVerticalRotation(), 1.f, 0.f, 0.f);

			// dead/cloak alpha
			float alpha = unit->getRenderAlpha();
			bool fade = alpha < 1.f;

			///@todo generalise so custom shaders can be attached to other things
			/// all controlled with Lua snippets perhaps.
			if (fade) {
				alphaThreshold = 0.f;
				if (unit->isCloaked()) {
					if (unit->getFaction()->isAlly(thisFaction)) {
						shader = unit->getType()->getCloakType()->getAllyShader();
					} else {
						shader = unit->getType()->getCloakType()->getEnemyShader();
					}
				}
			}

			// team colour tint shader?
			if (m_teamColourMode >= TeamColourMode::TINT) {
				shader = static_cast<ModelRendererGl*>(modelRenderer)->getTeamTintShader();
			}

			// render
			if (m_teamColourMode == TeamColourMode::OUTLINE || m_teamColourMode == TeamColourMode::BOTH) {
				modelRenderer->renderOutlined(model, 4, modelRenderer->getTeamColour(), alpha, frame, id, shader);
			} else {
				modelRenderer->render(model, alpha, frame, id, shader);
			}

			// inc tri & point counters
			triangleCount += model->getTriangleCount();
			pointCount += model->getVertexCount();

			// restore
			if (fade) {
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
				glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
			}
			glPopMatrix();
		}
	}
	modelRenderer->end();

	//restore
	static_cast<ModelRendererGl*>(modelRenderer)->setDuplicateTexCoords(true);
	glPopAttrib();

	//assert
	assertGl();
}

void Renderer::renderSelectionEffects() {
	const World *world= &g_world;
	const Map *map= world->getMap();
	const Selection *selection = game->getGui()->getSelection();
	const MapObject *selectedObj =  game->getGui()->getSelectedObject();

	glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDepthFunc(GL_ALWAYS);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glLineWidth(2.f);

	// units
	for (int i=0; i<selection->getCount(); ++i) {

		const Unit *unit= selection->getUnit(i);

		RUNTIME_CHECK(!unit->isCarried() && unit->getPos().x >= 0 && unit->getPos().y >= 0);

		// translate
		Vec3f currVec = unit->getCurrVectorFlat();
		currVec.y += 0.3f;

		// selection circle colour
		if (world->getThisFactionIndex() == unit->getFactionIndex()) {
			glColor4f(0.f, unit->getHpRatio(), 0.f, 0.3f);
		} else if (world->getThisFaction()->getTeam() == unit->getTeam()) {
			glColor4f(0, 0.f, unit->getHpRatio(), 0.3f);
		} else {
			glColor4f(unit->getHpRatio(), 0.f, 0.f, 0.3f);
		}
		renderSelectionCircle(currVec, unit->getType()->getSize(), selectionCircleRadius);

		// magic circle
		if (world->getThisFactionIndex() == unit->getFactionIndex() && unit->getType()->getMaxEp() > 0) {
			glColor4f(unit->getEpRatio()/2.f, unit->getEpRatio(), unit->getEpRatio(), 0.5f);
			renderSelectionCircle(currVec, unit->getType()->getSize(), magicCircleRadius);
		}
	}

	if (selectedObj) {
		MapResource *r = selectedObj->getResource();
		if (r) {
			const float ratio = float(r->getAmount()) / r->getType()->getDefResPerPatch();
			Vec3f currVec = selectedObj->getPos();
			currVec.y += 0.3f;
			glColor4f(ratio, ratio / 2.f, 0.f, 0.3f);
			renderSelectionCircle(currVec, GameConstants::cellScale, selectionCircleRadius);
		}		
	}

	// target arrow
	if (selection->getCount() == 1) {
		const Unit *unit =  selection->getUnit(0);
		Command *cmd = 0;
		if (selection->getUnit(0)->anyCommand()) {
			cmd = unit->getCurrCommand();
		}

		// comand arrow
		if (focusArrows && cmd && !cmd->isAuto()) {
			Vec3f arrowTarget, arrowColor;
			if (cmd->getType()->getArrowDetails(cmd, arrowTarget, arrowColor)) {
				RUNTIME_CHECK(!unit->isCarried() && unit->getPos().x >= 0 && unit->getPos().y >= 0);
				renderArrow(unit->getCurrVectorFlat(), arrowTarget, arrowColor, 0.3f);
			}
		}

		//meeting point arrow
		if(unit->getType()->hasMeetingPoint()){
			Vec2i pos= unit->getMeetingPos();
			Vec3f arrowTarget= Vec3f( (float)pos.x, map->getCell(pos)->getHeight(), (float)pos.y );
			renderArrow(unit->getCurrVectorFlat(), arrowTarget, Vec3f(0.f, 0.f, 1.f), 0.3f);
		}

	}

	//render selection hightlights
	for(int i=0; i<world->getFactionCount(); ++i){
		for(int j=0; j<world->getFaction(i)->getUnitCount(); ++j){
			const Unit *unit= world->getFaction(i)->getUnit(j);

			if (unit->isHighlighted() && !unit->isCarried()) {
				float highlight= unit->getHightlight();
				if(g_world.getThisFactionIndex()==unit->getFactionIndex()){
					glColor4f(0.f, 1.f, 0.f, highlight);
				}
				else{
					glColor4f(1.f, 0.f, 0.f, highlight);
				}

				RUNTIME_CHECK(!unit->isCarried() && unit->getPos().x >= 0 && unit->getPos().y >= 0);
				Vec3f v= unit->getCurrVectorFlat();
				v.y+= 0.3f;
				renderSelectionCircle(v, unit->getType()->getSize(), selectionCircleRadius);
			}
		}
	}

	glPopAttrib();
}

void Renderer::renderWaterEffects(){
	const World *world= &g_world;
	const WaterEffects *we= world->getWaterEffects();
	const Map *map= world->getMap();
	const CoreData &coreData= CoreData::getInstance();
	float height= map->getWaterLevel()+0.001f;

	assertGl();

	glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_COLOR_MATERIAL);

	glNormal3f(0.f, 1.f, 0.f);

	//splashes
	glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(coreData.getWaterSplashTexture())->getHandle());
	for(int i=0; i<we->getWaterSplashCount(); ++i){
		const WaterSplash *ws= we->getWaterSplash(i);

		//render only if enabled
		if(ws->getEnabled()){

			//render only if visible
			Vec2i intPos= Vec2i(static_cast<int>(ws->getPos().x), static_cast<int>(ws->getPos().y));
			if(map->getTile(Map::toTileCoords(intPos))->isVisible(world->getThisTeamIndex())){

				float scale= ws->getAnim();

				glColor4f(1.f, 1.f, 1.f, 1.f-ws->getAnim());
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.f, 1.f);
					glVertex3f(ws->getPos().x-scale, height, ws->getPos().y+scale);
					glTexCoord2f(0.f, 0.f);
					glVertex3f(ws->getPos().x-scale, height, ws->getPos().y-scale);
					glTexCoord2f(1.f, 1.f);
					glVertex3f(ws->getPos().x+scale, height, ws->getPos().y+scale);
					glTexCoord2f(1.f, 0.f);
					glVertex3f(ws->getPos().x+scale, height, ws->getPos().y-scale);
				glEnd();
			}
		}
	}

	glPopAttrib();

	assertGl();
}

void Renderer::renderMenuBackground(const MenuBackground *menuBackground){
	assertGl();
	Vec3f cameraPosition = menuBackground->getCamera()->getPosition();
	glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT);
	
	// clear
	Vec4f fogColor = Vec4f(0.4f, 0.4f, 0.4f, 1.f) * menuBackground->getFade();
	glClearColor(fogColor.r, fogColor.g, fogColor.b, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFogfv(GL_FOG_COLOR, fogColor.ptr());
	
	// light
	Vec4f lightPos  = Vec4f(-1.f, -1.f, -1.f, 0.f)/* * menuBackground->getFade()*/;
	Vec4f diffLight = Vec4f(0.9f, 0.9f, 0.9f, 1.f) * menuBackground->getFade();
	Vec4f ambLight  = Vec4f(0.3f, 0.3f, 0.3f, 1.f) * menuBackground->getFade();
	Vec4f specLight = Vec4f(0.1f, 0.1f, 0.1f, 1.f) * menuBackground->getFade();
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos.ptr());
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffLight.ptr());
	glLightfv(GL_LIGHT0, GL_AMBIENT,  ambLight.ptr());
	glLightfv(GL_LIGHT0, GL_SPECULAR, specLight.ptr());
	modelRenderer->setLightCount(1);
	
	// main model
	modelRenderer->setTeamColour(Vec3f(0.f));
	modelRenderer->setAlphaThreshold(0.5f);
	glColor3f(1.f, 1.f, 1.f);
	
	ModelRendererGl *mr = static_cast<ModelRendererGl*>(modelRenderer);
	modelRenderer->begin(RenderMode::OBJECTS, menuBackground->getFog());
	modelRenderer->render(menuBackground->getMainModel());
	modelRenderer->end();
	glDisable(GL_ALPHA_TEST);
	
	// characters
	float dist = menuBackground->getAboutPosition().dist(cameraPosition);
	float minDist = 3.f;
	if (dist < minDist) {
		modelRenderer->setAlphaThreshold(0.f);
		float alpha = clamp((minDist-dist) / minDist, 0.f, 1.f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.0f, 1.0f, 1.0f, alpha).ptr());
		glColor4f(1.f, 1.f, 1.f, alpha);
		modelRenderer->begin(RenderMode::OBJECTS, menuBackground->getFog());
		for (int i=0; i < MenuBackground::characterCount; ++i) {
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();
			glTranslatef(i * 2.f - 4.f, -3.f, -6.5f);
			menuBackground->getCharacterModel(i)->updateInterpolationData(menuBackground->getAnim(), true);
			modelRenderer->render(menuBackground->getCharacterModel(i));
			glPopMatrix();
		}
		modelRenderer->end();
	}
	// water
	if (menuBackground->getWater()) {
		// water surface
		const int waterTesselation = 10;
		const int waterSize = 250;
		const int waterQuadSize = 2 * waterSize / waterTesselation;
		const float waterHeight = menuBackground->getWaterHeight();
		glEnable(GL_BLEND);
		glNormal3f(0.f, 1.f, 0.f);
		glColor3f(1.f, 1.f, 1.f);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, Vec4f(1.f, 1.f, 1.f, 1.f).ptr());
		GLuint waterHandle= static_cast<Texture2DGl*>(menuBackground->getWaterTexture())->getHandle();
		glBindTexture(GL_TEXTURE_2D, waterHandle);
		for (int i=1; i < waterTesselation; ++i) {
			glBegin(GL_TRIANGLE_STRIP);
			for (int j=1; j < waterTesselation; ++j) {
				glTexCoord2i(1, 2 % j);
				glVertex3f(float(-waterSize + i * waterQuadSize), waterHeight, float(-waterSize + j * waterQuadSize));
				glTexCoord2i(0, 2 % j);
				glVertex3f(float(-waterSize + (i + 1) * waterQuadSize), waterHeight, float(-waterSize + j * waterQuadSize));
			}
			glEnd();
		}
		
		// raindrops
		if (menuBackground->getRain()) {
			const float maxRaindropAlpha = 0.5f;
			glDisable(GL_LIGHTING);
			glDisable(GL_ALPHA_TEST);
			glDepthMask(GL_FALSE);

			// splashes
			CoreData &coreData= CoreData::getInstance();
			glBindTexture(GL_TEXTURE_2D, static_cast<Texture2DGl*>(coreData.getWaterSplashTexture())->getHandle());
			for (int i=0; i < MenuBackground::raindropCount; ++i) {
				Vec2f pos = menuBackground->getRaindropPos(i);
				float scale = menuBackground->getRaindropState(i);
				float alpha = maxRaindropAlpha - scale * maxRaindropAlpha;
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glColor4f(1.f, 1.f, 1.f, alpha);
				glTranslatef(pos.x, waterHeight + 0.01f, pos.y);
				glBegin(GL_TRIANGLE_STRIP);
					glTexCoord2f(0.f, 1.f);
					glVertex3f(-scale, 0, scale);
					glTexCoord2f(0.f, 0.f);
					glVertex3f(-scale, 0, -scale);
					glTexCoord2f(1.f, 1.f);
					glVertex3f(scale, 0, scale);
					glTexCoord2f(1.f, 0.f);
					glVertex3f(scale, 0, -scale);
				glEnd();
				glPopMatrix();
			}
		}
		glDisable(GL_BLEND);
	}
	glPopAttrib();
	assertGl();
}

// ==================== computing ====================

bool Renderer::computePosition(const Vec2i &screenPos, Vec2i &worldPos){
	assertGl();
	const Map* map= g_world.getMap();
	const Metrics &metrics= Metrics::getInstance();
	float depth= 0.0f;
	GLdouble modelviewMatrix[16];
	GLdouble projectionMatrix[16];
	GLint viewport[4]= {0, 0, metrics.getScreenW(), metrics.getScreenH()};
	GLdouble worldX;
	GLdouble worldY;
	GLdouble worldZ;
	GLint screenX = screenPos.x;
	GLint screenY = g_metrics.getScreenH() - screenPos.y;

	//get the depth in the cursor pixel
	glReadPixels(screenX, screenY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

	//load matrices
	loadProjectionMatrix();
	loadGameCameraMatrix();

	//get matrices
	glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);

	//get the world coordinates
	gluUnProject(
		screenX, screenY, depth,
		modelviewMatrix, projectionMatrix, viewport,
		&worldX, &worldY, &worldZ);

	//conver coords to int
	worldPos= Vec2i(static_cast<int>(roundf(worldX)), static_cast<int>(roundf(worldZ)));

	//clamp coords to map size
	return map->isInside(worldPos);
}

struct PickHit {
	GLuint	nearDist, name1, name2;

	PickHit(GLuint nearDist, GLuint name1, GLuint name2)
			: nearDist(nearDist), name1(name1), name2(name2) {}

	bool operator<(const PickHit &that) const {
		return (memcmp(this, &that, sizeof(PickHit)) < 0);
	}
};

void Renderer::computeSelected(UnitVector &units, const MapObject *&obj, const Vec2i &posDown, const Vec2i &posUp){
	SECTION_TIMER(RENDER_SELECT);
	// declarations
	GLuint selectBuffer[UserInterface::maxSelBuff];
	const Metrics &metrics= Metrics::getInstance();

	// compute center and dimensions of selection rectangle
	int x = (posDown.x + posUp.x) / 2;
	int y = ((metrics.getScreenH() - posDown.y) + (metrics.getScreenH() - posUp.y)) / 2;
	int w = abs(posDown.x - posUp.x);
	int h = abs(posDown.y - posUp.y);
	if (w < 1) w = 1;
	if (h < 1) h = 1;

	//setup matrices
	glSelectBuffer(UserInterface::maxSelBuff, selectBuffer);
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	GLint view[]= {0, 0, metrics.getScreenW(), metrics.getScreenH()};
	glRenderMode(GL_SELECT);
	glLoadIdentity();
	gluPickMatrix(x, y, w, h, view);
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, perspFarPlane);
	loadGameCameraMatrix();

	glInitNames();

	//render units and resources
	renderUnitsFast();
	renderObjectsFast();

	//pop matrices
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	//g_gameState.resetRawPick();
	//g_gameState.resetUnitPickHits();

	// process hits
	int selCount = glRenderMode(GL_RENDER);
	set<PickHit> unitHits, objectHits;
	GLuint *ptr = selectBuffer;
	for (int i = 0; i < selCount; ++i) {
		INVARIANT(*ptr == 2, "something was rendered in selection mode without 2 names on the stack");
		++ptr;
		GLuint nearDist = *ptr++;
		++ptr;
		GLuint name1 = *ptr++;
		GLuint name2 = *ptr++;

		//string rawHit = intToStr(name1) + " : " + intToStr(name2);
		//g_gameState.addRawPick(rawHit);

		if (name1 < GameConstants::maxPlayers + 1) {
			unitHits.insert(PickHit(nearDist, name1, name2));
		} else {
			objectHits.insert(PickHit(nearDist, name1, name2));
		}
	}
	units.clear();
	obj = 0;
	if (unitHits.empty()) { // no units, check objects
		if (!objectHits.empty()) { 
			foreach_const (set<PickHit>, it, objectHits) {
				if (it->name1 == 0x101) { // closest resource hit
					obj = g_world.getMapObj(it->name2);
					break;
				}
			}
			if (!obj) { // no resources, get closest object
				obj = g_world.getMapObj(objectHits.begin()->name2);
			}
		}
	} else {
		foreach_const (set<PickHit>, it, unitHits) {
			Unit *unit = g_world.getUnit(it->name2);
			//string str = intToStr(unit->getId()) + " : " + unit->getType()->getName();
			//g_gameState.addUnitPickHit(str);
			units.push_back(unit);
		}
	}
}


// ==================== shadows ====================

void Renderer::renderShadowsToTexture() {
	SECTION_TIMER(RENDER_SHADOWS);
	if (m_shadowMode == ShadowMode::PROJECTED || m_shadowMode == ShadowMode::MAPPED) {
		shadowMapFrame = (shadowMapFrame + 1) % (shadowFrameSkip + 1);
		if (shadowMapFrame == 0) {
			assertGl();
			glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT | GL_POLYGON_BIT);
			if (m_shadowMode == ShadowMode::MAPPED) {
				glClear(GL_DEPTH_BUFFER_BIT);
			} else {
				float color = 1.0f - shadowAlpha;
				glColor3f(color, color, color);
				glClearColor(1.f, 1.f, 1.f, 1.f);
				glDisable(GL_DEPTH_TEST);
				glClear(GL_COLOR_BUFFER_BIT);
			}
			//clear color buffer
			//
			//set viewport, we leave one texel always in white to avoid problems
			glViewport(1, 1, shadowTextureSize - 2, shadowTextureSize - 2);
			if (nearestLightPos.w == 0.f) {
				//directional light
				//light pos
				const TimeFlow *tf = g_world.getTimeFlow();
				float ang = tf->isDay() ? computeSunAngle(tf->getTime()) : computeMoonAngle(tf->getTime());
				ang = radToDeg(ang);
				//push and set projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				if (game->getGameCamera()->getState() == GameCamera::sGame) {
					glOrtho(-35, 5, -15, 15, -1000, 1000);
				} else {
					glOrtho(-30, 30, -20, 20, -1000, 1000);
				}
				//push and set modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glRotatef(15, 0, 1, 0);
				glRotatef(ang, 1, 0, 0);
				glRotatef(90, 0, 1, 0);
				Vec3f pos = game->getGameCamera()->getPos();
				glTranslatef(-pos.x, 0, -pos.z);
			} else {
				//non directional light
				//push projection
				glMatrixMode(GL_PROJECTION);
				glPushMatrix();
				glLoadIdentity();
				gluPerspective(150, 1.f, perspNearPlane, perspFarPlane);
				//push modelview
				glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				glRotatef(-90, -1, 0, 0);
				glTranslatef(-nearestLightPos.x, -nearestLightPos.y - 2, -nearestLightPos.z);
			}
			if (m_shadowMode == ShadowMode::MAPPED) {
				glEnable(GL_POLYGON_OFFSET_FILL);
				glPolygonOffset(1.0f, 0.001f);
			}
			//render 3d
			renderUnitsFast(true);
			renderObjectsFast(true);
			//read color buffer
			glBindTexture(GL_TEXTURE_2D, shadowMapHandle);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, shadowTextureSize, shadowTextureSize);
			//get elemental matrices
			Matrix4f matrix1;
			matrix1[0] = 0.5f;	matrix1[4] = 0.f;	matrix1[8] = 0.f;	matrix1[12] = 0.5f;
			matrix1[1] = 0.f;	matrix1[5] = 0.5f;	matrix1[9] = 0.f;	matrix1[13] = 0.5f;
			matrix1[2] = 0.f;	matrix1[6] = 0.f;	matrix1[10] = 0.5f;	matrix1[14] = 0.5f;
			matrix1[3] = 0.f;	matrix1[7] = 0.f;	matrix1[11] = 0.f;	matrix1[15] = 1.f;
			Matrix4f matrix2;
			glGetFloatv(GL_PROJECTION_MATRIX, matrix2.ptr());
			Matrix4f matrix3;
			glGetFloatv(GL_MODELVIEW_MATRIX, matrix3.ptr());
			//pop both matrices
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			//compute texture matrix
			glLoadMatrixf(matrix1.ptr());
			glMultMatrixf(matrix2.ptr());
			glMultMatrixf(matrix3.ptr());
			glGetFloatv(GL_TRANSPOSE_PROJECTION_MATRIX_ARB, shadowMapMatrix.ptr());
			//pop
			glPopMatrix();
			glPopAttrib();
			assertGl();
		}
	}
}


// ==================== gl wrap ====================

string Renderer::getGlInfo(){
	string infoStr;
	Lang &lang= Lang::getInstance();
	infoStr += lang.get("OpenGlInfo")+":\n";
	infoStr += "   "+lang.get("OpenGlVersion")+": ";
	infoStr += string(getGlVersion())+"\n";
	infoStr += "   "+lang.get("OpenGlRenderer")+": ";
	infoStr += string(getGlRenderer())+"\n";
	infoStr += "   "+lang.get("OpenGlVendor")+": ";
	infoStr += string(getGlVendor())+"\n";
	infoStr += "   "+lang.get("OpenGlMaxLights")+": ";
	infoStr += intToStr(getGlMaxLights())+"\n";
	infoStr += "   "+lang.get("OpenGlMaxTextureSize")+": ";
	infoStr += intToStr(getGlMaxTextureSize())+"\n";
	
	infoStr += "   "+lang.get("OpenGlMaxConventionalTextureUnits")+": ";
	infoStr += intToStr(getGlMaxTextureUnits())+"\n";

	infoStr += "   " + lang.get("OpenGlMaxCombinedTextureUnits") + ": ";
	infoStr += intToStr(getGlMaxShaderTexUnits()) + "\n";

	infoStr += "   "+lang.get("OpenGlModelviewStack")+": ";
	infoStr += intToStr(getGlModelviewMatrixStackDepth())+"\n";
	infoStr += "   "+lang.get("OpenGlProjectionStack")+": ";
	infoStr += intToStr(getGlProjectionMatrixStackDepth())+"\n";
	return infoStr;
}

string Renderer::getGlMoreInfo() {
	stringstream ss;
	Lang &lang= Lang::getInstance();
	
	// gl extensions
	ss << lang.get("OpenGlExtensions")+":\n";
	string extensions= getGlExtensions();
	string::size_type start = 0, length = 0;
	bool restart = false;
	for (int i=0; i < extensions.size(); ++i, ++length) {
		if (restart) {
			start = i;
			length = 0;
			restart = false;
		}
		if (extensions[i] == ' ') {
			ss << "   " << extensions.substr(start, length) << "\n";
			restart = true;
		}
	}
	ss << "   " << extensions.substr(start, length) << "\n";
	return ss.str();
}

string Renderer::getGlMoreInfo2() {
	stringstream ss;
	Lang &lang= Lang::getInstance();
	
	//platform extensions
	ss << lang.get("OpenGlPlatformExtensions")+":\n";
	string platformExtensions= getGlPlatformExtensions();
	string::size_type start = 0, length = 0;
	bool restart = false;
	for (int i=0; i < platformExtensions.size(); ++i, ++length) {
		if (restart) {
			start = i;
			length = 0;
			restart = false;
		}
		if (platformExtensions[i] == ' ') {
			ss << "   " << platformExtensions.substr(start, length) << "\n";
			restart = true;
		}
	}
	ss << "   " << platformExtensions.substr(start, length) << "\n";
	return ss.str();
}

void Renderer::autoConfig(){
	Config &config= Config::getInstance();
	bool nvidiaCard= toLower(getGlVendor()).find("nvidia")!=string::npos;
	bool atiCard= toLower(getGlVendor()).find("ati")!=string::npos;
	bool shadowExtensions = isGlExtensionSupported("GL_ARB_shadow");

	// 3D textures
	config.setRenderTextures3D(isGlExtensionSupported("GL_EXT_texture3D"));

	string shadows; // shadows mode
	if (getGlMaxTextureUnits() >= 3) {
		if (nvidiaCard && shadowExtensions) {
			shadows = shadowsToStr(ShadowMode::MAPPED);
		} else {
			shadows = shadowsToStr(ShadowMode::PROJECTED);
		}
	} else {
		shadows = shadowsToStr(ShadowMode::DISABLED);
	}
	config.setRenderShadows(shadows);
	config.setRenderLightsMax(atiCard? 1: 4); // max lights
	config.setRenderFilter("Bilinear");       // texture filter

	// Model shaders...
	if (isGlVersionSupported(2, 0, 0)) {
		config.setRenderUseShaders(true);
		config.setRenderEnableBumpMapping(false);
		config.setRenderEnableSpecMapping(false);
	} else {
		config.setRenderUseShaders(false);
		config.setRenderEnableBumpMapping(false);
		config.setRenderEnableSpecMapping(false);
	}
}

void Renderer::clearBuffers(){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::clearZBuffer(){
	glClear(GL_DEPTH_BUFFER_BIT);
}

void Renderer::loadConfig(){
	Config &config= Config::getInstance();

	//cache most used config params
	maxLights= config.getRenderLightsMax();
	photoMode= config.getUiPhotoMode();
	focusArrows= config.getUiFocusArrows();
	textures3D= config.getRenderTextures3D();

	//load shadows
	m_shadowMode = strToShadows(config.getRenderShadows());
	if(m_shadowMode == ShadowMode::PROJECTED || m_shadowMode == ShadowMode::MAPPED){
		shadowTextureSize= config.getRenderShadowTextureSize();
		shadowFrameSkip= config.getRenderShadowFrameSkip();
		shadowAlpha= config.getRenderShadowAlpha();
	}

	// set filter settings
	Texture2D::Filter textureFilter = strToTextureFilter(config.getRenderFilter());
	int maxAnisotropy = config.getRenderFilterMaxAnisotropy();
	for(int i=0; i<ResourceScope::COUNT; ++i){
		textureManager[i]->setFilter(textureFilter);
		textureManager[i]->setMaxAnisotropy(maxAnisotropy);
	}
}

void Renderer::saveScreen(const string &path){

	const Metrics &sm= Metrics::getInstance();

	Pixmap2D pixmap(sm.getScreenW(), sm.getScreenH(), 3);

	glReadPixels(0, 0, pixmap.getW(), pixmap.getH(), GL_RGB, GL_UNSIGNED_BYTE, pixmap.getPixels());
	pixmap.save(path);
}

// ==================== PRIVATE ====================

float Renderer::computeSunAngle(float time){

	float dayTime= TimeFlow::dusk-TimeFlow::dawn;
	float fTime= (time-TimeFlow::dawn)/dayTime;
	return clamp(fTime*pi, pi/8.f, 7.f*pi/8.f);
}

float Renderer::computeMoonAngle(float time){
	float nightTime= 24-(TimeFlow::dusk-TimeFlow::dawn);

	if(time<TimeFlow::dawn){
		time+= 24.f;
	}

	float fTime= (time-TimeFlow::dusk)/nightTime;
	return clamp((1.0f-fTime)*pi, pi/8.f, 7.f*pi/8.f);
}

Vec4f Renderer::computeSunPos(float time){
	float ang= computeSunAngle(time);
	return Vec4f(-cosf(ang)*sunDist, sinf(ang)*sunDist, 0.f, 0.f);
}

Vec4f Renderer::computeMoonPos(float time){
	float ang= computeMoonAngle(time);
	return Vec4f(-cosf(ang)*moonDist, sinf(ang)*moonDist, 0.f, 0.f);
}

Vec3f Renderer::computeLightColor(float time){
	const Tileset *tileset= g_world.getTileset();
	Vec3f color;

	const float transition= 2;
	const float dayStart= TimeFlow::dawn;
	const float dayEnd= TimeFlow::dusk-transition;
	const float nightStart= TimeFlow::dusk;
	const float nightEnd= TimeFlow::dawn-transition;

	if ( time > dayStart && time < dayEnd ) {
		color= tileset->getSunLightColor();
	} else if( time>nightStart || time<nightEnd ) {
		color= tileset->getMoonLightColor();
	} else if( time>=dayEnd && time<=nightStart ) {
		color= tileset->getSunLightColor().lerp((time-dayEnd)/transition, tileset->getMoonLightColor());
	} else if( time>=nightEnd && time<=dayStart ) {
		color= tileset->getMoonLightColor().lerp((time-nightEnd)/transition, tileset->getSunLightColor());
	} else {
		assert(false);
		color= tileset->getSunLightColor();
	}
	return color;
}

Vec4f Renderer::computeWaterColor(float waterLevel, float cellHeight){
	const float waterFactor= 1.5f;
	return Vec4f(1.f, 1.f, 1.f, clamp((waterLevel-cellHeight)*waterFactor, 0.f, 1.f));
}

// ==================== fast render ====================

//render units for shadows or selection purposes
void Renderer::renderUnitsFast(bool renderingShadows) {
	const Unit *unit;
	bool changeColor = false;
	const World *world= &g_world;
	const Faction *thisFaction = world->getThisFaction();
	const Map *map = world->getMap();

	assertGl();

	glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);

	if (!renderingShadows) {
		glDisable(GL_TEXTURE_2D);
	} else {
		glEnable(GL_TEXTURE_2D);
		modelRenderer->setAlphaThreshold(0.5f);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		//set color to the texture alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		//set alpha to the texture alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	}

	RenderMode mode = renderingShadows ? RenderMode::SHADOWS : RenderMode::SELECTION;
	modelRenderer->begin(mode, false, 0);

	vector<const Unit*> toRender[GameConstants::maxPlayers + 1];
	set<const Unit*> unitsSeen;
	for (SceneCuller::iterator it = culler.cell_begin(); it != culler.cell_end(); ++it ) {
		const Vec2i &pos = *it;
		if (!map->isInside(pos)) continue;
		for (Zone z(0); z < Zone::COUNT; ++z) {
			const Unit *unit = map->getCell(pos)->getUnit(z);
			if (unit && thisFaction->canSee(unit) && unitsSeen.find(unit) == unitsSeen.end()) {
				unitsSeen.insert(unit);
				toRender[unit->getFactionIndex() + 1].push_back(unit);
			}
 		}
	}

	for (int i=0; i < GameConstants::maxPlayers + 1; ++i) {
		if (toRender[i].empty()) continue;

		glPushName(i);
		vector<const Unit *>::iterator it = toRender[i].begin();
		for ( ; it != toRender[i].end(); ++it) {
			unit = *it;
			if (unit->isCarried() || (unit->isCloaked() && renderingShadows)) {
				continue;
			}

			glPushName(unit->getId());
			glMatrixMode(GL_MODELVIEW);

			//debuxar modelo
			glPushMatrix();

			RUNTIME_CHECK(!unit->isCarried() && unit->getPos().x >= 0 && unit->getPos().y >= 0);

			//translate
			Vec3f currVec = unit->getCurrVectorFlat();

			glTranslatef(currVec.x, currVec.y, currVec.z);

			//rotate
			glRotatef(unit->getRotation(), 0.f, 1.f, 0.f);

			// faded shadows
			if(renderingShadows) {
				float color = 1.0f - shadowAlpha;
				
				//dead alpha
				float alpha = unit->getRenderAlpha();
				float fade = alpha < 1.0;
				color *= alpha;
				changeColor = changeColor || fade;

				if(m_shadowMode == ShadowMode::MAPPED) {
					if(changeColor) {
						//fprintf(stderr, "color = %f\n", color);
						glColor3f(color, color, color);
						glClearColor(1.f, 1.f, 1.f, 1.f);
						glDisable(GL_DEPTH_TEST);
						glClear(GL_COLOR_BUFFER_BIT);
						if(!fade) {
							changeColor = false;
						}
					}
				} else if(fade) {
					// skip this unit's shadow in projected shadows
					glPopMatrix();
					continue;
				}
			}

			//render
			const Model *model = unit->getCurrentModel();
			model->updateInterpolationData(unit->getAnimProgress(), unit->isAlive());
			modelRenderer->render(model);

			glPopMatrix();
			glPopName();
		}
		glPopName();
	}
	modelRenderer->end();
	glPopAttrib();
}

//render objects for shadows
void Renderer::renderObjectsFast(bool renderingShadows) {
	const World *world= &g_world;
	const Map *map= world->getMap();

	assertGl();

	glPushAttrib(GL_ENABLE_BIT| GL_TEXTURE_BIT);
	glDisable(GL_LIGHTING);
	if (!renderingShadows) {
		glDisable(GL_TEXTURE_2D);
	} else {
		modelRenderer->setAlphaThreshold(0.5f);

		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);

		//set color to the texture alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);

		//set alpha to the texture alpha
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	}

	RenderMode mode = renderingShadows ? RenderMode::SHADOWS : RenderMode::SELECTION;
	modelRenderer->begin(mode, false, 0);

	int thisTeamIndex = world->getThisTeamIndex();

	SceneCuller::iterator it = culler.tile_begin();
	for ( ; it != culler.tile_end(); ++it) {
		const Vec2i &pos = *it;
		if (!map->isInside(pos)) {
			continue;
		}
		Tile *sc = map->getTile(pos);
		MapObject *o = sc->getObject();
		if(o && sc->isExplored(thisTeamIndex)) {
			MapResource *r = o->getResource();
			if (!renderingShadows && !r) {
				continue;
			}
			glPushName(0x101);	// resource
			glPushName(o->getId());	// obj id

			const Model *objModel = sc->getObject()->getModel();
			Vec3f v = o->getPos();
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
				glTranslatef(v.x, v.y, v.z);
				glRotatef(o->getRotation(), 0.f, 1.f, 0.f);
				modelRenderer->render(objModel);
			glPopMatrix();

			glPopName();
			glPopName();
		}
	}
	modelRenderer->end();
	glPopAttrib();
	assertGl();
}

// ==================== gl caps ====================

void Renderer::checkGlCaps(){

	//opengl 1.3
	if(!isGlVersionSupported(1, 3, 0)){
		string message;

		message += "Your system supports OpenGL version \"";
 		message += getGlVersion() + string("\"\n");
 		message += "Glest needs at least version 1.3 to work\n";
 		message += "You may solve this problem by installing your latest video card drivers";

 		throw runtime_error(message.c_str());
	}

	//opengl 1.4 or extension
	if(!isGlVersionSupported(1, 4, 0)){
		checkExtension("GL_ARB_texture_env_crossbar", "Glest");
	}

	//checkExtension("GL_ARB_imaging", "Blending");
}

void Renderer::checkGlOptionalCaps(){

	//shadows
	if(m_shadowMode == ShadowMode::PROJECTED || m_shadowMode == ShadowMode::MAPPED){
		if(getGlMaxTextureUnits()<3){
			throw runtime_error("Your system doesn't support 3 texture units, required for shadows");
		}
	}

	//shadow mapping
	if(m_shadowMode == ShadowMode::MAPPED){
		checkExtension("GL_ARB_shadow", "Shadow Mapping");
	}
}

void Renderer::checkExtension(const string &extension, const string &msg){
	if(!isGlExtensionSupported(extension.c_str())){
		string str= "OpenGL extension not supported: " + extension +  ", required for " + msg;
		throw runtime_error(str);
	}
}

// ==================== init 3d lists ====================

void Renderer::init3dList(){

	const Metrics &metrics= Metrics::getInstance();

	assertGl();

	list3d= glGenLists(1);
	glNewList(list3d, GL_COMPILE_AND_EXECUTE);
	//need to execute, because if not gluPerspective takes no effect and gluLoadMatrix is wrong

		//misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(fowColor.x, fowColor.y, fowColor.z, fowColor.w);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);

		//texture state
		glActiveTexture(shadowTexUnit);
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(fowTexUnit);
		glDisable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		glActiveTexture(baseTexUnit);
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		//material state
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpecularColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glColor4fv(defColor.ptr());

		//blend state
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//alpha test state
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);

		//depth test state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		//lighting state
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		//matrix mode
		glMatrixMode(GL_MODELVIEW);

		//stencil test
		glDisable(GL_STENCIL_TEST);

		//fog
		const Tileset *tileset= g_world.getTileset();
		if(tileset->getFog()){
			glEnable(GL_FOG);
			if(tileset->getFogMode()==fmExp){
				glFogi(GL_FOG_MODE, GL_EXP);
			}
			else{
				glFogi(GL_FOG_MODE, GL_EXP2);
			}

			glFogf(GL_FOG_DENSITY, tileset->getFogDensity());
			glFogfv(GL_FOG_COLOR, tileset->getFogColor().ptr());
		}

	glEndList();

	//assert
	assertGl();

}

void Renderer::init3dListGLSL(){

	const Metrics &metrics= Metrics::getInstance();

	assertGl();

	list3dGLSL = glGenLists(1);
	glNewList(list3dGLSL, GL_COMPILE_AND_EXECUTE);
	//need to execute, because if not gluPerspective takes no effect and gluLoadMatrix is wrong

		// misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(fowColor.r, fowColor.g, fowColor.b, fowColor.a);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);

		// texture state
		glActiveTexture(baseTexUnit);
		glEnable(GL_TEXTURE_2D);

		// material state
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpecularColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);
		glColor4fv(defColor.ptr());

		// alpha test state
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);

		// depth test state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		// lighting state
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		// matrix mode
		glMatrixMode(GL_MODELVIEW);

		// stencil test
		glDisable(GL_STENCIL_TEST);

		// fog
		const Tileset *tileset = g_world.getTileset();
		if (tileset->getFog()) {
			glEnable(GL_FOG);
			if (tileset->getFogMode() == fmExp) {
				glFogi(GL_FOG_MODE, GL_EXP);
			} else {
				glFogi(GL_FOG_MODE, GL_EXP2);
			}
			glFogf(GL_FOG_DENSITY, tileset->getFogDensity());
			glFogfv(GL_FOG_COLOR, tileset->getFogColor().ptr());
		}

	glEndList();

	//assert
	assertGl();
}

void Renderer::init2dList() {
	const Metrics &metrics = g_metrics;
	int width = metrics.getScreenW();//g_config.getDisplayWidth();
	int height = metrics.getScreenH();//g_config.getDisplayHeight();

	//this list sets the state for the 2d rendering without 'virtual' co-ordinates
	list2d = glGenLists(1);
	glNewList(list2d, GL_COMPILE);

		//projection
		glViewport(0, 0, width, height);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.f, width, 0.f, height, 0.f, 1.f);
		glScalef(1, -1, 1);
		glTranslatef(0.f, -float(height), 0.f);

		//modelview
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		//disable everything
		glDisable(GL_LIGHTING);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);
		glFrontFace(GL_CCW);
		glActiveTexture(baseTexUnit);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glDisable(GL_TEXTURE_2D);

		//blend func
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//color
		glColor4f(1.f, 1.f, 1.f, 1.f);

	glEndList();

	assertGl();
}

void Renderer::init3dListMenu(MainMenu *mm){
	assertGl();

	const Metrics &metrics= Metrics::getInstance();
	const MenuBackground *mb= mm->getMenuBackground();

	list3dMenu= glGenLists(1);
	glNewList(list3dMenu, GL_COMPILE_AND_EXECUTE);

		//misc
		glViewport(0, 0, metrics.getScreenW(), metrics.getScreenH());
		glClearColor(0.4f, 0.4f, 0.4f, 1.f);
		glFrontFace(GL_CW);
		glEnable(GL_CULL_FACE);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, 1000);

		//texture state
		glEnable(GL_TEXTURE_2D);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		//material state
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, defSpecularColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, defAmbientColor.ptr());
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, defDiffuseColor.ptr());
		glColor4fv(defColor.ptr());
		glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

		//blend state
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//alpha test state
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.f);

		//depth test state
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

		//lighting state
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

		//matrix mode
		glMatrixMode(GL_MODELVIEW);

		//stencil test
		glDisable(GL_STENCIL_TEST);

		//fog
		if (mb->getFog()) {
			glEnable(GL_FOG);
			glFogi(GL_FOG_MODE, GL_EXP2);
			glFogf(GL_FOG_DENSITY, mb->getFogDensity());
		}

	glEndList();

	//assert
	assertGl();
}


// ==================== misc ====================

void Renderer::loadProjectionMatrix(){
	GLdouble clipping;
	const Metrics &metrics= Metrics::getInstance();

	assertGl();

	clipping= photoMode ? perspFarPlane*100 : perspFarPlane;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(perspFov, metrics.getAspectRatio(), perspNearPlane, clipping);

	assertGl();
}

void Renderer::enableProjectiveTexturing(){
	glTexGenfv(GL_S, GL_EYE_PLANE, &shadowMapMatrix[0]);
	glTexGenfv(GL_T, GL_EYE_PLANE, &shadowMapMatrix[4]);
	glTexGenfv(GL_R, GL_EYE_PLANE, &shadowMapMatrix[8]);
	glTexGenfv(GL_Q, GL_EYE_PLANE, &shadowMapMatrix[12]);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);
	glEnable(GL_TEXTURE_GEN_Q);
}

// ==================== private aux drawing ====================

void Renderer::renderSelectionCircle(Vec3f v, int size, float radius){
	GLUquadricObj *disc;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(v.x, v.y, v.z);
	glRotatef(90.f, 1.f, 0.f, 0.f);
	disc= gluNewQuadric();
	gluQuadricDrawStyle(disc, GLU_FILL);
	gluCylinder(disc, radius*(size-0.2f), radius*size, 0.2f, 30, 1);
	gluDeleteQuadric(disc);

	glPopMatrix();
}

void Renderer::renderArrow(const Vec3f &pos1, const Vec3f &pos2, const Vec3f &color, float width){
	const int tesselation= 3;
	const float arrowEndSize= 0.4f;
	const float maxlen= 25;
	const float blendDelay= 5.f;

	Vec3f dir= Vec3f(pos2-pos1);
	float len= dir.length();

	if(len>maxlen){
		return;
	}
	float alphaFactor= clamp((maxlen-len)/blendDelay, 0.f, 1.f);

	dir.normalize();
	Vec3f normal= dir.cross(Vec3f(0, 1, 0));

	Vec3f pos2Left= pos2 + normal*(width-0.05f) - dir*arrowEndSize*width;
	Vec3f pos2Right= pos2 - normal*(width-0.05f) - dir*arrowEndSize*width;
	Vec3f pos1Left= pos1 + normal*(width+0.05f);
	Vec3f pos1Right= pos1 - normal*(width+0.05f);

	//arrow body
	glBegin(GL_TRIANGLE_STRIP);
	for(int i=0; i<=tesselation; ++i){
		float t= float(i)/tesselation;
		Vec3f a= pos1Left.lerp(t, pos2Left);
		Vec3f b= pos1Right.lerp(t, pos2Right);
		Vec4f c= Vec4f(color, t*0.25f*alphaFactor);

		glColor4fv(c.ptr());
		glVertex3fv(a.ptr());
		glVertex3fv(b.ptr());
	}
	glEnd();

	//arrow end
	glBegin(GL_TRIANGLES);
		glVertex3fv((pos2Left + normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2Right - normal*(arrowEndSize-0.1f)).ptr());
		glVertex3fv((pos2 + dir*(arrowEndSize-0.1f)).ptr());
	glEnd();
}

void Renderer::renderText(const string &text, const Font *font, const Vec4f &color, int x, int y) {
	textRendererFT->begin(font);
	glColor4fv(color.ptr());
	textRendererFT->render(text, x, y + int(font->getMetrics()->getMaxAscent()));
	textRendererFT->end();
}

void Renderer::renderProgressBar(int progress, int x, int y, int w, int h, const Font *font) {
	assert(progress <= maxProgressBar);
	//assert(x >= maxProgressBar);//wtf?

	int widthFactor = static_cast<int>(w / maxProgressBar);
	int width = progress * widthFactor;

	// bar
	glBegin(GL_QUADS);
		glColor4fv(progressBarFront2.ptr());
		glVertex2i(x, y + h);  // bottom left
		glVertex2i(x, y);      // top left
		glColor4fv(progressBarFront1.ptr());
		glVertex2i(x + width, y);     // top right
		glVertex2i(x + width, y + h); // bottom right
	glEnd();

	// transp bar
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);
		glColor4fv(progressBarBack2.ptr());
		glVertex2i(x + width, y + h); // bottom left
		glVertex2i(x + width, y);     // top left
		glColor4fv(progressBarBack1.ptr());
		glVertex2i(x + maxProgressBar * widthFactor, y);     // top right
		glVertex2i(x + maxProgressBar * widthFactor, y + h); // bottom right
	glEnd();
	glDisable(GL_BLEND);

	// text
	string msg = intToStr(progress) + "%";
	int mx = x + maxProgressBar * widthFactor / 2;
	glColor3fv(defColor.ptr());
	textRendererFT->begin(font);
	glColor3f(0.f, 0.f, 0.f);
	textRendererFT->render(msg, mx + 2, y + 2 + int(font->getMetrics()->getMaxAscent()), true);
	glColor3f(1.f, 1.f, 1.f);
	textRendererFT->render(msg, mx, y + int(font->getMetrics()->getMaxAscent()), true);
	textRendererFT->end();
}

void Renderer::renderTile(const Vec2i &pos){

	const Map *map= g_world.getMap();
	Vec2i scaledPos= pos * GameConstants::cellScale;

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(-0.5f, 0.f, -0.5f);

	glInitNames();
	for (int i=0; i < GameConstants::cellScale; ++i) {
		for (int j=0; j < GameConstants::cellScale; ++j) {

			Vec2i renderPos = scaledPos + Vec2i(i, j);

			glPushName(renderPos.y);
			glPushName(renderPos.x);

			glDisable(GL_CULL_FACE);

			glBegin(GL_TRIANGLE_STRIP);
			glVertex3f(
				float(renderPos.x),
				map->getCell(renderPos.x, renderPos.y)->getHeight(),
				float(renderPos.y));
			glVertex3f(
				float(renderPos.x),
				map->getCell(renderPos.x, renderPos.y+1)->getHeight(),
				float(renderPos.y+1));
			glVertex3f(
				float(renderPos.x+1),
				map->getCell(renderPos.x+1, renderPos.y)->getHeight(),
				float(renderPos.y));
			glVertex3f(
				float(renderPos.x+1),
				map->getCell(renderPos.x+1, renderPos.y+1)->getHeight(),
				float(renderPos.y+1));
			glEnd();

			glPopName();
			glPopName();
		}
	}

	glPopMatrix();
}

void Renderer::renderQuad(int x, int y, int w, int h, const Texture2D *texture){
	glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(texture)->getHandle());
	glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2i(0, 0);
		glVertex2i(x, y+h);
		glTexCoord2i(0, 1);
		glVertex2i(x, y);
		glTexCoord2i(1, 0);
		glVertex2i(x+w, y+h);
		glTexCoord2i(1, 1);
		glVertex2i(x+w, y);
	glEnd();
}

ShadowMode Renderer::strToShadows(const string &s){
	if ( s == "Projected" ) {
		return ShadowMode::PROJECTED;
	} else if ( s == "ShadowMapping" ) {
		return ShadowMode::MAPPED;
	}
	return ShadowMode::DISABLED;
}

string Renderer::shadowsToStr(ShadowMode mode){
	switch (mode) {
		case ShadowMode::DISABLED:
			return "Disabled";
		case ShadowMode::PROJECTED:
			return "Projected";
		case ShadowMode::MAPPED:
			return "ShadowMapping";
		default:
			assert(false);
			return "";
	}
}

Texture2D::Filter Renderer::strToTextureFilter(const string &s){
	if ( s == "Bilinear" ) {
		return Texture2D::fBilinear;
	} else if ( s == "Trilinear" ) {
		return Texture2D::fTrilinear;
	}
	throw runtime_error("Error converting from string to FilterType, found: "+s);
}

}}//end namespace