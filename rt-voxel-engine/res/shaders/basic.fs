#version 330 core
out vec4 FragColor;

in vec2 v_TexCoord;
in float v_Light;
uniform sampler2D texture0;

void main()
{
    FragColor = texture(texture0, v_TexCoord) * v_Light;
} 