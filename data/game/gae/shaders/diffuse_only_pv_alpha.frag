// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner
//
//  GPL V2, see source/licence.txt
// ==============================================================

#version 110

varying float diffuseIntensity, colourAlpha;

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
	gl_FragColor = vec4((0.5 * diffuse.rgb + 1.5 * ambient.rgb) * texDiffuseBase.rgb, alpha);
}
