#version 460

layout (location = 0) in vec4 v_Normal;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    o_FragColor = v_Normal;
}
