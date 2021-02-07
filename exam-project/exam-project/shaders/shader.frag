#version 330 core

#define MAX_STEPS 200
#define MAX_DIST 100.
#define SURFACE_DIST .001
#define TILING_FACTOR 0.4
#define BASE_DIFFUSE 0.1
#define SURFACE_DIST_SHADOW .1
#define SHADOW_K 8

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

float sdRoundBox( vec3 p, vec3 b, float r )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
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

float sdCappedCone( vec3 p, float h, float r1, float r2, float roundness)
{
    vec2 q = vec2( length(p.xz), p.y );
    vec2 k1 = vec2(r2,h);
    vec2 k2 = vec2(r2-r1,2.0*h);
    vec2 ca = vec2(q.x-min(q.x,(q.y<0.0)?r1:r2), abs(q.y)-h);
    vec2 cb = q - k1 + k2*clamp( dot(k1-q,k2)/dot(k2, k2), 0.0, 1.0 );
    float s = (cb.x<0.0 && ca.y<0.0) ? -1.0 : 1.0;
    return s*sqrt( min(dot(ca, ca),dot(cb, cb))) - roundness;
}

float sdVerticalCapsule( vec3 p, float h, float r )
{
    p.y -= clamp( p.y, 0.0, h );
    return length( p ) - r;
}

float sdBox( vec3 p, vec3 b )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdCone( vec3 p, vec2 c, float h )
{
    float q = length(p.xz);
    return max(dot(c.xy,vec2(q,p.y)),-h-p.y);
}

// A cute tree in a vase :D
float sdTree(vec3 p){
    float vase = sdCappedCone(p, 0.4, 0.4, 0.6, 0.1);
    float cone1 = sdCone(p - vec3(0, 2.7, 0), vec2(0.86602540378, 0.5), 1.5);
    float cone2 = sdCone(p - vec3(0, 1.7, 0), vec2(0.70710678118, 0.70710678118), 1.);
    float cones = min(cone1, cone2);
    float trunk = sdVerticalCapsule(p - vec3(0, 0.5, 0), 1.0, 0.1);
    return min(vase, min(trunk,cones));
}

float sdForest(vec3 p){
    float c = 3;
    vec3 q = mod(p+0.5*c,c)-0.5*c; // q is for repetition
    q.y = p.y;
    return max(sdTree(q), -sdBox(p, vec3(11, 10, 11))); // subtraction: (forest - area (box) without a forest)
}

float sdTorusSphereLerp(vec3 p){
    float sphereDist = sdSphere(p - vec3(0, 1, 0), 1.0);
    float torusDist = sdTorus(p - vec3(0, 1, 0), vec2(1.0, 0.1));
    float t = sin(uTime)*0.5+0.5;
    return t*sphereDist+(1-t)*torusDist;
}

float GetDist(vec3 p){

    float torusSphereBlendDist = sdTorusSphereLerp(p); // The object that lerps between a sphere and a torus through time

    float terrainDist = p.y; // The terrain (xz plan)

    float dist = smin(torusSphereBlendDist, terrainDist*0.2, 0.5); // smin = smooth union between the terrain and the object

    float roundBoxDist = sdRoundBox(p - vec3(3, .6, 0), vec3(.5, .25, .5), .1); // A box

    dist = min(dist, roundBoxDist); // min = union between the previous objects and the box

    float forest = sdForest(p); // A forest (trees are repeated every 3 meters, excep in a 11x11 area in the center)

    dist = min(dist, forest); // min = union between the previous objects and the forest

    return dist;
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

float softshadow( in vec3 ro, in vec3 rd, float k )
{
    float res = 1.0;
    for( float t=SURFACE_DIST_SHADOW; t<MAX_DIST; )
    {
        float h = GetDist(ro + rd*t); // How close my point was to hit an object
        if( h<SURFACE_DIST )
        return 0.0;
        // t ends up being the distance between the point to shade and the closest scene object
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}

// Returns the (diffuse+specular) * shadow factor
float GetLight(vec3 pos, vec3 normal, vec3 lightDir){
    // Blinn-phong
    vec3 viewDir = normalize(uCamPosition-pos);

    vec3 halfDir = normalize(lightDir + viewDir);
    float specAngle = clamp(dot(halfDir, normal), 0., 1.);
    float specular = clamp(pow(specAngle, uShininess), 0., 1.);

    float dif = clamp(dot(normal, lightDir), 0., 1.);
    float res = clamp(dif + specular, 0., 1.);

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

vec4 TriplanarMapping(sampler2D xzSampler, sampler2D xySampler, sampler2D yzSampler, vec3 pos, vec3 normal){
    vec4 xz_projection = texture(xzSampler, pos.xz * TILING_FACTOR);
    vec4 xy_projection = texture(xySampler, pos.xy * TILING_FACTOR);
    vec4 yz_projection = texture(yzSampler, pos.yz * TILING_FACTOR);

    vec4 albedo = yz_projection * normal.x + xz_projection * normal.y + xy_projection * normal.z;
    return albedo;
}

void main()
{
    // My point light position
    vec3 lightPos = vec3(5.0, 5.0, 6);

    vec2 uv = (gl_FragCoord.xy/uScreenSize) * 2.0 - 1.0; // [-1, 1]x [-1, 1]

    vec3 ray_direction = getRayDir(uv);

    float d = RayMarch(uCamPosition, ray_direction);

    fragColor = vec4(0.0, 0.0, 0.0, 0.0); // default color
    if(d < MAX_DIST){
        vec3 pos = vec3(uCamPosition + d * ray_direction); // position of the point in the ""point cloud""

        vec3 lightDir = normalize(lightPos-pos);

        vec3 normal = GetNormal(pos);

        float diffuseSpec = GetLight(pos, normal, lightDir);

        // For shadows, raymarch from the collision point towards the light source. K is a smoothing factor
        float shadow = softshadow(pos, lightDir, SHADOW_K);

        vec4 albedo = TriplanarMapping(textureTop, textureSides, textureSides, pos, normal);
        fragColor = albedo * clamp(diffuseSpec * shadow + BASE_DIFFUSE, 0., 1.);
    }


    //fragColor = vec4(ray_direction, 1.0);
    //fragColor = vec4(uv, 0.0, 1.0);
    //fragColor = texture(textureSides, uv);
    //fragColor = texture(textureTop, uv);
}