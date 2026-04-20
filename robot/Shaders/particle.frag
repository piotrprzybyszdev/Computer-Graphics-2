#version 460

layout(set = 0, binding = 2) uniform sampler2D u_Textures;

layout (location = 0) in vec2 v_UV;
layout (location = 1) in float v_Alpha;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const vec4 color = texture(u_Textures, v_UV);
    o_FragColor = vec4(color.rgb, v_Alpha * color.a);
}
