#version 330 core

#define MAX_STEPS 100
#define MAX_DIST 100.
#define SURFACE_DIST .01

out vec4 fragColor;

uniform float uScreenHeight;
uniform float uTime;
uniform vec3 uCamForward;
uniform vec3 uCamPosition;

// polynomial smooth min
float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}

float GetDist(vec3 p){
    vec4 sphere = vec4(0, 1, 0, 1.);

    float sphereDist = length(p - sphere.xyz) - sphere.w;
    float planeDist = p.y;

    return smin(sphereDist, planeDist*.2, 0.5);
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

    vec3 pos = vec3(ray_origin+d*ray_direction);

    float diffuse = GetLight(pos);
    
    vec3 col = vec3(diffuse);

    fragColor = vec4(col, 1.0);
}