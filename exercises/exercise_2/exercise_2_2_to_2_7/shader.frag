#version 330 core

out vec4 fragColor;

void main()
{
    // TODO 2.4 set the alpha value to 0.2 (alpha is the 4th value)
    // after 2.4, TODO 2.5 and 2.6: improve the particles appearance
    float d = clamp(1 - 2 * length(gl_PointCoord - vec2(0.5)), 0.0, 1.0);
    fragColor = vec4(1, 1, 1, d);

}