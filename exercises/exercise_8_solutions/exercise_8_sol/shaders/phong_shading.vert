#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord; // here for completness, but we are not using it just yet

uniform mat4 model; // represents model coordinates in the world coord space
uniform mat4 view;  // represents the world coordinates in the eye coord space
uniform mat4 invTranspMV; // inverse of the transpose of (view * model) (used to multiply vectors if there is non-uniform scaling)
uniform mat4 projection; // camera projection matrix

// send shaded color to the fragment shader, you won't need it anymore from exercise 8.4
//out vec4 shadedColor;

// TODO add the variables needed for lighting
// light uniform variables
uniform vec3 light1Position;
uniform vec3 light2Position;

// exercises 8.4 and 8.5
out vec3 Pos_eye_frag;
out vec3 N_eye_frag;
out vec3 L1_eye_frag;
out vec3 L2_eye_frag;


void main() {
   // vertex in eye space (for light computation in eye space)
   vec4 Pos_eye = view * model * vec4(vertex, 1.0);
   // normal in eye space (for light computation in eye space)
   vec3 N_eye = normalize((invTranspMV * vec4(normal, 0.0)).xyz);

   // final vertex transform (for opengl rendering, not for lighting)
   gl_Position = projection * Pos_eye;

   // 8.4 pass the positions in eye space to fragment shader
   Pos_eye_frag = Pos_eye.xyz;
   N_eye_frag = N_eye;
   L1_eye_frag = (view * vec4(light1Position, 1.0)).xyz;
   L2_eye_frag = (view * vec4(light2Position, 1.0)).xyz;

}