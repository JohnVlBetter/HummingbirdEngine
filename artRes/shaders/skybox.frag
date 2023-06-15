#version 450

layout (binding = 2) uniform samplerCube samplerEnv;

layout (set = 0, location = 0) in vec3 inUVW;

layout (set = 0, location = 0) out vec4 outColor;

layout (set = 0, binding = 1) uniform UBOParams {
	vec4 _pad0;
	float exposure;
	float gamma;
} uboParams;

#define SRGB 1

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

vec4 Tonemap(vec4 color)
{
	vec3 outcol = Tonemap_F(color.rgb * uboParams.exposure);
	outcol = outcol * (1.0f / Tonemap_F(vec3(11.2f)));	
	//gamma correction
	return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);
}

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

void main() 
{
	vec3 color = SRGBtoLiner(Tonemap(textureLod(samplerEnv, inUVW, 1.5))).rgb;	
	outColor = vec4(color, 1.0);
}