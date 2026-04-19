#version 460

layout (location = 0) in vec4 v_Position;
layout (location = 1) in vec4 v_Normal;

layout (location = 0) out vec4 o_FragColor;

void main()
{
    const float ka = 0.1f;

    o_FragColor = vec4(vec3(ka), 1.0f);
}
