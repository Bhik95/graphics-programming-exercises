#version 330 core

#define MAX_STEPS 200
#define MAX_DIST 100.
#define SURFACE_DIST .001
#define TILING_FACTOR 0.1

out vec4 fragColor;

uniform vec2 uScreenSize;
uniform float uTime;
uniform vec3 uCamPosition;
uniform mat4 cameraViewMat;
uniform float uFov;
uniform float uShininess;

uniform sampler2D textureSides;
uniform sampler2D textureTop;

// polynomial smooth min
float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}

float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}

float sdTorus( vec3 p, vec2 t )
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float GetDist(vec3 p){
    float sphereDist = sdSphere(p - vec3(0, 1, 0), 1.0);
    float torusDist = sdTorus(p - vec3(0, 1, 0), vec2(1.0, 0.1));
    float planeDist = p.y;

    float t = sin(uTime)*0.5+0.5;

    return smin(t*sphereDist+(1-t)*torusDist, planeDist*.2, 0.5);
}

vec3 GetNormal(vec3 p){
    vec2 e = vec2(.01, 0);
    float d = GetDist(p);
    vec3 n = vec3(
    d-GetDist(p-e.xyy),
    d-GetDist(p-e.yxy),
    d-GetDist(p-e.yyx));
    return normalize(n);
}

float RayMarch(vec3 ro, vec3 rd){
    float d0 = 0.;

    for(int i=0; i<MAX_STEPS; i++){
        vec3 p = ro+d0*rd;
        float dS = GetDist(p);
        d0 += dS;
        if(dS<SURFACE_DIST || abs(d0) > MAX_DIST) break;
    }

    return d0;
}

// Returns the (diffuse+specular) * shadow factor
float GetLight(vec3 pos, vec3 normal){

    // My point light position
    vec3 lightPos = vec3(5.0, 5.0, 6);

    // Blinn-phong
    vec3 lightDir = normalize(lightPos-pos);
    vec3 viewDir = normalize(uCamPosition-pos);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = clamp(dot(halfDir, normal), 0., 1.);
    float specular = pow(specAngle, uShininess);

    float dif = clamp(dot(normal, lightDir), 0., 1.);
    float res = clamp(dif + specular, 0., 1.);

    // Shadow:
    float d = RayMarch(pos + normal*SURFACE_DIST*2.0, lightDir);
    if(d<length(lightPos-pos))
    res*=.1;

    return res;
}

//Calculate the ray direction starting from a certain screen position (given the Field of View)
vec3 getRayDir(vec2 uv) {
    vec2 h = vec2(
    tan(uFov / 2.0) * (uScreenSize.x / uScreenSize.y),
    tan(uFov / 2.0)
    );
    vec3 pCam = vec3(uv * h, -1.0);
    // Convert from eye space (uv) to world space with the inverse view matrix:
    return normalize((inverse(cameraViewMat) * vec4(pCam, 0.0)).xyz);
}

void main()
{
    vec2 uv = (gl_FragCoord.xy/uScreenSize) * 2.0 - 1.0; // [-1, 1]x [-1, 1]

    vec3 ray_direction = getRayDir(uv);

    float d = RayMarch(uCamPosition, ray_direction);

    fragColor = vec4(0.0, 0.0, 0.0, 0.0); // default color
    if(d < MAX_DIST){
        vec3 pos = vec3(uCamPosition + d * ray_direction); // position of the point in the ""point cloud""

        vec3 normal = GetNormal(pos);

        float diffuseSpec = GetLight(pos, normal);

        vec4 xz_projection = texture(textureTop, pos.xz * TILING_FACTOR);
        vec4 xy_projection = texture(textureSides, pos.xy * TILING_FACTOR);
        vec4 yz_projection = texture(textureSides, pos.yz * TILING_FACTOR);

        vec4 albedo = yz_projection * normal.x + xz_projection * normal.y + xy_projection * normal.z;
        fragColor = albedo * diffuseSpec;
        //fragColor = vec4(diffuseSpec, diffuseSpec, diffuseSpec, 1.0);
    }


    //fragColor = vec4(ray_direction, 1.0);
    //fragColor = vec4(uv, 0.0, 1.0);
    //fragColor = texture(textureSides, uv);
    //fragColor = texture(textureTop, uv);
}