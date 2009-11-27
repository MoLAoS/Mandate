
#if ! DEBUG_RENDERING_ENABLED
#	error debug_renderer.h included without DEBUG_RENDERING_ENABLED
#endif

#ifndef _GLEST_GAME_DEBUG_RENDERER_
#define _GLEST_GAME_DEBUG_RENDERER_

#include "vec.h"
#include "math_util.h"
#include "pixmap.h"
#include "texture.h"
#include "graphics_factory_gl.h"

#include <set>

#include "world.h"

#define theMap (*World::getCurrWorld()->getMap())

using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;
using namespace Shared::Util;

namespace Glest { namespace Game{

class RegionHilightCallback {
public:
	static set<Vec2i> cells;

	Vec4f operator()( const Vec2i &cell ) {
		Vec4f colour(0.f, 0.f, 1.f, 0.f);
		if ( cells.find(cell) == cells.end() ) {
			colour.w = 0.f;
		} else {
			colour.w = 0.6f;
		}
		return colour;
	}
};

class DebugRenderer {
public:
	DebugRenderer () {}

	template< typename CellTextureCallback >
	void renderCellTextures ( Quad2i &visibleQuad ) {
		const Rect2i mapBounds(0, 0, theMap.getTileW()-1, theMap.getTileH()-1);
		float coordStep= theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		assertGl();

		glPushAttrib(GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT);
		glEnable(GL_BLEND);
		glEnable(GL_COLOR_MATERIAL); 
		glDisable(GL_ALPHA_TEST);
		glActiveTexture( GL_TEXTURE0 );

		Quad2i scaledQuad = visibleQuad / Map::cellScale;
		PosQuadIterator pqi( scaledQuad );
		while ( pqi.next() ) {
			const Vec2i &pos= pqi.getPos();
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if(mapBounds.isInside(pos)){
				Tile *tc00 = theMap.getTile(pos.x, pos.y), *tc10 = theMap.getTile(pos.x+1, pos.y),
					*tc01 = theMap.getTile(pos.x, pos.y+1), *tc11 = theMap.getTile(pos.x+1, pos.y+1);
				Vec3f tl = tc00->getVertex (), tr = tc10->getVertex (),
					bl = tc01->getVertex (), br = tc11->getVertex ();
				Vec3f tc = tl + (tr - tl) / 2,  ml = tl + (bl - tl) / 2,
					mr = tr + (br - tr) / 2, mc = ml + (mr - ml) / 2, bc = bl + (br - bl) / 2;
				Vec2i cPos ( cx, cy );
				const Texture2DGl *tex = CellTextureCallback()( cPos );
				renderCellTextured( tex, tc00->getNormal(), tl, tc, mc, ml );
				cPos = Vec2i( cx+1, cy );
				tex = CellTextureCallback()( cPos );
				renderCellTextured( tex, tc00->getNormal(), tc, tr, mr, mc );
				cPos = Vec2i( cx, cy + 1 );
				tex = CellTextureCallback()( cPos );
				renderCellTextured( tex, tc00->getNormal(), ml, mc, bc, bl );
				cPos = Vec2i( cx + 1, cy + 1 );
				tex = CellTextureCallback()( cPos );
				renderCellTextured( tex, tc00->getNormal(), mc, mr, br, bc );
			}
		}
		//Restore
		glPopAttrib();
		//assert
		glGetError();	//remove when first mtex problem solved
		assertGl();

	} // renderCellTextures ()

	template< typename CellOverlayColourCallback >
	void renderCellOverlay ( Quad2i &visibleQuad ) {
		const Rect2i mapBounds( 0, 0, theMap.getTileW() - 1, theMap.getTileH() - 1 );
		float coordStep = theWorld.getTileset()->getSurfaceAtlas()->getCoordStep();
		Vec4f colour;
		assertGl();
		glPushAttrib( GL_LIGHTING_BIT | GL_ENABLE_BIT | GL_FOG_BIT | GL_TEXTURE_BIT );
		glEnable( GL_BLEND );
		glEnable( GL_COLOR_MATERIAL ); 
		glDisable( GL_ALPHA_TEST );
		glActiveTexture( GL_TEXTURE0 );
		glDisable( GL_TEXTURE_2D );
		Quad2i scaledQuad = visibleQuad / Map::cellScale;
		PosQuadIterator pqi( scaledQuad );
		while ( pqi.next() ){
			const Vec2i &pos = pqi.getPos();
			int cx, cy;
			cx = pos.x * 2;
			cy = pos.y * 2;
			if ( mapBounds.isInside( pos ) ) {
				Tile *tc00= theMap.getTile( pos.x, pos.y ),		*tc10= theMap.getTile( pos.x+1, pos.y ),
					 *tc01= theMap.getTile( pos.x, pos.y+1 ),	*tc11= theMap.getTile( pos.x+1, pos.y+1 );
				Vec3f tl = tc00->getVertex(),	tr = tc10->getVertex(),
					  bl = tc01->getVertex(),	br = tc11->getVertex(); 
				tl.y += 0.25f; tr.y += 0.25f; bl.y += 0.25f; br.y += 0.25f;
				Vec3f tc = tl + (tr - tl) / 2,	ml = tl + (bl - tl) / 2,	mr = tr + (br - tr) / 2,
					  mc = ml + (mr - ml) / 2,	bc = bl + (br - bl) / 2;

				colour = CellOverlayColourCallback()( Vec2i(cx,cy ) );
				renderCellOverlay( colour, tc00->getNormal(), tl, tc, mc, ml );
				colour = CellOverlayColourCallback()( Vec2i(cx+1, cy) );
				renderCellOverlay( colour, tc00->getNormal(), tc, tr, mr, mc );
				colour = CellOverlayColourCallback()( Vec2i(cx, cy + 1) );
				renderCellOverlay( colour, tc00->getNormal(), ml, mc, bc, bl );
				colour = CellOverlayColourCallback()( Vec2i(cx + 1, cy + 1) );
				renderCellOverlay( colour, tc00->getNormal(), mc, mr, br, bc );
			}
		}
		glEnd(); //??
		//Restore
		glPopAttrib();
		//assert
		glGetError();	//remove when first mtex problem solved
		assertGl();
	}

	void renderRegionHilight(Quad2i &visibleQuad) {
		renderCellOverlay<RegionHilightCallback>(visibleQuad);
	}


private:
	void renderCellTextured( const Texture2DGl *tex, const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3  ) {
		glBindTexture( GL_TEXTURE_2D, tex->getHandle() );
		glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( 0.f, 1.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v0.ptr() );

			glTexCoord2f( 1.f, 1.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v1.ptr() );

			glTexCoord2f( 1.f, 0.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v2.ptr() );

			glTexCoord2f( 0.f, 0.f );
			glNormal3fv( norm.ptr() );
			glVertex3fv( v3.ptr() );                        
		glEnd ();
	}

	void renderCellOverlay( const Vec4f colour,  const Vec3f &norm, const Vec3f &v0, 
				const Vec3f &v1, const Vec3f &v2, const Vec3f &v3  ) {
		glBegin ( GL_TRIANGLE_FAN );
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v0.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v1.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v2.ptr());
			glNormal3fv(norm.ptr());
			glColor4fv( colour.ptr() );
			glVertex3fv(v3.ptr());                        
		glEnd ();
	}
};

}}

#endif