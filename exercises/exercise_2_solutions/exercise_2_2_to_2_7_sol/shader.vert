#version 330 core
// initial position, velocity and timeOfBirth of the particle, set in the application
layout (location = 0) in vec2 pos;
// TODO 2.2 add velocity and timeOfBirth as vertex attributes
layout (location = 1) in vec2 velocity;
layout (location = 2) in float timeOfBirth;


// TODO 2.3 create and use a float uniform for currentTime
// current application time. Set every new frame by the application.
uniform float currentTime;


// send the current age of the particl to the fragment shader
out float age;

// hard coded max age
float maxAge = 10.0;

void main()
{
    // TODO 2.3 use the currentTime to control the particle in different stages of its lifetime
    // elapsed time since particle birth
    age = currentTime - timeOfBirth;

    // is this particle dead?
    if (timeOfBirth == 0 || age > maxAge){
        // out of frustum (in this canse out of the NDC)
        gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);
    } else {
        // update position - integrate velocity with elapsed time
        gl_Position = vec4(pos + velocity * age, 0.0, 1.0);
    }

    // linear interpolation in the range [1,20] using normalized age
    gl_PointSize = mix(1.0, 20.0, age / maxAge);
}