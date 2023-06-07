//Hummingbird PBR fragment shader

#version 450

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in vec4 inColor0;

layout (set = 0, binding = 0) uniform UBO {
	mat4 projection;
	mat4 model;
	mat4 view;
	vec3 camPos;
} ubo;

layout (set = 0, binding = 1) uniform UBOParams {
	vec4 lightDir;
	float exposure;
	float gamma;
	float prefilteredCubeMipLevels;
	float scaleIBLAmbient;
	float debugViewInputs;
	float debugViewEquation;
	vec3 lightColor;
} uboParams;

layout (set = 0, binding = 2) uniform samplerCube samplerIrradiance;
layout (set = 0, binding = 3) uniform samplerCube prefilteredMap;
layout (set = 0, binding = 4) uniform sampler2D samplerBRDFLUT;

layout (set = 1, binding = 0) uniform sampler2D colorMap;
layout (set = 1, binding = 1) uniform sampler2D physicalDescriptorMap;
layout (set = 1, binding = 2) uniform sampler2D normalMap;
layout (set = 1, binding = 3) uniform sampler2D aoMap;
layout (set = 1, binding = 4) uniform sampler2D emissiveMap;

layout (push_constant) uniform Material {
	vec4 baseColorFactor;
	vec4 emissiveFactor;
	vec4 diffuseFactor;
	vec4 specularFactor;
	float workflow;
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;	
	int occlusionTextureSet;
	int emissiveTextureSet;
	float metallicFactor;	
	float roughnessFactor;	
	float alphaMask;	
	float alphaMaskCutoff;
} material;

layout (location = 0) out vec4 outColor;

struct PBRData
{
	float NdotL;                  // cos angle between normal and light direction
	float NdotV;                  // cos angle between normal and view direction
	float NdotH;                  // cos angle between normal and half vector
	float LdotH;                  // cos angle between light direction and half vector
	float VdotH;                  // cos angle between view direction and half vector
	float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
	float metalness;              // metallic value at the surface
	vec3 reflectance0;            // full reflectance color (normal incidence angle)
	vec3 reflectance90;           // reflectance color at grazing angle
	float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
	vec3 diffuseColor;            // color contribution from diffuse lighting
	vec3 specularColor;           // color contribution from specular lighting
};

const float PI = 3.141592653589793;
const float MinRoughness = 0.04;

vec3 Diffuse(vec3 diffuseColor/*PBRData data*/){
    return diffuseColor / PI;
}

void main()
{
    PBRData data;
    vec4 baseColor;
    if (material.baseColorTextureSet > -1) {
		baseColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) * material.baseColorFactor;
	} else {
		baseColor = material.baseColorFactor;
	}

    float metallic;
    float perceptualRoughness = material.roughnessFactor;
	metallic = material.metallicFactor;
	if (material.physicalDescriptorTextureSet > -1) {
		// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
		vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
		perceptualRoughness = mrSample.g * perceptualRoughness;
		metallic = mrSample.b * metallic;
	} else {
		perceptualRoughness = clamp(perceptualRoughness, MinRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}

    vec3 f0 = vec3(0.04);
    vec3 diffuseColor = (vec3(1.0) - f0) * (1.0 - metallic) * baseColor.rgb;
    //diffuse
    vec3 diffuse = Diffuse(diffuseColor);
    
    //specular
	//vec3 F = specularReflection(pbrInputs);
	//float G = geometricOcclusion(pbrInputs);
	//float D = microfacetDistribution(pbrInputs);
    vec3 specular = vec3(0);//F*G*D/(4.0 * NdotL * NdotV);

    vec3 color = uboParams.lightColor * (diffuse + specular);

    outColor = vec4(color,baseColor.a);
}