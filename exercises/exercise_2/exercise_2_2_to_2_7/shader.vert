#version 330 core
layout (location = 0) in vec2 pos;   // the position variable has attribute position 0
// TODO 2.2 add velocity and timeOfBirth as vertex attributes

// TODO 2.3 create and use a float uniform for currentTime

void main()
{
    // TODO 2.3 use the currentTime to control the particle in different stages of its lifetime

    gl_Position = vec4(pos, 0.0, 1.0);
    gl_PointSize = 10.0;
}