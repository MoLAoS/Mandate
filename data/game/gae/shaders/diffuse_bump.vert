// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner
//
//  GPL V2, see source/licence.txt
// ==============================================================

in vec3 tangent;
out vec3 normal, lightDir, eyeVec;

// TODO replace with the precalculated version
vec3 getTangent()
{
	vec3 tangent;
	vec3 c1 = cross(gl_Normal, vec3(0.0, 0.0, 1.0));
	vec3 c2 = cross(gl_Normal, vec3(0.0, 1.0, 0.0));
	
	if (length(c1) > length(c2))
	{
		tangent = c1;
	}
	else
	{
		tangent = c2;
	}

	return normalize(tangent);
}

void main()
{	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;

	lightDir = normalize(vec3(gl_LightSource[0].position));
	normal = normalize(gl_NormalMatrix * gl_Normal);
	
	// determine pixel position
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	/*
	// for normal mapping : from http://www.ozone3d.net/tutorials/bump_mapping_p4.php
	vec3 n = normalize(gl_NormalMatrix * gl_Normal);
	vec3 t = normalize(gl_NormalMatrix * getTangent());//vTangent);
	vec3 b = cross(n, t);
	
	vec3 vertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	vec3 tmpVec = gl_LightSource[0].position.xyz - vertex;

	// transform light to tangent space
	lightDir.x = dot(tmpVec, t);
	lightDir.y = dot(tmpVec, b);
	lightDir.z = dot(tmpVec, n);
	*/
}
