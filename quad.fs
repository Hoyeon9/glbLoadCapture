#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform sampler2D texture1;


void main(){
    vec3 color = texture(texture1, texCoord).rgb;
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}