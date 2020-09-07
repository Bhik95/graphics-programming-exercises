#version 330 core

#define MAX_AGE 2

out vec4 fragColor;

in float age;

void main()
{
    // TODO 2.4 set the alpha value to 0.2 (alpha is the 4th value)
    // after 2.4, TODO 2.5 and 2.6: improve the particles appearance
    float d = clamp(1 - 2 * length(gl_PointCoord - vec2(0.5)), 0.0, 1.0);

    //Relative Age
    float relAge = age/MAX_AGE;

    vec3 col01 = mix(vec3(1.0, 1.0, 0.05), vec3(1.0, 0.5, 0.01), clamp(0.0, 1.0, relAge * 2));
    vec3 col12 = mix(col01, vec3(0.0, 0.0, 0.0), clamp(0.0, 1.0, relAge * 2 - 1));

    fragColor = vec4(col12, d * (1-relAge));

}