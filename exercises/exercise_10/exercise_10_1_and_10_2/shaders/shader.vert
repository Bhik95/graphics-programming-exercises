#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out VS_OUT {
   vec3 normal;
   vec3 position;
} vout;

uniform mat4 model;
uniform mat4 modelInvT;
uniform mat4 view;
uniform mat4 projection;

void main()
{
   // we send the normal and position to the fragment shader in WORLD space
   vout.normal = mat3(modelInvT) * normal;
   vout.position = vec3(model * vec4(position, 1.0));
   // this is just the usual
   gl_Position = projection * view * model * vec4(position, 1.0);
}

