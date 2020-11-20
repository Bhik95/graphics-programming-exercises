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
uniform vec3 ambientLightColor;
uniform vec3 light1Position;
uniform vec3 light1Color;
uniform vec3 light2Position;
uniform vec3 light2Color;

// material properties
uniform vec3 reflectionColor;
uniform float ambientReflectance;
uniform float diffuseReflectance;
uniform float specularReflectance;
uniform float specularExponent;

// attenuation uniforms (ex 8.6 only)
uniform float attenuationC0;
uniform float attenuationC1;
uniform float attenuationC2;

void main() {
   // vertex in eye space (for light computation in eye space)
   vec4 Pos_eye = view * model * vec4(vertex, 1.0);
   // normal in eye space (for light computation in eye space)
   vec3 N_eye = normalize((invTranspMV * vec4(normal, 0.0)).xyz);

   // final vertex transform (for opengl rendering, not for lighting)
   gl_Position = projection * Pos_eye;


   // TODO exercises 8.1, 8.2 and 8.3 - Gouraud shading (i.e. Phong reflection model computed in the vertex shader)
   // light position in eye space
   vec4 L1_eye = view * vec4(light1Position, 1.0);

   // 8.1 ambient
   vec3 ambient = ambientLightColor * ambientReflectance * reflectionColor;

   // 8.2 diffuse
   // L: vertex to light vector
   vec3 L_eye = normalize(L1_eye - Pos_eye).xyz;
   float diffuseModulation = max(dot(N_eye, L_eye), 0.0);
   vec3 diffuse = light1Color * diffuseReflectance * diffuseModulation * reflectionColor;

   // 8.3 specular
   // R: incident light vector (i.e. -L) reflected using normal normEyeSpace (could have used the reflect function)
   vec3 R_eye =  -L_eye - 2 * dot(-L_eye, N_eye) * N_eye; // the same as reflect(-L_eye, normal)
   float specModulation = pow(max(dot(R_eye, normalize(-Pos_eye.xyz)), 0.0), specularExponent);
   vec3 specular = light1Color * specularReflectance * specModulation;

   // TODO exercuse 8.6 - attenuation - light 1
   float distance = length(Pos_eye - L1_eye);
   float attenuation =  1.0 / (attenuationC0 + attenuationC1 * distance + attenuationC2 * distance * distance);

   shadedColor = vec4(ambient + (diffuse + specular) * attenuation, 1);

}