#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out VS_OUT {
   vec3 Pos_eye;
   vec3 N_eye;
   vec3 Light_eye;
   vec2 textCoord;
} vs_out;

// transformations
uniform mat4 projection; // camera projection matrix
uniform mat4 view;  // represents the world in the eye coord space
uniform mat4 model; // represents model in the world coord space
uniform mat4 invTranspMV; // inverse of the transpose of (view * model) (used to multiply vectors if there is non-uniform scaling)

// light uniform variables
uniform vec3 lightPosition;

// TODO exercise 9.2, get uvScale as a uniform
uniform float uvScale;

void main() {
   // vertex in eye space (for light computation in eye space)
   vec4 Pos_eye = view * model * vec4(vertex, 1.0);
   // normal in eye space (for light computation in eye space)
   vec3 N_eye = normalize((invTranspMV * vec4(normal, 0.0)).xyz);
   // light in eye space
   vec4 Light_eye = view * vec4(lightPosition, 1.0);

   // final vertex transform (for opengl rendering)
   gl_Position = projection * Pos_eye;

   // out info
   vs_out.Pos_eye = Pos_eye.xyz;
   vs_out.N_eye = N_eye;
   vs_out.Light_eye = Light_eye.xyz;
   // TODO exercise 9.2, use uvScale to scale the texture coordinates
   float uvOffset = uvScale / 2.0 - 0.5;
   vec2 textCoordScaled =  textCoord * uvScale - uvOffset;

   vs_out.textCoord = textCoordScaled;
}