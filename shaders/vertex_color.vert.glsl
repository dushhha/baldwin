#version 460
#extension GL_EXT_buffer_reference : require
#pragma shader_stage(vertex)

layout (location = 0) out vec3 outColor;

struct Vertex {
	vec3 pos;
	float uv1;
	vec3 normal;
	float uv2;
	vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
	Vertex vertices[];
};
layout(push_constant) uniform constants
{	
	mat4 worldMatrix;
	uint vertexOffset;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex + PushConstants.vertexOffset];
	gl_Position = PushConstants.worldMatrix * vec4(v.pos, 1.0f);
	outColor = v.normal.rgb;
}
