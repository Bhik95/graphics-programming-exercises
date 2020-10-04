#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 model;
uniform vec3 offset;
uniform vec3 boxSize;

void main()
{
    gl_Position = model*vec4(mod(pos+offset, boxSize), 1.0);
    gl_PointSize = 2.0*(1-gl_Position.z);
}