#version 330 core

out vec4 fragColor;

in vec3 posC;

void main()
{
    fragColor = vec4(posC.x, posC.y, posC.z, 0.5);
}