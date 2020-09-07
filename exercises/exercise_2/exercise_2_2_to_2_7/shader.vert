#version 330 core

#define MAX_AGE 2

layout (location = 0) in vec2 pos;   // the position variable has attribute position 0
// TODO 2.2 add velocity and timeOfBirth as vertex attributes
layout (location = 1) in vec2 velocity;
layout (location = 2) in float timeOfBirth;

out float age;

// TODO 2.3 create and use a float uniform for currentTime
uniform float currentTime;

void main()
{
    // TODO 2.3 use the currentTime to control the particle in different stages of its lifetime
    age = currentTime - timeOfBirth;
    if(timeOfBirth == 0){
        gl_Position = vec4(-5.0, -5.0, 0.0, 1.0);
    }
    else{
        if(age > MAX_AGE){
            gl_Position = vec4(-5.0, -5.0, 0.0, 1.0);
        }
        else{
            gl_Position = vec4(pos + age*velocity, 0.0, 1.0);
            gl_PointSize = mix(0.1, 20.0, clamp(0.0, 1.0, age/MAX_AGE));
        }
    }
}