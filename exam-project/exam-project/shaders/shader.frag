#version 330 core

/*Ray marching params*/
#define MAX_STEPS 200 // Max number of Raymaching iterations
#define MAX_DIST 100. // Max reachable distance of the raymarcher
#define SURFACE_DIST .001 // The distance from the surface of an object used as a threshold to identify whether a collision happened or not

/*Triplanar Mapping params*/
#define TILING_FACTOR 0.4 // Tiling factor for the textures
#define TRIPLANAR_BLEND_SHARPNESS 3 // How sharp is the blending from one projection to the other

#define BASE_DIFFUSE 0.1 // Fake global light

/*(Soft) Shadows Raymarching params*/
/* Notice that MAX_STEPS_SHADOW is much lower compared to MAX_STEPS:
Decreasing the value greatly improves performance and the fidelity of the soft shadows is not noticeable
*/
#define MAX_STEPS_SHADOW 5
#define SURFACE_DIST_SHADOW .1
#define SHADOW_K 8 // Softness param

out vec4 fragColor;

uniform vec2 uScreenSize; // Screen size in pixel
uniform float uTime;
uniform vec3 uCamPosition;
uniform mat4 cameraViewMat; // Camera view matrix
uniform float uFov; // Field of view
uniform float uShininess; // Shininess of the material

uniform sampler2D textureSides;
uniform sampler2D textureTop;

// polynomial smooth min
float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}

// Signed distance field of a Round Box with bounds b and radius r
float sdRoundBox( vec3 p, vec3 b, float r )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

// Signed distance field of a Sphere of radius s
float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}

// Signed distance field of a torus with (radius1, radius2) t
float sdTorus( vec3 p, vec2 t )
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

// Signed distance field of a capped cone of height h, radiuses r1 and r2, and roundness
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

// Signed distance field of a capsule with height h and radius r
float sdVerticalCapsule( vec3 p, float h, float r )
{
    p.y -= clamp( p.y, 0.0, h );
    return length( p ) - r;
}

// Signed distance field of a box with bounds b
float sdBox( vec3 p, vec3 b )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// Signed distance field of a cone with c=(cos(theta), sin(theta)) and height h
float sdCone( vec3 p, vec2 c, float h )
{
    float q = length(p.xz);
    return max(dot(c.xy,vec2(q,p.y)),-h-p.y);
}

// Signed distance field of a tree
float sdTree(vec3 p){
    float cone1 = sdCone(p - vec3(0, 2.7, 0), vec2(0.86602540378, 0.5), 1.5);
    float cone2 = sdCone(p - vec3(0, 1.7, 0), vec2(0.70710678118, 0.70710678118), 1.);
    float cones = min(cone1, cone2);
    float trunk = sdVerticalCapsule(p - vec3(0, -0.5, 0), 1.5, 0.1);
    return min(trunk,cones);
}

// Infinite Repetition operator, where c is the repetition period
vec3 transformInfiniteRepeat(vec3 p, float c){
    return mod(p+0.5*c,c)-0.5*c;
}

// Signed distance field of a forest (repeated trees) with a central area with no trees
float sdForest(vec3 p){
    vec3 q = transformInfiniteRepeat(p, 3);
    q.y = p.y; // Avoid repeating along the y axis
    float repeatedTreesDist = sdTree(q);
    return max(repeatedTreesDist, -sdBox(p, vec3(11, 10, 11))); // Boolean Subtraction: max(d1, -d2)
}

// Signed distance field of a sphere and a torus that lerp over time in a sinusoidal pattern
float sdTorusSphereLerp(vec3 p){
    float sphereDist = sdSphere(p, 1.0);
    float torusDist = sdTorus(p, vec2(1.0, 0.1));

    float t = sin(uTime)*0.5+0.5;
    return t*sphereDist+(1-t)*torusDist; // Lerp
}

// Signed distance field of the WHOLE SCENE
float sdScene(vec3 p){
    float torusSphereBlendDist = sdTorusSphereLerp(p - vec3(0, 1, 0)); // The object that lerps between a sphere and a torus through time
    float terrainDist = p.y; // The terrain (xz plan)
    float roundBoxDist = sdRoundBox(p - vec3(3, .6, 0), vec3(.5, .25, .5), .1); // The round box
    float forest = sdForest(p); // The forest (trees are repeated every 3 meters, excep in a 11x11 area in the center)

    float dist = smin(torusSphereBlendDist, terrainDist*0.2, 0.5); // smin = smooth union between the terrain and the object
    dist = min(dist, roundBoxDist); // min = union between the previous objects and the box
    dist = min(dist, forest); // min = union between the previous objects and the forest

    return dist;
}

//Calculate the ray direction starting from a certain screen position
vec3 getRayDir(vec2 uv) {
    vec2 h = vec2(
    tan(uFov / 2.0) * (uScreenSize.x / uScreenSize.y),
    tan(uFov / 2.0)
    );
    vec3 pCam = vec3(uv * h, -1.0);
    // Convert from eye space (uv) to world space with the inverse view matrix:
    return normalize((inverse(cameraViewMat) * vec4(pCam, 0.0)).xyz);
}

// Raymarch from the origin ro along the direction rd
float rayMarch(vec3 ro, vec3 rd){
    float d0 = 0.;

    for(int i=0; i<MAX_STEPS; i++){
        vec3 p = ro+d0*rd;
        float dS = sdScene(p);
        d0 += dS;
        if(dS<SURFACE_DIST || abs(d0) > MAX_DIST) break;// Break if there's a collision or the max distance is reached
    }

    return d0;
}

// Estimation of the normals (World Space)
vec3 getNormal(vec3 p){
    vec2 e = vec2(.01, 0);
    float d = sdScene(p);
    vec3 n = vec3(
    // Sample the neighbours' distances from the closest object and compare them to the central point
    d-sdScene(p-e.xyy),
    d-sdScene(p-e.yxy),
    d-sdScene(p-e.yyx));
    return normalize(n);
}

// Returns the diffuse+specular (Blinn-phong)
float getLight(vec3 pos, vec3 normal, vec3 lightDir){
    vec3 viewDir = normalize(uCamPosition-pos);
    vec3 halfDir = normalize(lightDir + viewDir);

    float specAngle = clamp(dot(halfDir, normal), 0., 1.);
    float specular = clamp(pow(specAngle, uShininess), 0., 1.);

    float dif = clamp(dot(normal, lightDir), 0., 1.);
    float res = clamp(dif + specular, 0., 1.);

    return res;
}

// Raymarch from the origin ro along the direction rd with a smoothing parameter k
/*
The idea behind soft shadows is this:
Raymarch from the surface of the object towards the light source -> get distance d
If there is a collision, the shadow factor needs to be 0 (hard shadow)
If you ALMOST collide with an other object, the starting object needs to be darkened depending by:
- how much you missed the collision with the other object (h variable in the following code).
- the distance between the point to shade and the closest scene object (t variable)
*/
float softShadow( in vec3 ro, in vec3 rd, float k )
{
    float res = 1.0;
    for( float t=SURFACE_DIST_SHADOW; t<MAX_STEPS_SHADOW; )
    {
        float h = sdScene(ro + rd*t); // How close my point was to hit an object
        if( h<SURFACE_DIST )
        return 0.0;
        // t ends up being the distance between the point to shade and the closest scene object
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}

vec4 triplanarMapping(sampler2D xzSampler, sampler2D xySampler, sampler2D yzSampler, vec3 pos, vec3 normal){
    vec4 xz_projection = texture(xzSampler, pos.xz * TILING_FACTOR);
    vec4 xy_projection = texture(xySampler, pos.xy * TILING_FACTOR);
    vec4 yz_projection = texture(yzSampler, pos.yz * TILING_FACTOR);

    vec3 absNormal = pow(abs(normal), vec3(TRIPLANAR_BLEND_SHARPNESS));

    vec4 albedo = (yz_projection * absNormal.x + xz_projection * absNormal.y + xy_projection * absNormal.z) / (absNormal.x + absNormal.y + absNormal.z);
    return albedo;
}

void main()
{
    // My point light position
    vec3 lightPos = vec3(5.0, 5.0, 6);

    vec2 uv = (gl_FragCoord.xy/uScreenSize) * 2.0 - 1.0; // NDC [-1, 1]x [-1, 1]

    vec3 ray_direction = getRayDir(uv);

    float d = rayMarch(uCamPosition, ray_direction);

    fragColor = vec4(0.0, 0.0, 0.0, 0.0); // default color
    if(d < 0){
        // Show red color if camera is inside an object
       fragColor = vec4(0.5, 0, 0, 1);
    }
    else if(d < MAX_DIST){
        vec3 pos = vec3(uCamPosition + d * ray_direction); // position of the point in the ""point cloud""
        vec3 lightDir = normalize(lightPos-pos);
        vec3 normal = getNormal(pos);

        float diffuseSpec = getLight(pos, normal, lightDir);

        // For shadows, raymarch from the collision point towards the light source. K is a smoothing factor
        float shadow = softShadow(pos, lightDir, SHADOW_K);

        // Texturing: a texture on the top (xz plane) and a texture for the sides (xy and yz planes)
        vec4 albedo = triplanarMapping(textureTop, textureSides, textureSides, pos, normal);

        fragColor = albedo * clamp(diffuseSpec * shadow + BASE_DIFFUSE, 0., 1.);
        //fragColor(normal, 1);
    }


    //fragColor = vec4(ray_direction, 1.0);
    //fragColor = vec4(uv, 0.0, 1.0);
    //fragColor = texture(textureSides, uv);
    //fragColor = texture(textureTop, uv);
}