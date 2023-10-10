#version 330 core

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

struct Material{
	sampler2D texture_diffuse1;
	sampler2D texture_specular1;
	sampler2D texture_normal1;
	sampler2D texture_albedo1;
	sampler2D texture_metallic1;
	sampler2D texture_roughness1;
	sampler2D texture_emissive1;
};
out vec4 FragColor;
  
uniform vec3 camPos;
uniform vec3 camView;
uniform Material material;
uniform int lightNum;
uniform int albedoChannels;

uniform int renderMode;
  

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D   brdfLUT;

const float PI = 3.14159265359;

vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 getNormalFromMap();

void main()
{
    vec3 albedo = pow(texture(material.texture_diffuse1, TexCoords).rgb, vec3(2.2));
    float metallic = texture(material.texture_metallic1, TexCoords).b;
    float roughness = texture(material.texture_roughness1, TexCoords).g;
    float ao = texture(material.texture_metallic1, TexCoords).r;

    if(renderMode > 0){
        vec3 color;
        if (renderMode == 1){
            color = albedo;
            if(albedoChannels == 4){
                FragColor = vec4(color, texture(material.texture_diffuse1, TexCoords).a);
                return;
            }
        } else if (renderMode == 2){
            color = texture(material.texture_metallic1, TexCoords).rgb;
        } else if (renderMode == 3){
            color = vec3(metallic, metallic, metallic);
        } else if (renderMode == 4){
            color = vec3(roughness, roughness, roughness);
        } else if (renderMode == 5){
            color = vec3(ao, ao, ao);
        }
        FragColor = vec4(color, 1.0f);
        return;
    }
    

    vec3 N = getNormalFromMap();//normalize(Normal); 
    vec3 V = normalize(camPos + normalize(camView) * 0.3 - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    //Spot light sources
    vec3 Lo = vec3(0.0);
    for(int i=0; i<lightNum; i++){
        vec3 L = normalize(lightPositions[i] - WorldPos);
        vec3 H = normalize(V+L);
        float distance = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0/(distance * distance);
        vec3 radiance = lightColors[i] * attenuation;

        //BRDF(Cook-Torrance)
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; //prevent a divide by zero
        vec3 specular = numerator / denominator;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }


    //IBL
    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
  
    vec3 ambient = (kD * diffuse + specular) * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    //color = texture(material.texture_metallic1, TexCoords).rgb;
    vec3 emissive;

    if(albedoChannels == 3)
        FragColor = vec4(color, 1.0f);
    else if(albedoChannels == 4)
        FragColor = vec4(color, texture(material.texture_diffuse1, TexCoords).a);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}
vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(material.texture_normal1, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(WorldPos);
    vec3 Q2  = dFdy(WorldPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}