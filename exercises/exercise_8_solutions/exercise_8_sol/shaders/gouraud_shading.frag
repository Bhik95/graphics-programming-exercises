#version 330 core

in vec4 shadedColor;
out vec4 FragColor;

void main()
{
   // we pass through the inteprolated color since lighting was already computed in the vertex shader
   FragColor = shadedColor;
}