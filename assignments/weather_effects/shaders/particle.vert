#version 330 core
layout (location = 0) in vec3 pos;

uniform mat4 modelViewProj;
uniform mat4 modelViewProjPrev;
uniform vec3 offset;
uniform vec3 boxSize;
uniform float lineLength;

out vec3 posC;

void main()
{
    vec4 worldPos = vec4(mod(pos+offset, boxSize) - boxSize/2.0, 1.0);

    vec4 worldPosPrev;
    worldPosPrev.xyz = worldPos.xyz + vec3(0.0, -lineLength, 0.0);
    worldPosPrev.w = 1.0;

    vec4 bottom = modelViewProj * worldPos;
    vec4 topPrev = modelViewProjPrev * worldPosPrev;

    vec2 dir = (topPrev.xy/topPrev.w) - (bottom.xy/bottom.w);

    vec2 dirPerp = normalize(vec2(-dir.y, dir.x));

    vec4 projPos;

    projPos = mix(topPrev, bottom, gl_VertexID % 2);

    //projPos.xy += (0.5 - uv.x) * dirPerp * lineLength;

    gl_Position = projPos;

    //posC = gl_Position.xyz / 10.0;
    posC = vec3(gl_Position.xyz / gl_Position.w);
}