#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform samplerCube texture1;


void main(){
    //FragColor = textureLod(texture1, vec3(texCoord, 1), 1.2);
    FragColor = texture(texture1, vec3(texCoord, 1));
}