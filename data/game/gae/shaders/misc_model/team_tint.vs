//     This file is a part of The Glest Advanced Engine.
//     Copyright (C) 2010-2011 Nathan Turner & James McCulloch
//     GPL V2, see source/licence.txt

#version 110

uniform float            gae_AlphaThreshold;
uniform int              gae_LightCount;
uniform int              gae_IsUsingFog;
uniform int              gae_MeshFlags;
uniform vec3             gae_TeamColour;
uniform sampler2D        gae_DiffuseTex;

varying float            fogFactor;
varying vec3             lightColour;
varying float            meshAlpha;

// calc diffuse and ambient terms from light source 0 (always directional)
void doPrimaryLight(in vec3 normal, inout vec3 diffuse, inout vec3 ambient) {
	vec3 lightDir = normalize(vec3(gl_LightSource[0].position)); // direction of main light source
	float diffuseIntensity = max(dot(lightDir, normal), 0.0);         // calculate diffuse term

	diffuse += gl_LightSource[0].diffuse.rgb * gl_FrontMaterial.diffuse.rgb * gl_Color.rgb
					* diffuseIntensity;
	ambient += gl_LightSource[0].ambient.rgb * gl_FrontMaterial.ambient.rgb;
}

// calculate diffuse and ambient terms for the point light at index i
void doPointLight(in int i, in vec3 ecPos, in vec3 normal, inout vec3 diffuse, inout vec3 ambient) {
	// TODO: Copy code from the orange book ;)
	vec3 VP = gl_LightSource[i].position.xyz - ecPos;
	float distance = length(VP);
	VP = normalize(VP);
	
	float attenuation = 1.0 / (gl_LightSource[i].constantAttenuation
			+ gl_LightSource[i].linearAttenuation * distance
			+ gl_LightSource[i].quadraticAttenuation * distance * distance);
			
	float diffuseIntensity = max(dot(normal, VP), 0.0);
	
	ambient += gl_LightSource[i].ambient.rgb * attenuation;
	diffuse += gl_LightSource[i].diffuse.rgb * diffuseIntensity * attenuation;
}

// calculate diffuse and ambient light intensity for all active point lights
// Warning: Assumes gae_LightCount >= 2
void doActivePointLights(in vec3 normal, inout vec3 diffuse, inout vec3 ambient) {
	vec4 ecVec = gl_ModelViewMatrix * gl_Vertex;
	vec3 ecPos = ecVec.xyz / ecVec.w;
	doPointLight(1, ecPos, normal, diffuse, ambient);
	if (gae_LightCount > 2) {
		doPointLight(2, ecPos, normal, diffuse, ambient);
	}
	if (gae_LightCount > 3) {
		doPointLight(3, ecPos, normal, diffuse, ambient);
	}
	if (gae_LightCount > 4) {
		doPointLight(4, ecPos, normal, diffuse, ambient);
	}
	if (gae_LightCount > 5) {
		doPointLight(5, ecPos, normal, diffuse, ambient);
	}					
	// for loops in GLSL are not well supported...
	//for (int i=1; i < gae_LightCount; ++i) {
	//	doPointLight(i, ecPos, normal, diffuse, ambient);
	//}
}

// compute fog factor
float calculateFogFactor() {
	float fog;
	if (gae_IsUsingFog == 0) {
		fog = 1.0;
	} else {
		float dist = length(vec3(gl_ModelViewMatrix * gl_Vertex)); // distance from camera
		const float one_divide_ln_2 = 1.44269504088896341;
		fog = exp2(-gl_Fog.density * gl_Fog.density * dist * dist * one_divide_ln_2);
	}
	return clamp(fog, 0.0, 1.0);	
}

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
