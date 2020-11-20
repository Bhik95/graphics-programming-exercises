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
   vec3 reflectVec = reflect(fin.position - cameraPos, fin.normal);
   vec4 reflectColor = texture(skybox, reflectVec);


   // TODO exercise 10.2 - refract the camera to fragment vector and sample the skybox with the reffracted direction
   float ratio = 1.0 / 1.55;
   vec3 refractVec = refract(fin.position - cameraPos, fin.normal, ratio);
   vec4 refractColor = texture(skybox, refractVec);

   FragColor = mix(reflectColor, refractColor, reflectRefractMix);
}