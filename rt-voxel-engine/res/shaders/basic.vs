#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in float light;

out vec2 v_TexCoord;
out float v_Light;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position =  projection * view * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    v_TexCoord = texCoord;
    v_Light = light;
}