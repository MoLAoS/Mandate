#include "renderer.h"

#include "opengl.h"
#include "texture_gl.h"
#include "graphics_interface.h"
#include "graphics_factory_gl.h"

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;

namespace Shared{ namespace G3dViewer{

// ===============================================
// 	class MeshCallbackTeamColor
// ===============================================

void MeshCallbackTeamColor::execute(const Mesh *mesh){
	
	//team color
	if(mesh->getCustomTexture() && teamTexture!=NULL){
		//texture 0
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		
		//set color to interpolation	
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
		
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE1);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);

		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
		
		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);

		//texture 1
		glActiveTexture(GL_TEXTURE1);
		glMultiTexCoord2f(GL_TEXTURE1, 0.f, 0.f);
		glEnable(GL_TEXTURE_2D);

		glBindTexture(GL_TEXTURE_2D, static_cast<const Texture2DGl*>(teamTexture)->getHandle());
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
				
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_PREVIOUS);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		
		//set alpha to 1
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		
		glActiveTexture(GL_TEXTURE0);
	}
	else{
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
}

// ===============================================
// 	class Renderer
// ===============================================

Renderer::Renderer() {
	normals= false;
	wireframe= false;
	grid= true;

	memset(colours, 0, 4 * 3);
	memset(customTextures, 0, 4 * sizeof(Texture2D*));

	// default colours...

	// player 1 : red
	colours[3*0 + 0] = 0xFF;
	
	// player 2 : blue
	colours[3*1 + 2] = 0xFF;

	// player 3 : green
	colours[3*2 + 1] = 0x5E;
	colours[3*2 + 2] = 0x20;

	// player 4 : yellow
	colours[3*3 + 0] = 0xFF;
	colours[3*3 + 1] = 0xFF;
}

Renderer::~Renderer(){
	delete modelRenderer;
	delete textureManager;
}

Renderer * Renderer::getInstance(){
	static Renderer * renderer = new Renderer();
	return renderer;
}

void Renderer::transform(float rotX, float rotY, float zoom){
	assertGl();
	
	glMatrixMode(GL_MODELVIEW);
	glRotatef(rotY, 1.0f, 0.0f, 0.0f);
	glRotatef(rotX, 0.0f, 1.0f, 0.0f);
	glScalef(zoom, zoom, zoom);
	Vec4f pos(-8.0f, 5.0f, 10.0f, 0.0f);
	glLightfv(GL_LIGHT0,GL_POSITION, pos.ptr());

	assertGl();
}

void Renderer::resetTeamTexture(int i, uint8 red, uint8 green, uint8 blue) {
	assert(i >= 0 && i < 4);
	colours[3*i+0] = red;
	colours[3*i+1] = green;
	colours[3*i+2] = blue;
	
	customTextures[i] = textureManager->newTexture2D();
	customTextures[i]->getPixmap()->init(1, 1, 3);
	customTextures[i]->getPixmap()->setPixel(0, 0, &colours[3*i]);
	textureManager->init();
}

void Renderer::init(){
	assertGl();

	GraphicsFactory *gf= new GraphicsFactoryGl();

	GraphicsInterface::getInstance().setFactory(gf);

	modelRenderer= gf->newModelRenderer();
	textureManager= gf->newTextureManager();

	for (int i=0; i < 4; ++i) {
		customTextures[i] = textureManager->newTexture2D();
		customTextures[i]->getPixmap()->init(1, 1, 3);
		customTextures[i]->getPixmap()->setPixel(0, 0, &colours[3*i]);
	}

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_TEXTURE_2D);
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	Vec4f diffuse= Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
	Vec4f ambient= Vec4f(0.3f, 0.3f, 0.3f, 1.0f);
	Vec4f specular= Vec4f(0.1f, 0.1f, 0.1f, 1.0f);

	glLightfv(GL_LIGHT0,GL_AMBIENT, ambient.ptr());
	glLightfv(GL_LIGHT0,GL_DIFFUSE, diffuse.ptr());
	glLightfv(GL_LIGHT0,GL_SPECULAR, specular.ptr());

	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);

	assertGl();
}

void Renderer::reset(int x, int y, int w, int h, int playerColor){
	assertGl();

	glViewport(x, y, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0f, static_cast<float>(w)/h, 1.0f, 200.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, -1.5, -5);

	meshCallbackTeamColor.setTeamTexture(customTextures[playerColor]);

	if(wireframe){
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_LIGHT0);
	}
	else{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	assertGl();
}

void Renderer::renderGrid(){
	if(grid){
	
		float i;

		assertGl();

		glPushAttrib(GL_ENABLE_BIT);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);

		glBegin(GL_LINES);
		glColor3f(1.0f, 1.0f, 1.0f);
		for(i=-10.0f; i<=10.0f; i+=1.0f){
			glVertex3f(i, 0.0f, 10.0f);
			glVertex3f(i, 0.0f, -10.0f);
		}
		for(i=-10.0f; i<=10.0f; i+=1.0f){
			glVertex3f(10.f, 0.0f, i);
			glVertex3f(-10.f, 0.0f, i);
		}
		glEnd();

		glPopAttrib();

		assertGl();
	}
}

void Renderer::toggleNormals(){
	normals= normals? false: true;
}

void Renderer::toggleWireframe(){
	wireframe= wireframe? false: true;
}

void Renderer::toggleGrid(){
	grid= grid? false: true;
}

void Renderer::loadTheModel(Model *model, string file){
	model->setTextureManager(textureManager);
	model->loadG3d(file);
	textureManager->init();
}

void Renderer::renderTheModel(Model *model, float f){
	if(model != NULL){
		modelRenderer->begin(true, true, !wireframe, &meshCallbackTeamColor);	
		model->updateInterpolationData(f, true);
		modelRenderer->render(model);

		if(normals){
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);	
			modelRenderer->renderNormalsOnly(model);
			glPopAttrib();
		}

		modelRenderer->end();
	}
}

}}//end namespace
