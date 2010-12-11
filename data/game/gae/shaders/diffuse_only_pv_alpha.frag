// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner & James McCulloch
//
//  GPL V2, see source/licence.txt
// ==============================================================

#version 110

varying float diffuseIntensity, meshAlpha, fogFactor;

uniform sampler2D baseTexture;

void main()
{
	// sample textures
	vec4 texDiffuseBase = texture2D(baseTexture, vec2(gl_TexCoord[0].st));
	
	// light components
	vec4 diffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * diffuseIntensity;
	vec4 ambient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	
	// alpha
	float alpha = min(diffuse.a + ambient.a, texDiffuseBase.a);
	
	// final colour
	vec4 finalColour = vec4((ambient.rgb + diffuse.rgb) * texDiffuseBase.rgb, alpha);
	gl_FragColor = mix(gl_Fog.color, finalColour, fogFactor);
}
