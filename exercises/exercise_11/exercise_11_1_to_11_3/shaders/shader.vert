#version 330 core
layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 textCoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

out VS_OUT {
   vec2 textCoord;         // uv texture coordinate
   vec3 tangentPos;        // tangent space position
   vec4 lightSpacePos;     // light space position
   vec3 tangentViewPos;    // tangent space camera position
   vec3 tangentLightDir;   // tangent space light direction
   vec3 tangentNorm;       // tangent space normal
   mat3 invTBN;            // matrix tha rotates from TBN to worls space
} vs_out;

// transformations
uniform mat4 projection;   // camera projection matrix
uniform mat4 view;         // represents the world in the eye coord space
uniform mat4 model;        // represents model in the world coord space
uniform mat4 modelInvT;    // inverse of the transpose of the model matrix

// light computation uniforms
uniform vec3 lightDirection;  // world space light direction
uniform vec3 viewPosition;    // world space camera position

// shadowmapping uniforms
uniform mat4 lightSpaceMatrix;   // transforms from world space to

void main() {
   // send text coord to fragment shader
   vs_out.textCoord = textCoord;

   // get only rotation and scale portion of the matrix
   mat3 normalModelInvT = mat3(modelInvT);

   // vertex normal in world space
   vec3 N = normalize(normalModelInvT * normal);

   // TBN matrix,
   // we define TBN as the matrix that transform vectors from world to tangent space (that is why we take the transpose)
   // notice that tangent and bitangent are vertex properties
   mat3 TBN = inverse( mat3( normalize(normalModelInvT * tangent),
                             normalize(normalModelInvT * bitangent),
                             N));

   // inverse of TBN, to map from tangent space to world space
   vs_out.invTBN = inverse(TBN);
   // light direction in tangent space
   vs_out.tangentLightDir = TBN * lightDirection;
   // view in tangent space
   vs_out.tangentViewPos = TBN * viewPosition;
   // vertex in tangent space
   vs_out.tangentPos  = TBN * (model * vec4(vertex, 1.0)).xyz;
   // vertex normal in tangent space
   vs_out.tangentNorm = TBN * N;

   // TODO exercise 11.1 - transform the vertex position (vertex variable) from local space to light space
   // hint - you will need two matrices for that
   vs_out.lightSpacePos = vec4(0.0, 0.0, 0.0, 1.0);

   // final vertex transform (for opengl rendering)
   gl_Position = projection * view * model * vec4(vertex, 1.0);
}