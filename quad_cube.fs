#version 330 core

in vec2 texCoord;

out vec4 FragColor;

uniform samplerCube texture1;
uniform vec3 direction;


void main(){
    vec2 sampleTex = 2 * texCoord - vec2(1.0);
    vec3 sampleVec;
    if(direction.x != 0){
        sampleVec = vec3(direction.x, sampleTex.y, direction.x * sampleTex.x);
    }
    if(direction.y != 0){
        sampleVec = vec3(sampleTex.x, direction.y, direction.y * sampleTex.y);
    }
    if(direction.z != 0){
        sampleVec = vec3( - sampleTex.x * direction.z, sampleTex.y, direction.z);
    }
    //view.x * x + view.y * y + view.z * z 
    //FragColor = textureLod(texture1, vec3(texCoord, 1), 1.2);

    vec4 color = texture(texture1, normalize(sampleVec)).rgb;
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));
    FragColor = vec4(color, 1.0);
}