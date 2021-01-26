#version 330 core

#define MAX_STEPS 200
#define MAX_DIST 100.
#define SURFACE_DIST .001
#define TILING_FACTOR 0.1

out vec4 fragColor;

uniform float uScreenHeight;
uniform float uTime;
uniform vec3 uCamForward;
uniform vec3 uCamPosition;
// material texture
uniform sampler2D texture_diffuse;

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

float GetLight(vec3 pos){
    //Light position
    vec3 lightPos = vec3(5.0, 5.0, 6);

    vec3 lightDir = normalize(lightPos-pos);

    vec3 normal = GetNormal(pos);

    float dif = clamp(dot(normal, lightDir), 0., 1.);

    // Shadow:
    float d = RayMarch(pos + normal*SURFACE_DIST*2.0, lightDir);
    if(d<length(lightPos-pos))
    dif*=.1;

    return dif;
}

void main()
{
    vec2 uv = (gl_FragCoord.xy/uScreenHeight) * 2.0 - 1.0; //[-1; 1]x[-1; 1]

    // Camera Model
    vec3 ray_origin = uCamPosition; //Ray origin (camera)

    vec3 camRight = cross(uCamForward, vec3(0.0, 1.0, 0.0));
    vec3 camUp = cross(camRight, uCamForward);

    vec3 ray_direction = normalize(uv.x * camRight + uv.y * camUp + uCamForward);//TODO: Ray direction given by mouse

    float d = RayMarch(ray_origin, ray_direction);

    fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    if(d < MAX_DIST){
        vec3 pos = vec3(ray_origin+d*ray_direction);

        float diffuse = GetLight(pos);

        vec3 normal = GetNormal(pos);

        vec4 xz_projection = texture(texture_diffuse, pos.xz * TILING_FACTOR);
        vec4 xy_projection = texture(texture_diffuse, pos.xy * TILING_FACTOR);
        vec4 yz_projection = texture(texture_diffuse, pos.yz * TILING_FACTOR);

        vec4 albedo = yz_projection * normal.x + xz_projection * normal.y + xy_projection * normal.z;
        fragColor = albedo * diffuse;

        vec4 aaf = fwidth(fragColor); // Anti-aliasing factor
    }
}