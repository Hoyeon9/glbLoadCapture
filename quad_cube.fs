#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform samplerCube cubeMap;


void main(){
    FragColor = texture(cubeMap, vec3(texCoord, 1));
}