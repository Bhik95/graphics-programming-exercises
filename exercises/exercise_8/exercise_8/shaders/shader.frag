#version 330 core

// TODO comment this for exercises 8.4, 8.5 and 8.6, where color should be computed here instead of in the vertex shader
in vec4 shadedColor;

out vec4 FragColor;

// TODO add the 'in' and 'uniform' variables needed for lighting
// in variables
// light uniforms
// material uniforms
// attenuation uniforms (ex 8.6 only)


void main()
{

   // TODO exercise 8.4 - phong shading (i.e. Phong reflection model computed in the fragment shader)


   // TODO exercise 8.5 - multiple lights


   // TODO exercuse 8.6 - attenuation


   // TODO notice that the shadedColor should be computer in this shader from exercise 8.4
   FragColor = shadedColor;

}