#version 330 core
// current particle age (input from the vertex shader)
in float age;
// final output color
out vec4 fragColor;

// hard coded max age
float maxAge = 10.0;

void main()
{
    // TODO 2.4 set the alpha value to 0.2 (alpha is the 4th value)
    // after 2.4, TODO 2.5 and 2.6: improve the particles appearance
    // put age in the range of [0,1]
    float ageNorm = age / maxAge;
    // opacity is linearly reduced the further away from the center of the point
    float alpha = 1.0 - length(gl_PointCoord - .5) * 2.0;
    // three colors used for interpolation
    vec3 colorFrom = vec3(1.0, 1.0, 0.05);
    vec3 colorMid = vec3(1.0, 0.5, 0.01);
    vec3 colorTo = vec3(0.0, 0.0, 0.0);
    // final color
    vec3 color;
    if (ageNorm < 0.5){
        // interpolate in the first half of the particle lifetime
        color = mix(colorFrom, colorMid, ageNorm * 2.0);
    }else{
        // interpolate in the second half of the particle lifetime
        color = mix(colorMid, colorTo, ageNorm * 2.0 - 1.0);
    }


    // final color, notice that the overall opacity is also controlled by the normalized particle age
    fragColor = vec4(color, alpha * (1.0 - ageNorm));

}