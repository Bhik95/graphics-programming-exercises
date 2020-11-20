#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out VS_OUT {
   vec3 CamPos_tangent;
   vec3 Pos_tangent;
   vec3 LightDir_tangent;
   vec3 Norm_tangent;
   vec2 textCoord;
   mat3 invTBN;
} vs_out;

// transformations
uniform mat4 projection;   // camera projection matrix
uniform mat4 view;         // represents the world in the eye coord space
uniform mat4 model;        // represents model in the world coord space
uniform mat4 modelInvT;    // inverse of the transpose of the model matrix

// light uniform variables
uniform vec3 lightDirection;
uniform vec3 viewPosition;

uniform int othoTangentSpace;

void main() {
   // send text coord to fragment shader
   vs_out.textCoord = textCoord;

   // get only rotation and scale portion of the matrix
   mat3 normalModelInvT = mat3(modelInvT);

   // vertex normal in world space
   vec3 N = normalize(normalModelInvT * normal);

   // TODO exercise 10.4 compute the TBN matrix, notice that tangent and bitangent are vertex properties
   mat3 TBN = transpose(mat3( normalize(normalModelInvT * tangent),
                              normalize(normalModelInvT * bitangent),
                              normalize(normalModelInvT * normal)));

   // variables we wanna send to the fragment shader
   // inverse of TBN, to map from tangent space to world space
   vs_out.invTBN = inverse(TBN);
   // light direction in tangent space
   vs_out.LightDir_tangent = TBN * lightDirection;
   // view in tangent space
   vs_out.CamPos_tangent = TBN * viewPosition;
   // vertex in tangent space
   vs_out.Pos_tangent  = vec3(0,0,0); // the position of the vertex in the tangent space is the origin of the space
   // vertex normal in tangent space
   vs_out.Norm_tangent = TBN * N;

   // final vertex transform (for opengl rendering)
   gl_Position = projection * view * model * vec4(vertex, 1.0);

}