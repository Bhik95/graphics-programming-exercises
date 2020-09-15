#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 color;
out vec4 vtxColor;

// TODO 3.1 create a mat4 uniform named 'model', you should set it for each part of the plane
uniform mat4 model;

void main()
{
   // TODO 3.1 multiply the postion by the 'model' matrix you have created
   gl_Position = model * vec4(pos, 1.0);
   vtxColor = color;
}