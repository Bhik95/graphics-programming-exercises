#version 330 core

// TODO comment this for exercises 8.4, 8.5 and 8.6, where color should be computed here instead of in the vertex shader
//in vec4 shadedColor;

// the output color of this fragment
out vec4 FragColor;

// TODO add the 'in' and 'uniform' variables needed for lighting
// light uniforms
uniform vec3 ambientLightColor;
uniform vec3 light1Color;
uniform vec3 light2Color;

// material uniforms
uniform vec3 reflectionColor;
uniform float ambientReflectance;
uniform float diffuseReflectance;
uniform float specularReflectance;
uniform float specularExponent;

// in variables - exercises 8.4 and 8.5
in vec3 Pos_eye_frag;
in vec3 N_eye_frag;
in vec3 L1_eye_frag;
in vec3 L2_eye_frag;

// attenuation uniforms (ex 8.6 only)
uniform float attenuationC0;
uniform float attenuationC1;
uniform float attenuationC2;


void main()
{

   // TODO exercise 8.4 - phong shading (i.e. Phong reflection model computed in the fragment shader)
   // ambient component
   vec3 ambient = ambientLightColor * ambientReflectance * reflectionColor;
   vec4 color = vec4(ambient,1);

   // diffuse component
   // L: vertex to light vector
   vec3 L_eye = normalize(L1_eye_frag - Pos_eye_frag).xyz;
   float diffuseModulation = max(dot(N_eye_frag, L_eye), 0.0);
   vec3 diffuse = light1Color * diffuseReflectance * diffuseModulation * reflectionColor;

   // specular component
   // R: incident light (-L) reflection vector, you can also use the reflect() function
   vec3 R_eye =  - L_eye - 2 * dot(-L_eye, N_eye_frag) * N_eye_frag;
   float specModulation = pow(max(dot(R_eye, normalize(-Pos_eye_frag)), 0.0), specularExponent);
   vec3 specular = light1Color * specularReflectance * specModulation;

   // TODO exercuse 8.6 - attenuation - light 1
   float distance = length(Pos_eye_frag - L1_eye_frag);
   float attenuation =  1.0 / (attenuationC0 + attenuationC1 * distance + attenuationC2 * distance * distance);
   color.xyz += (diffuse + specular) * attenuation;

   // TODO exercise 8.5 - multiple lights
   // L: vertex to light vector
   L_eye = normalize(L2_eye_frag - Pos_eye_frag).xyz;
   diffuseModulation = max(dot(N_eye_frag, L_eye), 0.0);
   diffuse = light2Color * diffuseReflectance * diffuseModulation * reflectionColor;

   // R: incident light (-L) reflection vector, you can also use the reflect() function
   R_eye =  - L_eye - 2 * dot(-L_eye, N_eye_frag) * N_eye_frag;
   specModulation = pow(max(dot(R_eye, normalize(-Pos_eye_frag)), 0.0), specularExponent);
   specular = light2Color * specularReflectance * specModulation;

   // TODO exercuse 8.6 - attenuation - light 2
   distance = length(Pos_eye_frag - L2_eye_frag);
   attenuation =  1.0 / (attenuationC0 + attenuationC1 * distance + attenuationC2 * distance * distance);
   color.xyz += (diffuse + specular) * attenuation;

   // you might have noticed that the shading contribution of multiple lights can fit a for loop nicely
   // we will be doing that on a more advanced class

   // TODO notice that the shadedColor should be computer in this shader from exercise 8.4
   FragColor = color;
}