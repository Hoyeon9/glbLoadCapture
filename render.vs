#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec3 aTangent;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out vec3 Tangent;

uniform mat4 mvp;
uniform mat4 model;
uniform mat4 view;
uniform mat3 normalMatrix;

void main(){
	WorldPos = vec3(model * vec4(aPos, 1.0f));
	Normal = normalMatrix * aNormal;
	Tangent = normalMatrix * aTangent;
	TexCoords = aTexCoord;

	gl_Position = mvp * vec4(aPos, 1.0f);
}