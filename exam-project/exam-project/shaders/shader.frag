#version 330 core
out vec4 fragColor;

uniform float screenHeight;

void main()
{
    vec2 uv = (gl_FragCoord.xy/screenHeight) * 2.0 - 1.0; //[-1; 1]x[-1; 1]
    float radius = 0.3;
    float d = smoothstep(0.49, 0.51, length(uv) - radius);
    fragColor = vec4(d, d, d, 1.0);
}