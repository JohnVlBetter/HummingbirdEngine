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

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal()
{
	// Perturb normal, see http://www.thetenthplanet.de/archives/1180
	vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;

	vec3 q1 = dFdx(inWorldPos);
	vec3 q2 = dFdy(inWorldPos);
	vec2 st1 = dFdx(inUV0);
	vec2 st2 = dFdy(inUV0);

	vec3 N = normalize(inNormal);
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);
	vec3 B = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

const float PI = 3.141592653589793;
const float MinRoughness = 0.04;

vec3 Diffuse(vec3 diffuseColor){
    return diffuseColor / PI;
}

vec3 F_Schlick(float VdotH, float metallic, vec3 baseColor){
	vec3 F0 = mix(vec3(MinRoughness), baseColor, metallic);
	return F0 + (1 - F0) * pow(clamp(1.0 - VdotH, 0.0, 1.0) , 5.0);
}

float D_GGX(float NdotH, float roughness){
    float roughness2 = roughness * roughness;
	float NdotH2 = NdotH * NdotH;
	float tmp = NdotH2 * (roughness2 - 1.0) + 1.0;
	float denominator = PI * tmp * tmp;
	denominator = max(denominator, 0.00001);
	return roughness2 / denominator;
}

float G_SchlickGGX(float NdotV, float k)
{
    float denominator = NdotV * (1.0 - k) + k;
    return NdotV / denominator;
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
	float tmp = (roughness + 1) / 2;
	float alpha = tmp * tmp;
	float k = alpha / 2;
    float ggx1 = G_SchlickGGX(NdotV, k);
    float ggx2 = G_SchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

void main()
{
    vec4 baseColor;
    if (material.baseColorTextureSet > -1) {
		baseColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) * material.baseColorFactor;
	} else {
		baseColor = material.baseColorFactor;
	}

    float metallic;
    float roughness = material.roughnessFactor;
	metallic = material.metallicFactor;
	if (material.physicalDescriptorTextureSet > -1) {
		// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
		// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
		vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);
		roughness = mrSample.g * roughness;
		metallic = mrSample.b * metallic;
	} else {
		roughness = clamp(roughness, MinRoughness, 1.0);
		metallic = clamp(metallic, 0.0, 1.0);
	}
	
	vec3 n = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);
	vec3 v = normalize(ubo.camPos - inWorldPos);    // view dir
	vec3 l = normalize(uboParams.lightDir.xyz);     // light dir
	vec3 h = normalize(l+v);                        // half vector

	float VdotH = clamp(dot(v, h), 0.0, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float NdotL = clamp(dot(n, l), 0.001, 1.0);
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    
    //specular
	vec3 F = F_Schlick(VdotH, metallic, baseColor.rgb);
	float G = G_Smith(NdotV, NdotL, roughness);
	float D = D_GGX(NdotH, roughness);
    vec3 specular = (F * G * D)/(4.0 * NdotL * NdotV);

    //diffuse
	vec3 kd = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuseColor = kd * baseColor.rgb;
    vec3 diffuse = Diffuse(diffuseColor);

    vec3 color = NdotL * uboParams.lightColor * (diffuse + specular);

	// AO
	if (material.occlusionTextureSet > -1) {
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		color *= ao;
	}

	//emissive
	if (material.emissiveTextureSet > -1) {
		vec3 emissive = texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb;
		color += emissive;
	}

    outColor = vec4(color, baseColor.a);
}