#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;

uniform sampler2D uBaseColorTexture;
uniform vec4 uBaseColorFactor;

uniform float uMetallicFactor;
// Roughness
uniform float uRoughnessFactor;
uniform sampler2D uMetallicRoughnessTexture;
// Emissive
uniform sampler2D uEmissiveTexture;
uniform vec3 uEmissiveFactor;

out vec3 fColor;

// Constants
const float GAMMA = 2.2;
const float INV_GAMMA = 1. / GAMMA;
const float M_PI = 3.141592653589793;
const float M_1_PI = 1.0 / M_PI;

// We need some simple tone mapping functions
// Basic gamma = 2.2 implementation
// Stolen here:
// https://github.com/KhronosGroup/glTF-Sample-Viewer/blob/master/src/shaders/tonemapping.glsl

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 LINEARtoSRGB(vec3 color) { return pow(color, vec3(INV_GAMMA)); }

// sRGB to linear approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec4 SRGBtoLINEAR(vec4 srgbIn){ return vec4(pow(srgbIn.xyz, vec3(GAMMA)), srgbIn.w);}

void main()
{
    vec3 N = normalize(vViewSpaceNormal);
    vec3 L = uLightDirection;
    vec3 V = normalize(-vViewSpacePosition);
    vec3 H = normalize(L + V);

    vec4 baseColor =
        SRGBtoLINEAR(texture(uBaseColorTexture, vTexCoords)) * uBaseColorFactor;
    vec4 metallicRougnessFromTexture =
        texture(uMetallicRoughnessTexture, vTexCoords);
    
    float NdotL = clamp(dot(N, L), 0, 1);
    float NdotV = clamp(dot(N, V), 0, 1);
    float NdotH = clamp(dot(N, H), 0, 1);
    float VdotH = clamp(dot(V, H), 0, 1);

    vec3 diffuse = baseColor.rgb * M_1_PI;

    //Metallic value is from the blue of the vector
    vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);
    //Roughness value is from the green of the vector
    float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;

    //From https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-b-brdf-implementation
    vec3 dielectricSpecular = vec3(0.04, 0.04, 0.04);
    vec3 black = vec3(0.f, 0.f, 0.f);

    vec3 cdiff = mix(baseColor.rgb * (1 - dielectricSpecular.r), black, metallic);
    vec3 F0 = mix(vec3(dielectricSpecular), baseColor.rgb, metallic);
    float alpha = roughness * roughness;

    // Faster power of 5
    float baseShlickFactor = 1 - VdotH;
    float shlickFactor = baseShlickFactor * baseShlickFactor; // power 2
    shlickFactor *= shlickFactor;                             // power 4
    shlickFactor *= baseShlickFactor;                         // power 5
    //shlickFactor value is used

    vec3 F = F0 + (vec3(1) - F0)  * shlickFactor;
    
    vec3 f_diffuse = (1 - F) * diffuse;

    //For code visibility
    float sqrAlpha = alpha * alpha;
    //Denomitator could be 0
    float visDenominator =
        NdotL * sqrt((NdotV * NdotV) * (1 - sqrAlpha) + sqrAlpha) +
        NdotV * sqrt((NdotL * NdotL) * (1 - sqrAlpha) + sqrAlpha);
    float vis = visDenominator > 0. ? 0.5 / visDenominator : 0.0;

    float denom = M_PI * ((NdotH * NdotH) * (sqrAlpha -1) + 1);
    float D = (sqrAlpha) / (M_PI * (denom * denom));
    
    vec3 f_specular = F * vis * D;
    
    //Specular result
    vec3 color = (f_diffuse + f_specular) * uLightIntensity * NdotL;

    vec3 emissive = SRGBtoLINEAR(texture2D(uEmissiveTexture, vTexCoords)).rgb *
                    uEmissiveFactor;
    //Texture self-lighting
    color += emissive;

    fColor = LINEARtoSRGB(color);
}