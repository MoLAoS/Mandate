//     This file is a part of The Glest Advanced Engine.
//     Copyright (C) 2010-2011 Nathan Turner & James McCulloch
//     GPL V2, see source/licence.txt

#version 110

uniform float      gae_AlphaThreshold;
uniform int        gae_IsUsingFog;
uniform int        gae_UsesTeamColour;
uniform int        gae_HasNormalMap;
uniform vec3       gae_TeamColour;
uniform sampler2D  gae_DiffuseTex;
uniform sampler2D  gae_NormalMap;

varying vec3     normal;      // normal vector in eye space
varying vec3	 esLightDir;  // light direction vector in eye space
varying vec3	 tsLightDir;  // light direction vector in tangent space

varying vec3     meshColour;
//varying vec3     eyeVec;

varying float    fogFactor;
varying float    meshAlpha;


void main() {
	// sample diffuse texture
	vec4 colour = texture2D(gae_DiffuseTex, vec2(gl_TexCoord[0].st));

	if (gae_UsesTeamColour == 1) {
		// interpolate the base and team colour according to the base alpha
		colour = vec4(mix(gae_TeamColour, colour.rgb, colour.a), 1.0);
	}
	
	// diffuse shading
	vec3 normal = normalize(normal);
	float diffuseIntensity = max(dot(esLightDir, normal), 0.0);
	vec3 diffuse;
	if (gae_HasNormalMap == 1) {
		// calculate intensity from texture normal
		vec3 texNormalMap = texture2D(gae_NormalMap, vec2(gl_TexCoord[0].st)).xyz;
		vec3 n = normalize(texNormalMap * 2.0 - 1.0);
		float bump = max(dot(tsLightDir, n), 0.0);
		diffuse = gl_FrontLightProduct[0].diffuse.rgb * (bump * 0.8 + diffuseIntensity * 0.2)
				* meshColour;
	} else {
		diffuse = gl_FrontLightProduct[0].diffuse.rgb * diffuseIntensity * meshColour;
	}
	vec3 ambient = gl_FrontLightProduct[0].ambient.rgb + gl_FrontLightModelProduct.sceneColor.rgb;
	vec3 lightColour = clamp(ambient + diffuse, 0.0, 1.0);
					
	// final colour
	vec4 finalColour = vec4(lightColour * colour.rgb, meshAlpha * colour.a);

	// below alpha threshold ?
	if (finalColour.a < gae_AlphaThreshold) {
		discard;
	}
	gl_FragColor = mix(gl_Fog.color, finalColour, fogFactor);
}
