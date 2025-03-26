#version 460
#pragma shader_stage(fragment)

#extension GL_GOOGLE_include_directive : require

#include "common_structs.glsl"

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec3 inNormal;
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 0) uniform SceneBuffer {
    SceneData sceneData;
};
void main() 
{
    vec3 ambient = sceneData.ambientColor.rgb * sceneData.ambientColor.a;
    float nDotL = max(dot(inNormal, sceneData.sunlightDirection.rgb), 0.0);
    vec3 direct = nDotL * sceneData.sunlightColor.rgb * sceneData.sunlightColor.a;
    vec3 finalColor = ambient + direct;

    outFragColor = vec4(finalColor, 1.0f);
}
