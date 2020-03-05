#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

//Light direction and intensity (color intensity)
uniform vec3 uLightDirection;//Normalized
uniform vec3 uLightIntensity;

out vec3 fColor;

void main()
{
   vec3 viewSpaceNormal = normalize(vViewSpaceNormal);
   fColor = vec3(1. / 3.141592) * uLightIntensity * dot(viewSpaceNormal, uLightDirection);
}