#version 330 core

in VS_OUT {
   vec3 CamPos_tangent;    // tangent space camera position
   vec3 Pos_tangent;       // tangent space position
   vec4 Pos_lightSpace;    // light space position
   vec3 LightDir_tangent;  // tangent space light direction
   vec3 Norm_tangent;      // tangent space normal
   vec2 textCoord;         // uv texture coordinate
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
   // lightSpacePos is in the [-1, 1] range, textures are sampled in the [0, 1]
   vec3 shadowMapSpacePos = lightSpacePos.xyz * 0.5 + 0.5;

   float shadow = 0.0;
   if(!softShadows){
      // TODO exercise 11.1 - single sample shadow
      float depth = texture(shadowMap, shadowMapSpacePos.xy).r;
      // TODO exercise 11.2 - use the shadowBias to apply an offset to the sampled distance
      shadow = depth + shadowBias < shadowMapSpacePos.z ? 1.0 : 0.0;
   }
   else {
      // TODO exercise 11.3 - sample and test multiple texels and set shadow to the weighted contribution of all shadow tests
      vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
      for(int x = -1; x <= 1; ++x) {
         for (int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(texelSize.x * x, texelSize.y * y);
            float depth = texture(shadowMap, shadowMapSpacePos.xy + offset).r;
            shadow += depth + shadowBias < shadowMapSpacePos.z ? 1.0 : 0.0;
         }
      }
      shadow /= 9.0;
   }

   // TODO exercise 11.1 - the texture only contains values in the range [0,1], but your lightSpacePos distance can be bigger than 1,
   // you should ignore the shadow computation (set shadow to 0) if lightSpacePos distance if bigger than 1

   return shadow;
}

void main()
{
   // TEXTURE SAMPLING
   // diffuse texture sampling and material colors
   vec4 albedo = texture(texture_diffuse1, fs_in.textCoord);

   // ambient occlusion texture sampling and range adjustment
   float ambientOcclusion = texture(texture_ambient1, fs_in.textCoord).r;
   ambientOcclusion = mix(1.0, ambientOcclusion, ambientOcclusionMix);

   // normal texture sampling and range adjustment
   // fix normal rgb sampled range goes from [0,1] to xyz normal vector range [-1,1]
   vec3 N = texture(texture_normal1, fs_in.textCoord).rgb;
   N = normalize(N * 2.0 - 1.0);

   // mix the vertex normal and the normal map texture so we can visualize the difference with normal mapping
   N = normalize(mix(fs_in.Norm_tangent, N, normalMappingMix));


   // SKYBOX REFLECTION
   vec3 tangentIncident = (fs_in.Pos_tangent - fs_in.CamPos_tangent);
   vec3 tangentReflect = reflect(tangentIncident, N);

   //  the cube map has to be sampled with world space directions, rotate the normal so that it is in world space
   vec3 reflectionColor = texture(skybox, fs_in.invTBN * tangentReflect).rgb;


   // LIGHTING
   // ambient light
   vec3 ambient = mix(ambientLightColor, reflectionColor, reflectionMix) * albedo.rgb;

   // notice that we are now using parallel light instead of a point light
   vec3 L = normalize(-fs_in.LightDir_tangent);   // L: - light direction
   float diffuseModulation = max(dot(N, L), 0.0);
   vec3 diffuse = lightColor * diffuseModulation * albedo.rgb;

   // notice that we are now using the blinn-phong specular reflection
   vec3 V = normalize(fs_in.CamPos_tangent - fs_in.Pos_tangent); // V: surface to eye vector
   vec3 H = normalize(L + V); // H: half-vector between L and V
   float specModulation = max(dot(N, H), 0.0);
   specModulation = pow(specModulation, specularExponent);
   vec3 specular = lightColor * specModulation * reflectionColor;


   // SHADOW
   // TODO exercise 11 - complete the implementation of the ShadowCalculation function
   float shadow = ShadowCalculation(fs_in.Pos_lightSpace);

   // TODO exercise 11.1 - use the shadow value to modulate the color of the fragment
   FragColor = vec4(ambient * ambientOcclusion + (diffuse + specular) * ambientOcclusion * (1.0f - shadow), albedo.a);
}