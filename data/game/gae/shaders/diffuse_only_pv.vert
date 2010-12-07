// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner
//
//  GPL V2, see source/licence.txt
// ==============================================================

#version 110

varying float diffuseIntensity, colourAlpha;

void main()
{	
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec3 lightDir = normalize(vec3(gl_LightSource[0].position));
	vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
	
	// diffuse shading
	diffuseIntensity = max(dot(lightDir, normal), 0.0);

	// pass-on alpha component of glColor
	colourAlpha = gl_Color.a;
	
	// determine pixel position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
