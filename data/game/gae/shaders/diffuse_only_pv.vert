// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner & James McCulloch
//
//  GPL V2, see source/licence.txt
// ==============================================================

#version 110

varying float diffuseIntensity, meshAlpha;
varying float fogFactor;

void main()
{	
	gl_TexCoord[0] = gl_MultiTexCoord0;

	vec3 lightDir = normalize(vec3(gl_LightSource[0].position));
	vec3 normal = normalize(gl_NormalMatrix * gl_Normal);
	
	// diffuse shading
	diffuseIntensity = max(dot(lightDir, normal), 0.0);
	
	// fog
	vec3 posFromCamera = vec3(gl_ModelViewMatrix * gl_Vertex);
	float dist = length(posFromCamera); // distance from camera
	float fog = gl_Fog.density;
	const float log_2 = 1.442695;
	fogFactor = exp2(-fog * fog * dist * dist * log_2);
	fogFactor = clamp(fogFactor, 0.0, 1.0);	
	
	// pass-on mesh alpha
	meshAlpha = gl_Color.a;
	
	// determine pixel position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
