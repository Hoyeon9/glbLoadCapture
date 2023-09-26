#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D texture1;


void main(){
    //if(texture(texture1, texCoord).a < 0.1)
    //    discard;
    FragColor = texture(texture1, texCoord);
}