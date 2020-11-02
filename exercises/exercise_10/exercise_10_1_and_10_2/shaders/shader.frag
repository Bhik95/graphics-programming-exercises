#version 330 core
out vec4 FragColor;

in VS_OUT {
   vec3 normal;
   vec3 position;
} fin;


uniform vec3 cameraPos;
uniform samplerCube skybox;

uniform float reflectRefractMix;

void main()
{
   // TODO exercise 10.1 - reflect camera to fragment vector and sample the skybox with the reflected direction
   vec4 reflectColor = vec4(1.0, 1.0, 1.0, 1.0);

   // TODO exercise 10.2 - refract the camera to fragment vector and sample the skybox with the reffracted direction
   vec4 refractColor = vec4(1.0, 1.0, 1.0, 1.0);

   FragColor = mix(reflectColor, refractColor, reflectRefractMix);
}