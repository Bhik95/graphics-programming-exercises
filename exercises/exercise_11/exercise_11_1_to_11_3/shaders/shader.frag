#version 330 core

in VS_OUT {
   vec2 textCoord;         // uv texture coordinate
   vec3 tangentPos;        // tangent space position
   vec4 lightSpacePos;     // light space position
   vec3 tangentViewPos;    // tangent space camera position
   vec3 tangentLightDir;   // tangent space light direction
   vec3 tangentNorm;       // tangent space normal
   mat3 invTBN;            // matrix tha rotates from TBN to worls space
} fs_in;

// light uniform variables
uniform vec3 ambientLightColor;
uniform vec3 lightColor;
uniform vec3 lightDirection;

// material properties
uniform float ambientOcclusionMix;
uniform float normalMappingMix;
uniform float reflectionMix;
uniform float specularExponent;

// material textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_ambient1;

// skybox texture
uniform samplerCube skybox;

// shadow map uniforms
uniform sampler2D shadowMap;
uniform bool softShadows;
uniform float shadowBias;

// output color
out vec4 FragColor;

float ShadowCalculation(vec4 lightSpacePos)
{
   // TODO exercise 11.1 - complete the implementation of the ShadowCalculation function
   // mind that, when you sample the shadowMap texture, the depth information is contained in the red channel


   float shadow = 0.0;
   if(!softShadows){
      // TODO exercise 11.1 - single sample shadow

      // TODO exercise 11.2 - use the shadowBias to apply an offset to the sampled distance

   }
   else {
      // TODO exercise 11.3 - sample and test multiple texels and set shadow to the weighted contribution of all shadow tests

   }

   // TODO exercise 11.1 - the texture only contains values in the range [0,1], but your lightSpacePos distance can be bigger than 1,
   // you should ignore the shadow computation (set shadow to 0) if lightSpacePos distance if bigger than 1

   return shadow;
}

void main()
{

   // normal texture sampling and range adjustment
   vec3 N = texture(texture_normal1, fs_in.textCoord).rgb;
   // fix normal rgb sampled range goes from [0,1] to xyz normal vector range [-1,1]
   N = normalize(N * 2.0 - 1.0);

   // mix the vertex normal and the normal map texture so we can visualize the difference with normal mapping
   N = normalize(mix(fs_in.tangentNorm, N, normalMappingMix));

   // ambient occlusion texture sampling and range adjustment
   float ambientOcclusion = texture(texture_ambient1, fs_in.textCoord).r;
   ambientOcclusion = mix(1.0, ambientOcclusion, ambientOcclusionMix);

   // diffuse texture sampling and material colors
   vec4 albedo = texture(texture_diffuse1, fs_in.textCoord);

   // skybox reflection
   vec3 tangentIncident = (fs_in.tangentPos - fs_in.tangentViewPos);
   vec3 tangentReflect = reflect(tangentIncident, N);

   // the cube map has to be sampled with world space directions,
   // rotate the sampled normal so that it is in world space
   vec3 reflectionColor = texture(skybox, fs_in.invTBN * tangentReflect).rgb;

   // ambient light
   vec3 ambient = mix(ambientLightColor, reflectionColor, reflectionMix) * albedo.rgb;

   // notice that we are now using parallel light instead of a point light
   vec3 L = normalize(-fs_in.tangentLightDir);   // L: - light direction
   float diffuseModulation = max(dot(N, L), 0.0);
   vec3 diffuse = lightColor * diffuseModulation * albedo.rgb;

   // notice that we are now using the blinn-phong specular reflection
   vec3 V = normalize(fs_in.tangentViewPos - fs_in.tangentPos); // V: surface to eye vector
   vec3 H = normalize(L + V); // H: half-vector between L and V
   float specModulation = max(dot(N, H), 0.0);
   specModulation = pow(specModulation, specularExponent);
   vec3 specular =  mix(lightColor, reflectionColor, reflectionMix)  * specModulation;

   // TODO exercise 11 - complete the implementation of the ShadowCalculation function
   float shadow = ShadowCalculation(fs_in.lightSpacePos);

   // TODO exercise 11.1 - use the shadow value to modulate the color of the fragment
   FragColor = vec4(ambient * ambientOcclusion + (diffuse + specular) * ambientOcclusion, albedo.a);
}