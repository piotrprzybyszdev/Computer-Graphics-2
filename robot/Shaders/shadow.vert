#version 460

layout (location = 0) in uvec4 v_Edge;

layout (location = 0) out uvec4 o_Edge;

void main()
{
    gl_Position = vec4(vec3(0.0f), 1.0f);
    o_Edge = v_Edge;
}