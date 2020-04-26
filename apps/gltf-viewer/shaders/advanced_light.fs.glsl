#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

uniform vec3 uLightDirection;
uniform vec3 uLightIntensity;
uniform vec3 uEmissiveFactor;

uniform vec4 uBaseColorFactor;

uniform float uMetallicFactor;
uniform float uRoughnessFactor;
uniform float uNormalScale;


uniform sampler2D uBaseColorTexture;
uniform sampler2D uNormalTexture;
uniform sampler2D uMetallicRoughnessTexture;
uniform sampler2D uEmissiveTexture;


struct PointLight
{
  vec3 position;
  vec3 intensity;
  float attenuationDistance;
  float constantAttenuator;
  float linearAttenuator;
  float quadraticAttenuator;
};

struct SpotLight
{
  vec3 LightPosition;
  vec3 LightIntensity;
  vec3 LightDirection;
  float CutOff;
  float OuterCutOff;
  float DistAttenuation;
  float constantAttenuator;
  float linearAttenuator;
  float quadraticAttenuator;
};

//lights
#define NB_POINT_LIGHTS 4
uniform PointLight pointLights[NB_POINT_LIGHTS];

uniform SpotLight spotlight;


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

vec3 directionalLightRender()
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

  // Metallic value is from the blue of the vector
  vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);
  // Roughness value is from the green of the vector
  float roughness = uRoughnessFactor * metallicRougnessFromTexture.g;

  // From
  // https://github.com/KhronosGroup/glTF/tree/master/specification/2.0#appendix-b-brdf-implementation
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
  // shlickFactor value is used

  vec3 F = F0 + (vec3(1) - F0) * shlickFactor;

  vec3 f_diffuse = (1 - F) * diffuse;

  // For code visibility
  float sqrAlpha = alpha * alpha;
  // Denomitator could be 0
  float visDenominator =
      NdotL * sqrt((NdotV * NdotV) * (1 - sqrAlpha) + sqrAlpha) +
      NdotV * sqrt((NdotL * NdotL) * (1 - sqrAlpha) + sqrAlpha);
  float vis = visDenominator > 0. ? 0.5 / visDenominator : 0.0;

  float denom = M_PI * ((NdotH * NdotH) * (sqrAlpha - 1) + 1);
  float D = (sqrAlpha) / (M_PI * (denom * denom));

  vec3 f_specular = F * vis * D;

  // Specular result (linear)
  return (f_diffuse + f_specular) * uLightIntensity * NdotL;
}



vec3 pointLightRender(PointLight pointL)
{

  vec3 N = normalize(vViewSpaceNormal);
  vec3 L = normalize(pointL.position - vViewSpacePosition);
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

  // Metallic value is from the blue of the vector
  vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);
  // Roughness value is from the green of the vector
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
  // shlickFactor value is used

  vec3 F = F0 + (vec3(1) - F0) * shlickFactor;

  vec3 f_diffuse = (1 - F) * diffuse;

  // For code visibility
  float sqrAlpha = alpha * alpha;
  // Denomitator could be 0
  float visDenominator =
      NdotL * sqrt((NdotV * NdotV) * (1 - sqrAlpha) + sqrAlpha) +
      NdotV * sqrt((NdotL * NdotL) * (1 - sqrAlpha) + sqrAlpha);
  float vis = visDenominator > 0. ? 0.5 / visDenominator : 0.0;

  float denom = M_PI * ((NdotH * NdotH) * (sqrAlpha - 1) + 1);
  float D = (sqrAlpha) / (M_PI * (denom * denom));

  vec3 f_specular = F * vis * D;
  
  //Attenuation
  float dist = distance(pointL.position, vViewSpacePosition);

  float attC = pointL.constantAttenuator;
  float attL = pointL.linearAttenuator;
  float attQ = pointL.quadraticAttenuator;

  // Visibility
  float attenuation = 1.f / (attC + attL * dist + attQ * dist * dist);
  return (f_diffuse + f_specular) * pointL.intensity *
                          NdotL * attenuation;
}

vec3 spotlightRender(SpotLight spotL)
{

  vec3 N = normalize(vViewSpaceNormal);
  vec3 L = normalize(spotL.LightPosition - vViewSpacePosition);
  vec3 V = normalize(-vViewSpacePosition);
  vec3 H = normalize(L + V);


 
  float theta = dot(L, normalize(-spotL.LightDirection));
  float epsilon = spotL.CutOff - spotL.OuterCutOff;

  float coneIntensity = clamp((theta - spotL.OuterCutOff) / epsilon, 0.0, 1.0);
  if (coneIntensity <= 0.0) { // Optimisation
    return vec3(0.0);
  }

  vec4 baseColor =
      SRGBtoLINEAR(texture(uBaseColorTexture, vTexCoords)) * uBaseColorFactor;
  vec4 metallicRougnessFromTexture =
      texture(uMetallicRoughnessTexture, vTexCoords);

  float NdotL = clamp(dot(N, L), 0, 1);
  float NdotV = clamp(dot(N, V), 0, 1);
  float NdotH = clamp(dot(N, H), 0, 1);
  float VdotH = clamp(dot(V, H), 0, 1);

  vec3 diffuse = baseColor.rgb * M_1_PI;

  // Metallic value is from the blue of the vector
  vec3 metallic = vec3(uMetallicFactor * metallicRougnessFromTexture.b);
  // Roughness value is from the green of the vector
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
  // shlickFactor value is used

  vec3 F = F0 + (vec3(1) - F0) * shlickFactor;
  vec3 f_diffuse = (1 - F) * diffuse;

  // For code visibility
  float sqrAlpha = alpha * alpha;
  // Denomitator could be 0
  float visDenominator =
      NdotL * sqrt((NdotV * NdotV) * (1 - sqrAlpha) + sqrAlpha) +
      NdotV * sqrt((NdotL * NdotL) * (1 - sqrAlpha) + sqrAlpha);
  float vis = visDenominator > 0. ? 0.5 / visDenominator : 0.0;

  float denom = M_PI * ((NdotH * NdotH) * (sqrAlpha - 1) + 1);
  float D = (sqrAlpha) / (M_PI * (denom * denom));

  vec3 f_specular = F * vis * D;
  

  float dist = length(spotL.LightPosition - vViewSpacePosition);

  //Visibility
  float attC = spotL.constantAttenuator;
  float attL = spotL.linearAttenuator;
  float attQ = spotL.quadraticAttenuator;

  float attenuation = 1.f / (attC + attL * dist + attQ * dist * dist);


  return (f_diffuse + f_specular) *
                        spotL.LightIntensity * NdotL * attenuation *
                        coneIntensity;
}

vec3 emissiveTextureRender() {
  vec4 emissiveTexture = SRGBtoLINEAR(texture(uEmissiveTexture, vTexCoords));
  return uEmissiveFactor * emissiveTexture.rgb;
}

void main()
{
  vec3 color = directionalLightRender();
  for (int i = 0; i < NB_POINT_LIGHTS; i++) {
    color += pointLightRender(pointLights[i]);
  }
  color += spotlightRender(spotlight);

  color += emissiveTextureRender();

  fColor = LINEARtoSRGB(color);
}