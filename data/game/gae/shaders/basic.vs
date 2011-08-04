//     This file is a part of The Glest Advanced Engine.
//     Copyright (C) 2010-2011 Nathan Turner & James McCulloch
//     GPL V2, see source/licence.txt

#version 110

uniform float            gae_AlphaThreshold;
uniform int              gae_LightCount;
uniform int              gae_IsUsingFog;
uniform int              gae_UsesTeamColour;
uniform vec3             gae_TeamColour;
uniform sampler2D        gae_DiffuseTex;

varying float            fogFactor;
varying vec3             lightColour;
varying float            meshAlpha;

// calc diffuse and ambient terms from light source 0 (always directional)
void doPrimaryLight(in vec3 normal, inout vec3 diffuse, inout vec3 ambient);

// calculate diffuse and ambient terms for the point light at index i
void doPointLight(in int i, in vec3 ecPos, in vec3 normal, inout vec3 diffuse, inout vec3 ambient);

// calculate diffuse and ambient light intensity for all active point lights
// Warning: Assumes gae_LightCount >= 2
void doActivePointLights(in vec3 normal, inout vec3 diffuse, inout vec3 ambient);

// compute fog factor
float calculateFogFactor();

void main() {
	// pass-on tex-coord & mesh colour
	gl_TexCoord[0] = gl_MultiTexCoord0;
	meshAlpha = gl_Color.a;

	// Lighting (diffuse only, everything currently has Material.specular set to (0, 0, 0, 1))
	
	// convert normal to eye space
	vec3 normal = gl_NormalMatrix * gl_Normal;
	
	// diffuse and ambient accumulators
	vec3 diffuse = vec3(0.0);
	vec3 ambient = vec3(0.0);

	// Light source 0, sun/moon
	doPrimaryLight(normal, diffuse, ambient);

	// point lights
	if (gae_LightCount > 1) {
		doActivePointLights(normal, diffuse, ambient);
	}
	
	// add global ambient
	ambient += gl_FrontLightModelProduct.sceneColor.rgb;
	
	// sum light colour
	lightColour = clamp(ambient + diffuse, 0.0, 1.0);

	// fog
	fogFactor = calculateFogFactor();

	// Transform vertex
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
