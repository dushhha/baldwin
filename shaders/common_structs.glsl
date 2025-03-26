struct Vertex 
{
    vec3 pos;
    float uv1;
    vec3 normal;
    float uv2;
    vec4 color;
};

struct SceneData
{
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor; // w for intensity
    vec4 sunlightDirection;
    vec4 sunlightColor; // w for intensity
};
