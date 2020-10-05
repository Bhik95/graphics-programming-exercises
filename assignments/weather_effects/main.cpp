#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>
#include <math.h>

#include "shader.h"
#include "glmutils.h"

#include "plane_model.h"
#include "primitives.h"

// structure to hold render info
// -----------------------------
struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
    void drawSceneObject() const{
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArraySolid(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
unsigned int createVertexArrayParticles(unsigned int particlesNumber, int i);
void setup();
void drawObjects();
void drawParticles();

// glfw and input functions
// ------------------------
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void drawCube(glm::mat4 model);
void drawPlane(glm::mat4 model);

const unsigned int BOX_SIZE = 30;
const unsigned int ITERATIONS = 10;

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject floorObj;
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;
SceneObject particles[ITERATIONS];
unsigned int particlesVBO[ITERATIONS];
Shader* shaderProgramSolid;
Shader* shaderProgramParticle;

// global variables used for control
// ---------------------------------
float currentTime;
glm::vec3 camForward(.0f, .0f, -1.0f);
glm::vec3 camPosition(.0f, 1.6f, 0.0f);
float linearSpeed = 0.15f, rotationGain = 30.0f;

const unsigned int particleVertexBufferSize = 65536 / ITERATIONS;
const unsigned int particleSize = 3;
const float lineLength = 0.01f;

glm::vec3 gravityOffset[ITERATIONS];
glm::vec3 windOffset[ITERATIONS];
glm::vec3 gravityDeltas[ITERATIONS];

glm::mat4 modelViewProj[ITERATIONS];

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 4.6", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // setup mesh objects
    // ---------------------------------------
    setup();

    // set up the z-buffer
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system);
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE); //enable particle size

    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgramSolid->use();
        drawObjects();

        shaderProgramParticle->use();
        drawParticles();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete shaderProgramSolid;
    delete shaderProgramParticle;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void drawObjects(){

    glm::mat4 scale = glm::scale(1.f, 1.f, 1.f);

    glm::mat4 projection = glm::perspectiveFovRH_NO(70.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
    glm::mat4 view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));
    glm::mat4 viewProjection = projection * view;

    // draw floor (the floor was built so that it does not need to be transformed)
    shaderProgramSolid->setMat4("model", viewProjection);
    floorObj.drawSceneObject();

    // draw 2 cubes and 2 planes in different location and with different orientations
    drawCube(viewProjection * glm::translate(2.0f, 1.f, 2.0f) * glm::rotateY(glm::half_pi<float>()) * scale);
    drawCube(viewProjection * glm::translate(-2.0f, 1.f, -2.0f) * glm::rotateY(glm::quarter_pi<float>()) * scale);

    drawPlane(viewProjection * glm::translate(-2.0f, .5f, 2.0f) * glm::rotateX(glm::quarter_pi<float>()) * scale);
    drawPlane(viewProjection * glm::translate(2.0f, .5f, -2.0f) * glm::rotateX(glm::quarter_pi<float>()*3.f) * scale);

}

void drawParticles(){

    // Enable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::vec3 offset;
    for(int i=0;i<ITERATIONS;i++){

        glm::mat4 modelViewProjPrev = modelViewProj[i];

        //Update offset
        gravityOffset[i] += gravityDeltas[i];

        //Calculate actual offsets
        offset = gravityOffset[i] + windOffset[i] + glm::vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE);
        offset -= camPosition + camForward + glm::vec3(BOX_SIZE/2, BOX_SIZE/2, BOX_SIZE/2);

        glm::vec3 position = glm::mod(camPosition + offset, glm::vec3(BOX_SIZE, BOX_SIZE, BOX_SIZE));
        position += camPosition + camForward - glm::vec3(BOX_SIZE/2, BOX_SIZE/2, BOX_SIZE/2);

        glm::mat4 projection = glm::perspectiveFovRH_NO(70.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 100.0f);
        glm::mat4 view = glm::lookAt(camPosition, camPosition + camForward, glm::vec3(0,1,0));

        glm::mat4 translation = glm::translate(position);

        modelViewProj[i] = projection*view*translation;

        shaderProgramParticle->setMat4("modelViewProj", modelViewProj[i]);
        shaderProgramParticle->setMat4("modelViewProjPrev", modelViewProjPrev);
        shaderProgramParticle->setFloat("lineLength", lineLength);
        shaderProgramParticle->setVec3("boxSize", glm::vec3(BOX_SIZE,BOX_SIZE,BOX_SIZE));
        shaderProgramParticle->setVec3("offset", offset);


        glBindVertexArray(particles[i].VAO);
        glDrawArrays(GL_LINES, 0, particles[i].vertexCount);
    }
}


void drawCube(glm::mat4 model){
    // draw object
    shaderProgramSolid->setMat4("model", model);
    cube.drawSceneObject();
}


void drawPlane(glm::mat4 model){

    // draw plane body and right wing
    shaderProgramSolid->setMat4("model", model);
    planeBody.drawSceneObject();
    planeWing.drawSceneObject();

    // propeller,
    glm::mat4 propeller = model * glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgramSolid->setMat4("model", propeller);
    planePropeller.drawSceneObject();

    // right wing back,
    glm::mat4 wingRightBack = model * glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgramSolid->setMat4("model", wingRightBack);
    planeWing.drawSceneObject();

    // left wing,
    glm::mat4 wingLeft = model * glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgramSolid->setMat4("model", wingLeft);
    planeWing.drawSceneObject();

    // left wing back,
    glm::mat4 wingLeftBack =  model *  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgramSolid->setMat4("model", wingLeftBack);
    planeWing.drawSceneObject();
}



void setup(){
    // initialize shaders
    shaderProgramSolid = new Shader("shaders/solid.vert", "shaders/solid.frag");
    shaderProgramParticle = new Shader("shaders/particle.vert", "shaders/particle.frag");

    // load floor mesh into openGL
    floorObj.VAO = createVertexArraySolid(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();

    // load cube mesh into openGL
    cube.VAO = createVertexArraySolid(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // load plane meshes into openGL
    planeBody.VAO = createVertexArraySolid(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    planeWing.VAO = createVertexArraySolid(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    planePropeller.VAO = createVertexArraySolid(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();

    for(int i=0;i<ITERATIONS;i++){
        particles[i].VAO = createVertexArrayParticles(particleVertexBufferSize, i);
        particles[i].vertexCount = particleVertexBufferSize;

        //Setup the speed of the particles (gravity simulations)
        float minSpeed = 0.05f;
        float maxSpeed = 0.1f;
        float t = i / (float)ITERATIONS;
        float v = t*maxSpeed + (1-t)*minSpeed;
        gravityDeltas[i] =  glm::vec3 (0, -v, 0); //Direction: down
    }



}


unsigned int createVertexArraySolid(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(shaderProgramSolid->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(shaderProgramSolid->ID, "color");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // creates and bind the EBO
    createElementArrayBuffer(indices);

    return VAO;
}

unsigned int createVertexArrayParticles(unsigned int particlesNumber, int i){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);

    glGenBuffers(1, &particlesVBO[i]);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, particlesVBO[i]);

    // initialize particle buffer, set all values to 0
    std::vector<float> data(particleVertexBufferSize * particleSize);

    float r;
    for(unsigned int i = 0; i < data.size(); i++){
        if((i/particleSize) % 2 == 1){
            data[i] = data[i - particleSize];//2nd vertex of the
        }
        else{
            r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
            data[i] = BOX_SIZE + r * BOX_SIZE;//[BoxSize;2BoxSize]
        }


    }


    // allocate at openGL controlled memory
    glBufferData(GL_ARRAY_BUFFER, particleVertexBufferSize * particleSize * sizeof(float), &data[0], GL_DYNAMIC_DRAW);

    int posSize = 3; // each position has x,y and z
    GLuint vertexLocation = glGetAttribLocation(shaderProgramParticle->ID, "pos");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE, particleSize * sizeof(float), 0);


    return VAO;
}

unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

    return VBO;
}


unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

    return EBO;
}

// NEW!
// instead of using the NDC to transform from screen space you now can define the range using the
// min and max parameters
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y){
    float sum = max - min;
    float xInRange = (float) screenX / (float) screenW * sum - sum/2.0f;
    float yInRange = (float) screenY / (float) screenH * sum - sum/2.0f;
    x = xInRange;
    y = -yInRange; // flip screen space y axis
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    // TODO
    // rotate the camera position based on mouse movements
    // if you decide to use the lookAt function, make sure that the up vector and the
    // vector from the camera position to the lookAt target are not collinear
    static float rotationAroundVertical = 0;
    static float rotationAroundLateral = 0;

    int screenW, screenH;

    // get cursor position and scale it down to a smaller range
    glfwGetWindowSize(window, &screenW, &screenH);
    glm::vec2 cursorPosition(0.0f);
    cursorInRange(posX, posY, screenW, screenH, -1.0f, 1.0f, cursorPosition.x, cursorPosition.y);

    // initialize with first value so that there is no jump at startup
    static glm::vec2 lastCursorPosition = cursorPosition;

    // compute the cursor position change
    auto positionDiff = cursorPosition - lastCursorPosition;

    // require a minimum threshold to rotate
    if (glm::dot(positionDiff, positionDiff) > 1e-5f){
        // rotate the forward vector around the Y axis, notices that w is set to 0 since it is a vector
        rotationAroundVertical += glm::radians(-positionDiff.x * rotationGain);
        camForward = glm::rotateY(rotationAroundVertical) * glm::vec4(0,0,-1,0);
        // rotate the forward vector around the lateral axis
        rotationAroundLateral +=  glm::radians(positionDiff.y * rotationGain);
        // we need to clamp the range of the rotation, otherwise forward and Y axes get parallel
        rotationAroundLateral = glm::clamp(rotationAroundLateral, -glm::half_pi<float>() * 0.9f, glm::half_pi<float>() * 0.9f);
        glm::vec3 lateralAxis = glm::cross(camForward, glm::vec3(0, 1,0));
        camForward = glm::rotate(rotationAroundLateral, lateralAxis) * glm::rotateY(rotationAroundVertical) * glm::vec4(0,0,-1,0);
        camForward = glm::normalize(camForward);

        // save current cursor position
        lastCursorPosition = cursorPosition;
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // TODO
    // move the camera position based on keys pressed (use either WASD or the arrow keys)
    // camera forward in the XZ plane
    glm::vec3 forwardInXZ = glm::normalize(glm::vec3(camForward.x, 0, camForward.z));
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        camPosition += forwardInXZ * linearSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        camPosition -= forwardInXZ * linearSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        camPosition -= glm::cross(forwardInXZ, glm::vec3(0, 1, 0)) * linearSpeed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        camPosition += glm::cross(forwardInXZ, glm::vec3(0, 1, 0)) * linearSpeed;
    }
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}