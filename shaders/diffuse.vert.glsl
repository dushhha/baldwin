#version 460
#pragma shader_stage(vertex)

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "common_structs.glsl"

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outNormal;

layout(set = 0, binding = 0) uniform SceneBuffer {
	SceneData sceneData;
};
layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};
layout(push_constant) uniform PushConstants
{	
	mat4 worldMatrix;
	VertexBuffer vertexBuffer;
} pushConstants;

void main() 
{
	Vertex v = pushConstants.vertexBuffer.vertices[gl_VertexIndex];
	gl_Position = sceneData.viewproj * pushConstants.worldMatrix * vec4(v.pos, 1.0f);
	outColor = v.color.rgb;
	outNormal = v.normal.rgb;
}
