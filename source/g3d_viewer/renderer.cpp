#include "pch.h"

#include "renderer.h"

#include "opengl.h"
#include "texture_gl.h"
#include "graphics_interface.h"
#include "graphics_factory_gl.h"

namespace Shared { namespace G3dViewer {

using namespace Shared::Graphics::Gl;

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
	mesh = -1;

	memset(customTextures, 0, 8 * sizeof(Texture2D*));

	// default colours...
	colours[0] = Vec3<uint8>( 255u,   0u,   0u);
	colours[1] = Vec3<uint8>(   0u,   0u, 255u);
	colours[2] = Vec3<uint8>(  31u, 127u,   0u);
	colours[3] = Vec3<uint8>( 255u, 255u,   0u);
	colours[4] = Vec3<uint8>( 191u,   0u, 191u);
	colours[5] = Vec3<uint8>(   0u, 191u, 191u);
	colours[6] = Vec3<uint8>(  72u, 255u,  72u);
	colours[7] = Vec3<uint8>( 255u, 127u,   0u);

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
	assert(i >= 0 && i < 8);
	colours[i] = Vec3<uint8>(red, green, blue);
	
	customTextures[i] = textureManager->newTexture2D();
	customTextures[i]->getPixmap()->init(1, 1, 3);
	customTextures[i]->getPixmap()->setPixel(0, 0, colours[i].ptr());
	textureManager->init();
}

void Renderer::init(){
	assertGl();

	GraphicsFactory *gf= new GraphicsFactoryGl();

	GraphicsInterface::getInstance().setFactory(gf);

	modelRenderer= gf->newModelRenderer();
	textureManager= gf->newTextureManager();

	for (int i=0; i < 8; ++i) {
		customTextures[i] = textureManager->newTexture2D();
		customTextures[i]->getPixmap()->init(1, 1, 3);
		customTextures[i]->getPixmap()->setPixel(0, 0, colours[i].ptr());
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
	normals = !normals;
}

void Renderer::toggleWireframe(){
	wireframe = !wireframe;
}

void Renderer::toggleGrid(){
	grid = !grid;
}

void Renderer::loadTheModel(Model *model, string file){
	model->setTextureManager(textureManager);
	model->loadG3d(file);
	textureManager->init();
}

void Renderer::renderTheModel(Model *model, float t){
	if(model != NULL){
		RenderParams params(true, true, !wireframe, false);
		params.setMeshCallback(&meshCallbackTeamColor);
		modelRenderer->begin(params);
		model->updateInterpolationData(t, true);

		if (mesh == -1) {
			modelRenderer->render(model);
		} else {
			modelRenderer->renderMesh(model->getMesh(mesh));
		}

		if(normals){
			glPushAttrib(GL_ENABLE_BIT);
			glDisable(GL_LIGHTING);
			glDisable(GL_TEXTURE_2D);
			glColor3f(1.0f, 1.0f, 1.0f);	
			if (mesh == -1) {
				modelRenderer->renderNormalsOnly(model);
			} else {
				modelRenderer->renderMeshNormalsOnly(model->getMesh(mesh));
			}
			
			glPopAttrib();
		}

		modelRenderer->end();
	}
}

}}//end namespace
