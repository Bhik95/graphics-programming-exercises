#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D srlTexture;

void main()
{
   vec4 textColor = texture(srlTexture, TexCoord);
   FragColor = textColor;
}