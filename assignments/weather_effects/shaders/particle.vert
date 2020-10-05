#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 modelViewProj;
uniform mat4 modelViewProjPrev;
uniform vec3 offset;
uniform vec3 boxSize;
uniform float lineLength;

out vec4 color;

void main()
{
    vec4 worldPos = vec4(mod(pos+offset, boxSize) - boxSize/2.0, 1.0);

    vec4 worldPosPrev;
    worldPosPrev.xyz = worldPos.xyz + vec3(0.0, -lineLength, 0.0);
    worldPosPrev.w = 1.0;

    vec4 bottom = modelViewProj * worldPos;
    vec4 top = modelViewProj * worldPosPrev;
    vec4 topPrev = modelViewProjPrev * worldPosPrev;

    vec2 dir = (top.xy/top.w) - (bottom.xy/bottom.w);
    vec2 dirPrev = (topPrev.xy/topPrev.w) - (bottom.xy/bottom.w);

    float lenDir = length(dir);
    float lenDirPrev = length(dirPrev);

    float lenColorScale = clamp((lenDir/lenDirPrev) * boxSize.x, 0.0, 1.0);

    //projPos.xy += (0.5 - uv.x) * dirPerp * lineLength;

    gl_Position = mix(topPrev, bottom, gl_VertexID % 2);

    color = vec4(1.0, 1.0, 1.0, lenColorScale);
}