//     This file is a part of The Glest Advanced Engine.
//     Copyright (C) 2010-2011 Nathan Turner & James McCulloch
//     GPL V2, see source/licence.txt

#version 110

// interpolated vars
varying vec3     normal;      // normal vector in eye space
varying vec3	 esLightDir;  // light direction vector in eye space
varying vec3	 tsLightDir;  // light direction vector in tangent space
varying vec3     meshColour;
//varying vec3     eyeVec;
varying float    fogFactor;
varying float    meshAlpha;

// constant vars
uniform float      gae_AlphaThreshold;
uniform int        gae_IsUsingFog;
uniform int        gae_UsesTeamColour;
uniform int        gae_HasNormalMap;
uniform vec3       gae_TeamColour;
uniform sampler2D  gae_DiffuseTex;
uniform sampler2D  gae_NormalMap;

// vertex attributes
attribute vec3     gae_Tangent;

void main() {
	gl_TexCoord[0] = gl_MultiTexCoord0;
	meshColour = gl_Color.rgb;
	meshAlpha = gl_Color.a;
	
	normal = gl_NormalMatrix * gl_Normal; // pass normal to frag shader
	esLightDir = normalize(gl_LightSource[0].position.xyz); // light dir in eye space

	if (gae_HasNormalMap == 1) {
		// Build TBN vectors
		// for normal mapping : from http://www.ozone3d.net/tutorials/bump_mapping_p4.php
		vec3 n = normal;
		vec3 t = normalize(gl_NormalMatrix * gae_Tangent);
		vec3 b = cross(n, t);

		// Transform light dir to tangent space
		vec3 vertex = vec3(gl_ModelViewMatrix * gl_Vertex);
		vec3 tmpVec = normalize(gl_LightSource[0].position.xyz - vertex);
		tsLightDir = vec3(dot(tmpVec, t), dot(tmpVec, b), dot(tmpVec, n)); // light dir in tangent space
	}
	// Transform eye pos to tangent space (will need for spec mapping)
	//tmpVec = -vertex;
	//eyeVec.x = dot(tmpVec, t);
	//eyeVec.y = dot(tmpVec, b);
	//eyeVec.z = dot(tmpVec, n);

	// fog
	//if (gae_IsUsingFog == 1) {
		float dist = length(vec3(gl_ModelViewMatrix * gl_Vertex)); // distance from camera
		dist *= float(gae_IsUsingFog);
		const float log_2 = 1.442695;
		fogFactor = exp2(-gl_Fog.density * gl_Fog.density * dist * dist * log_2);
		fogFactor = clamp(fogFactor, 0.0, 1.0);	
	//} else {
	//	fogFactor = 1.f;
	//}
	
	// Transform vert
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
