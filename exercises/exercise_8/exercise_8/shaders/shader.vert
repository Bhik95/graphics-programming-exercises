#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord; // here for completness, but we are not using it just yet


uniform mat4 model; // represents model coordinates in the world coord space
uniform mat4 view;  // represents the world coordinates in the eye coord space
uniform mat4 invTranspMV; // inverse of the transpose of (view * model) (used to multiply vectors if there is non-uniform scaling)
uniform mat4 projection; // camera projection matrix

// send shaded color to the fragment shader, you won't need it anymore from exercise 8.4
out vec4 shadedColor;

// TODO add the variables needed for lighting
// light uniform variables
// material properties


void main() {
   // vertex in eye space (for light computation in eye space)
   vec4 posEyeSpace = view * model * vec4(vertex, 1.0);
   // normal in eye space (for light computation in eye space)
   vec3 normEyeSpace = normalize((invTranspMV * vec4(normal, 0.0)).xyz);

   // final vertex transform (for opengl rendering)
   gl_Position = projection * posEyeSpace;


   // TODO exercises 8.1, 8.2 and 8.3 - Gouraud shading (i.e. Phong reflection model computed in the vertex shader)
   //  light in eye space


   shadedColor = vec4(.8, .8, .8, 1); // replace color with your lighting model
}