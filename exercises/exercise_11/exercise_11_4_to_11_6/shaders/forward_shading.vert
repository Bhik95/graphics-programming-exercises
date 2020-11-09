#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec2 TexCoords;
out vec3 Normal;
out vec3 Position;
out mat3 TBN;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
   // fragment position (world space)
   Position = (model * vec4(aPos, 1.0)).xyz;
   TexCoords = aTexCoords;

   // fragment normal (world space)
   mat3 modelInvTransp = transpose(inverse(mat3(model)));
   Normal = modelInvTransp * aNormal;

   TBN = mat3( normalize(modelInvTransp * aTangent),
   normalize(modelInvTransp * aBitangent),
   normalize(modelInvTransp * aNormal));

   gl_Position = projection * view * model * vec4(aPos, 1.0);
}

