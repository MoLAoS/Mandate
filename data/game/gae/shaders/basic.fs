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

void main() {
	// sample texture
	vec4 colour = texture2D(gae_DiffuseTex, vec2(gl_TexCoord[0].st));

	// Mix-in team colour (if team colour mesh)
	if (gae_UsesTeamColour == 1) {
		// interpolate the base and team colour according to the base alpha
		colour = vec4(mix(gae_TeamColour, colour.rgb, colour.a), 1.0);
	}

	// Add light
	colour = vec4(lightColour * colour.rgb, meshAlpha * colour.a);
	
	// below alpha threshold ?
	if (colour.a < gae_AlphaThreshold) {
		discard;
	}
	gl_FragColor = mix(gl_Fog.color, colour, fogFactor);
}
