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

#define SRGB 1

vec4 SRGBtoLiner(vec4 srgb)
{
	#ifdef SRGB
		vec3 bLess = step(vec3(0.04045),srgb.xyz);
		vec3 linOut = mix( srgb.xyz/vec3(12.92), pow((srgb.xyz+vec3(0.055))/vec3(1.055),vec3(2.4)), bLess );
		return vec4(linOut,srgb.w);;
	#else //SRGB
		return srgb;
	#endif //SRGB
}

vec3 GetNormal()
{
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

vec3 Tonemap_F(vec3 color)
{
	float A = 0.22;
	float B = 0.30;
	float C = 0.10;
	float D = 0.20;
	float E = 0.01;
	float F = 0.30;

	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

/*vec3 ACESToneMapping(vec3 color)
{
	float A = 2.51;
	float B = 0.03;
	float C = 2.43;
	float D = 0.59;
	float E = 0.14;

	return (color * (A * color + B)) / (color * (C * color + D) + E);
}*/

vec4 Tonemap(vec4 color)
{
	vec3 outcol = Tonemap_F(color.rgb * uboParams.exposure);
	outcol = outcol * (1.0f / Tonemap_F(vec3(11.2f)));	
	//gamma correction
	return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);
}

const float PI = 3.141592653589793;
const float MinRoughness = 0.04;

const float PBR_MR = 0.0;
const float PBR_SG = 1.0f;

// Gets metallic from specular glossiness workflow 
float ConvertToMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {
	float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
	float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);
	if (perceivedSpecular < MinRoughness) return 0.0;
	float a = MinRoughness;
	float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - MinRoughness) + perceivedSpecular - 2.0 * MinRoughness;
	float c = MinRoughness - perceivedSpecular;
	float d = max(b * b - 4.0 * a * c, 0.0);
	return clamp((-b + sqrt(d)) / (2.0 * a), 0.0, 1.0);
}

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
	denominator = max(denominator, 1e-6);
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

vec3 IBL(vec3 diffuseColor, vec3 specularColor, vec3 n, vec3 reflection, float roughness, float NdotV){
	vec3 diffuse = SRGBtoLiner(Tonemap(texture(samplerIrradiance,n))).rgb * diffuseColor;

	vec3 brdf = texture(samplerBRDFLUT, vec2(NdotV, 1.0 - roughness)).rgb;
	float lod = roughness * uboParams.prefilteredCubeMipLevels;
	vec3 prefilteredColor = SRGBtoLiner(Tonemap(textureLod(prefilteredMap, reflection, lod))).rgb;
	vec3 specular = prefilteredColor * (specularColor * brdf.r + brdf.g);

	return diffuse + specular;
}

void main()
{
    vec4 baseColor = material.baseColorFactor;
    float metallic;
    float roughness;

	if (material.alphaMask == 1.0f) {
		if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLiner(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		}
		if (baseColor.a < material.alphaMaskCutoff) discard;
	}

	if (material.workflow == PBR_MR) {
		roughness = material.roughnessFactor;
		metallic = material.metallicFactor;
		if (material.physicalDescriptorTextureSet > -1) {
			// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
			// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
			vec4 mrSample = SRGBtoLiner(texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1));
			roughness = mrSample.g * roughness;
			metallic = mrSample.b * metallic;
		} else {
			roughness = clamp(roughness, MinRoughness, 1.0);
			metallic = clamp(metallic, 0.0, 1.0);
		}

		//base color
    	if (material.baseColorTextureSet > -1) {
			baseColor = SRGBtoLiner(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;
		} else {
			baseColor = material.baseColorFactor;
		}
	}

	if (material.workflow == PBR_SG) {
		// specular glossiness converted to metallic roughness
		if (material.physicalDescriptorTextureSet > -1) {
			roughness = 1.0 - texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;
		} else {
			roughness = 0.0;
		}

		const float epsilon = 1e-6;

		vec4 diffuse = SRGBtoLiner(texture(colorMap, inUV0));
		vec3 specular = SRGBtoLiner(texture(physicalDescriptorMap, inUV0)).rgb;

		float maxSpecular = max(max(specular.r, specular.g), specular.b);

		// Convert metallic roughness from specular glossiness
		metallic = ConvertToMetallic(diffuse.rgb, specular, maxSpecular);

		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - MinRoughness) / max(1 - metallic, epsilon)) * material.diffuseFactor.rgb;
		vec3 baseColorSpecularPart = specular - (vec3(MinRoughness) * (1 - metallic) * (1 / max(metallic, epsilon))) * material.specularFactor.rgb;
		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);
	}
	
	baseColor *= inColor0;
	
	// Roughness is authored as perceptual roughness; as is convention,
	// convert to material roughness by squaring the perceptual roughness.
	roughness = roughness * roughness;
	
	vec3 n = (material.normalTextureSet > -1) ? GetNormal() : normalize(inNormal);
	vec3 v = normalize(ubo.camPos - inWorldPos);    // view dir
	vec3 l = normalize(uboParams.lightDir.xyz);     // light dir
	vec3 h = normalize(l+v);                        // half vector
	vec3 reflection = -normalize(reflect(v, n));
	reflection.y *= -1.0;

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

	vec3 specularColor = mix(vec3(MinRoughness), baseColor.rgb, metallic);
	//IBL vec3 IBL(vec3 diffuseColor, vec3 specularColor, vec3 n, vec3 reflection, float roughness, float NdotV)
	color += IBL(diffuseColor, specularColor, n, reflection, roughness, NdotV);

	// AO
	if (material.occlusionTextureSet > -1) {
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;
		color *= ao;
	}

	//emissive
	if (material.emissiveTextureSet > -1) {
		vec3 emissive = SRGBtoLiner(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb;
		color += emissive;
	}

    outColor = vec4(color, baseColor.a);

	if (uboParams.debugViewInputs > 0.0) {
		int index = int(uboParams.debugViewInputs);
		switch (index) {
			case 1:
				outColor.rgba = material.baseColorTextureSet > -1 ? texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) : vec4(1.0f);
				break;
			case 2:
				outColor.rgb = (material.normalTextureSet > -1) ? texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rgb : normalize(inNormal);
				break;
			case 3:
				outColor.rgb = (material.occlusionTextureSet > -1) ? texture(aoMap, material.occlusionTextureSet == 0 ? inUV0 : inUV1).rrr : vec3(0.0f);
				break;
			case 4:
				outColor.rgb = (material.emissiveTextureSet > -1) ? texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb : vec3(0.0f);
				break;
			case 5:
				outColor.rgb = texture(physicalDescriptorMap, inUV0).bbb;
				break;
			case 6:
				outColor.rgb = texture(physicalDescriptorMap, inUV0).ggg;
				break;
		}
		outColor = SRGBtoLiner(outColor);
	}

	// "None", "Diffuse", "F", "G", "D", "Specular"
	if (uboParams.debugViewEquation > 0.0) {
		int index = int(uboParams.debugViewEquation);
		switch (index) {
			case 1:
				outColor.rgb = diffuse;
				break;
			case 2:
				outColor.rgb = F;
				break;
			case 3:
				outColor.rgb = vec3(G);
				break;
			case 4: 
				outColor.rgb = vec3(D);
				break;
			case 5:
				outColor.rgb = specular;
				break;				
		}
	}

}