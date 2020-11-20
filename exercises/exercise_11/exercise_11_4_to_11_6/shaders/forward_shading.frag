#version 330 core
out vec4 FragColor;

in mat3 TBN;
in vec2 TexCoords;
in vec3 Normal;
in vec3 Position;

struct Light {
   vec3 Position;
   vec3 Color;
   float Constant;
   float Linear;
   float Quadratic;
};

const int NR_LIGHTS = 64;
uniform Light lights[NR_LIGHTS];
uniform vec3 viewPos;
uniform float specularOffset;
uniform float lightIntensity;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_specular1;
uniform float normalMappingMix;

uniform bool lightsAreOn;


void main()
{

   vec3 FragPos = Position;

   vec3 Diffuse = texture(texture_diffuse1, TexCoords).rgb;
   float Specular = texture(texture_specular1, TexCoords).r + specularOffset;


   // also store the per-fragment normals into the gbuffer
   vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
   normalMap = normalMap * 2.0 - 1.0;
   normalMap = normalize(TBN * normalMap);
   vec3 FragNorm = normalize(mix(normalize(Normal), normalMap, normalMappingMix));

   vec3 lighting  = Diffuse;

   if (lightsAreOn){
      // calculate lighting
      lighting  = Diffuse * 0.1;// hard-coded ambient component
      vec3 viewDir  = normalize(viewPos - FragPos);

      for (int i = 0; i < NR_LIGHTS; ++i)
      {
         // diffuse
         vec3 lightDir = normalize(lights[i].Position - FragPos);
         vec3 diffuse = max(dot(FragNorm, lightDir), 0.0) * Diffuse * lights[i].Color;
         // specular
         vec3 halfwayDir = normalize(lightDir + viewDir);
         float spec = pow(max(dot(FragNorm, halfwayDir), 0.0), 64.0);
         vec3 specular = lights[i].Color * spec * Specular;
         // attenuation
         float distance = length(lights[i].Position - FragPos);
         float attenuation = 1.0 / (lights[i].Constant + lights[i].Linear * distance + lights[i].Quadratic * distance * distance);
         diffuse *= attenuation;
         specular *= attenuation;
         lighting += diffuse + specular;
      }
   }

   FragColor = vec4(lighting * lightIntensity, 1.0);
}

