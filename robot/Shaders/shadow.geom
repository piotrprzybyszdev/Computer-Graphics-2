#version 460

struct RobotMesh
{
    uint PositionOffset;
    uint PositionCount;
    uint VertexOffset;
    uint VertexCount;
    uint TriangleOffset;
    uint TriangleCount;
    uint EdgeOffset;
    uint EdgeCount;
};

struct Light
{
    vec4 Position;
};

layout(set = 0, binding = 0) uniform CameraBuffer {
    mat4x4 u_CameraProjection;
    mat4x4 u_CameraView;
    vec4 u_CameraOrigin;
};

layout(set = 0, binding = 1) readonly buffer PositionBuffer {
    vec4 s_Positions[];
};

layout(set = 0, binding = 2) readonly buffer MeshBuffer {
    RobotMesh s_Meshes[];
};

layout(set = 0, binding = 3) uniform TransformBuffer {
    mat4x4 u_Transforms[6];
};

layout(set = 0, binding = 4) readonly buffer LightBuffer {
    Light s_Lights[];
};

layout(set = 0, binding = 5) readonly buffer IndexBuffer {
    uint s_Indices[];
};

layout(set = 0, binding = 6) readonly buffer PositionIndexBuffer {
    uint s_PositionIndices[];
};

layout(push_constant) uniform PushConstants {
    uint pc_MeshIndex;
};

layout (points) in;
layout (location = 0) in uvec4 v_Edge[1];

layout (triangle_strip, max_vertices = 18) out;

uvec3 getTrianglePositionIndices(uint triangleIdx)
{
    const uint indexOffset = (s_Meshes[pc_MeshIndex].TriangleOffset + triangleIdx) * 3;
    return uvec3(
        s_PositionIndices[s_Meshes[pc_MeshIndex].VertexOffset + s_Indices[indexOffset + 0]],
        s_PositionIndices[s_Meshes[pc_MeshIndex].VertexOffset + s_Indices[indexOffset + 1]],
        s_PositionIndices[s_Meshes[pc_MeshIndex].VertexOffset + s_Indices[indexOffset + 2]]
    );
}

void getTrianglePositions(out vec3 positions[3], in uvec3 positionIndices)
{
    positions[0] = s_Positions[s_Meshes[pc_MeshIndex].PositionOffset + positionIndices.x].xyz;
    positions[1] = s_Positions[s_Meshes[pc_MeshIndex].PositionOffset + positionIndices.y].xyz;
    positions[2] = s_Positions[s_Meshes[pc_MeshIndex].PositionOffset + positionIndices.z].xyz;
}

vec3 getNormal(in vec3 trianglePositions[3])
{
    const vec3 e1 = trianglePositions[1] - trianglePositions[0];
    const vec3 e2 = trianglePositions[2] - trianglePositions[0];

    return cross(e1, e2);
}

bool isFacing(in vec3 trianglePositions[3], vec3 dir)
{ 
    const vec3 normal = getNormal(trianglePositions);
    return dot(normal, dir) >= 0;
}

bool isFacingLight(in vec3 trianglePositions[3])
{
    const vec3 lightDir = s_Lights[0].Position.xyz - trianglePositions[0].xyz;

    return isFacing(trianglePositions, lightDir);
}

void emitLightCap(in vec3 trianglePositions[3], mat4x4 MVP)
{
    const float epsilon = 0.0001f;

    // render the front cap
    vec3 lightDir = (normalize(trianglePositions[0] - s_Lights[0].Position.xyz));
    gl_Position = MVP * vec4((trianglePositions[0] + lightDir * epsilon), 1.0);
    EmitVertex();

    lightDir = (normalize(trianglePositions[1] - s_Lights[0].Position.xyz));
    gl_Position = MVP * vec4((trianglePositions[1] + lightDir * epsilon), 1.0);
    EmitVertex();

    lightDir = normalize(trianglePositions[2] - s_Lights[0].Position.xyz);
    gl_Position = MVP * vec4((trianglePositions[2] + lightDir * epsilon), 1.0);
    EmitVertex();
    EndPrimitive();

    // render the back cap
    lightDir = (normalize(trianglePositions[2] - s_Lights[0].Position.xyz));
    gl_Position = MVP * vec4((trianglePositions[2] + lightDir * 1.0f), 1.0);
    EmitVertex();

    lightDir = (normalize(trianglePositions[1] - s_Lights[0].Position.xyz));
    gl_Position = MVP * vec4((trianglePositions[1] + lightDir * 1.0f), 1.0);
    EmitVertex();

    lightDir = normalize(trianglePositions[0] - s_Lights[0].Position.xyz);
    gl_Position = MVP * vec4((trianglePositions[0] + lightDir * 1.0f), 1.0);
    EmitVertex();
    EndPrimitive();

    // lightDir = trianglePositions[0] - s_Lights[0].Position.xyz;
    // gl_Position = MVP * vec4(lightDir, 0.0);
    // EmitVertex();
    // 
    // lightDir = trianglePositions[2] - s_Lights[0].Position.xyz;
    // gl_Position = MVP * vec4(lightDir, 0.0);
    // EmitVertex();
    // 
    // lightDir = trianglePositions[1] - s_Lights[0].Position.xyz;
    // gl_Position = MVP * vec4(lightDir, 0.0);
    // EmitVertex();
    // EndPrimitive();
}

void main()
{
    const mat4x4 transform = u_Transforms[pc_MeshIndex];
    const mat4x4 MVP = u_CameraProjection * u_CameraView * transform;

    const uvec3 positionIndices1 = getTrianglePositionIndices(v_Edge[0].z);
    const uvec3 positionIndices2 = getTrianglePositionIndices(v_Edge[0].w);
    
    vec3 trianglePositions1[3], trianglePositions2[3];
    getTrianglePositions(trianglePositions1, positionIndices1);
    getTrianglePositions(trianglePositions2, positionIndices2);

    const uint positionOffset = s_Meshes[pc_MeshIndex].PositionOffset;
    vec3 edgePosition1 = s_Positions[positionOffset + v_Edge[0].x].xyz;
    vec3 edgePosition2 = s_Positions[positionOffset + v_Edge[0].y].xyz;

    if ((isFacingLight(trianglePositions1) && !isFacingLight(trianglePositions2)) ||
        (!isFacingLight(trianglePositions1) && isFacingLight(trianglePositions2)))
    {
        bool rev = false;
        if (!isFacingLight(trianglePositions1))
            rev = true;

        if (!((positionIndices1[0] == v_Edge[0].x && positionIndices1[1] == v_Edge[0].y) ||
            (positionIndices1[1] == v_Edge[0].x && positionIndices1[2] == v_Edge[0].y) ||
            (positionIndices1[2] == v_Edge[0].x && positionIndices1[0] == v_Edge[0].y)))
            rev = !rev;

        if (!rev)
        {
            const float epsilon = 0.0001f;
            vec3 lightDir = normalize(edgePosition1 - s_Lights[0].Position.xyz);
            gl_Position = MVP * vec4((edgePosition1 + lightDir * epsilon), 1.0);
            EmitVertex();

            // Vertex #2: the starting vertex projected to infinity
            gl_Position = MVP * vec4((edgePosition1 + lightDir * 1.0f), 1.0);
            EmitVertex();

            // Vertex #3: the ending vertex (just a tiny bit below the original edge)
            lightDir = normalize(edgePosition2 - s_Lights[0].Position.xyz);
            gl_Position = MVP * vec4((edgePosition2 + lightDir * epsilon), 1.0);
            EmitVertex();

            // Vertex #4: the ending vertex projected to infinity
            gl_Position = MVP * vec4((edgePosition2 + lightDir * 1.0f), 1.0);
            EmitVertex();
        }
        else
        {
            const float epsilon = 0.0001f;
            vec3 lightDir = normalize(edgePosition2 - s_Lights[0].Position.xyz);
            gl_Position = MVP * vec4((edgePosition2 + lightDir * epsilon), 1.0);
            EmitVertex();

            // Vertex #2: the starting vertex projected to infinity
            gl_Position = MVP * vec4((edgePosition2 + lightDir * 1.0f), 1.0);
            EmitVertex();

            // Vertex #3: the ending vertex (just a tiny bit below the original edge)
            lightDir = normalize(edgePosition1 - s_Lights[0].Position.xyz);
            gl_Position = MVP * vec4((edgePosition1 + lightDir * epsilon), 1.0);
            EmitVertex();

            // Vertex #4: the ending vertex projected to infinity
            gl_Position = MVP * vec4((edgePosition1 + lightDir * 1.0f), 1.0);
            EmitVertex();
        }

        EndPrimitive();
    }

    if (isFacingLight(trianglePositions1))
    {
        const uint maxIdx1 = max(max(positionIndices1.x, positionIndices1.y), positionIndices1.z);
        if (maxIdx1 != v_Edge[0].x && maxIdx1 != v_Edge[0].y)
            emitLightCap(trianglePositions1, MVP);
    }
    
    if (isFacingLight(trianglePositions2))
    {
        const uint maxIdx2 = max(max(positionIndices2.x, positionIndices2.y), positionIndices2.z);
        if (maxIdx2 != v_Edge[0].x && maxIdx2 != v_Edge[0].y)
            emitLightCap(trianglePositions2, MVP);
    }
}