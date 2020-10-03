#version 330 core
layout (location = 0) in vec3 pos;

uniform vec3 offset;

void main()
{
    vec3 finalPosition = mod(1.0 + pos + offset, 2.0) - 1.0;
    gl_Position = vec4(finalPosition, 1.0);
    gl_PointSize = 1-finalPosition.z;
}