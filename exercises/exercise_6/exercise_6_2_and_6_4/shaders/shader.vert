#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uvCoord;

out vec2 TexCoord;
uniform mat4 mvp;

void main()
{
   gl_Position = mvp * vec4(pos, 1.0);
   TexCoord = uvCoord;
}