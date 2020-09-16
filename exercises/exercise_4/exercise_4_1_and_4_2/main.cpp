#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>
#include <shader.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glmutils.h"

#include "plane_model.h"
#include "primitives.h"


// structure to hold render info
// -----------------------------
struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
};


// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawSceneObject(SceneObject obj);
void drawArrow();
void drawPlane();


// glfw and input functions
// ------------------------
void cursorInNdc(float screenX, float screenY, int screenW, int screenH, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void button_input_callback(GLFWwindow* window, int button, int action, int mods);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);


// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;


// global variables used for rendering
// -----------------------------------
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;
SceneObject arrow;
Shader* shaderProgram;

// global variables used for control
// -----------------------------------
float currentTime;
glm::vec2 clickStart(0.0f), clickEnd(0.0f);

// TODO 4.1 and 4.2 - global variables you might need
glm::vec2 planePosition(0.0f);
float planeRotation = 0.0f;
glm::vec2 startingPosition(0.0f);
glm::vec2 targetPosition(0.0f);
bool hasTarget = false;
float launchTime = 0.0f;
bool pressing = false;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 4", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    // NEW!
    // callbacks for all inputs
    glfwSetMouseButtonCallback(window, button_input_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetKeyCallback(window, key_input_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // the model was originally baked with lights for a left handed coordinate system, we are "fixing" the z-coordinate
    // so we can work with a right handed coordinate system
    invertModelZ(planeBodyVertices);
    invertModelZ(planeWingVertices);
    invertModelZ(planePropellerVertices);

    // setup mesh objects
    // ---------------------------------------
    setup();

    // set up the z-buffer
    glDepthRange(1,-1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC


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

        glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram->use();
        // NEW!
        // we now have a function to draw the arrow too
        drawArrow();
        drawPlane();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    delete shaderProgram;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void drawArrow(){
    if(!pressing)
        return;

    // TODO - 4.2 implement the draw arrow
    glm::mat4 rotation = glm::rotateZ(planeRotation);
    glm::mat4 translation = glm::translate(planePosition.x, planePosition.y, 0.0f);

    glm::vec2 deltaClick = clickEnd - planePosition;
    float length = glm::length(deltaClick);
    // scale matrix to make the plane 10 times smaller
    glm::mat4 scale = glm::scale(0.1f, length, 0.1f);

    // final plane transformation, matrices are applied in the right to left order in the convention we use in the class
    // 10 times smaller -> leaning toward the turn direction -> plane rotation -> plane position
    glm::mat4 model = translation * rotation * scale;

    // draw plane body and right wing
    shaderProgram->setMat4("model", model);
    drawSceneObject(arrow);
}

float smoothstep(float x){
    if( x <= 0.0f)
        return 0.0f;
    else if ( x >= 1.0f )
        return 1.0f;
    return x*x*(3-2*x);
}

void drawPlane(){
    // TODO - 4.1 translate and rotate the plane

    float tweenDuration = 2.0f;

    if(pressing){
        glm::vec2 deltaClick = clickEnd - planePosition;
        planeRotation = atan2(-deltaClick.x, deltaClick.y); // The order of the parameters is intentionally that way
    }

    float t = (currentTime - launchTime) / tweenDuration;
    //std::cout << "t: " << t << " " << (hasTarget ? "Y" : "N") << std::endl;
    if(hasTarget){
        t = smoothstep(glm::clamp(t, 0.0f, 1.0f));
        planePosition = (1-t)*startingPosition + t*targetPosition;
        if(t >= 1)
            hasTarget = false;
    }

    glm::mat4 rotation = glm::rotateZ(planeRotation);
    glm::mat4 translation = glm::translate(planePosition.x, planePosition.y, 0.0f);


    // scale matrix to make the plane 10 times smaller
    glm::mat4 scale = glm::scale(.1f, .1f, .1f);

    // final plane transformation, matrices are applied in the right to left order in the convention we use in the class
    // 10 times smaller -> leaning toward the turn direction -> plane rotation -> plane position
    glm::mat4 model = translation * rotation * scale;

    // draw plane body and right wing
    shaderProgram->setMat4("model", model);
    drawSceneObject(planeBody);
    drawSceneObject(planeWing);

    // propeller,
    // half size -> make perpendicular to plane forward axis -> rotate around plane forward axis -> move to the tip of the plane
    glm::mat4 propeller = model * glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgram->setMat4("model", propeller);
    drawSceneObject(planePropeller);

    // right wing back,
    // half size -> move to the back
    glm::mat4 wingRightBack = model * glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingRightBack);
    drawSceneObject(planeWing);

    // left wing,
    // mirror in x
    glm::mat4 wingLeft = model * glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgram->setMat4("model", wingLeft);
    drawSceneObject(planeWing);

    // left wing back,
    // half size + mirror in x -> move to the back
    glm::mat4 wingLeftBack =  model *  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingLeftBack);
    drawSceneObject(planeWing);

}



void drawSceneObject(SceneObject obj){
    glBindVertexArray(obj.VAO);
    glDrawElements(GL_TRIANGLES,  obj.vertexCount, GL_UNSIGNED_INT, 0);
}

void setup(){
    // initialize shaders
    shaderProgram = new Shader("shader.vert", "shader.frag");

    // initialize plane body mesh objects
    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    // initialize plane wing mesh objects
    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    // initialize plane wing mesh objects
    planePropeller.VAO = createVertexArray(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();

    // TODO 4.2 - load the arrow mesh
    arrow.VAO = createVertexArray(arrowVertices, arrowColors, arrowIndices);
    arrow.vertexCount = arrowIndices.size();

}


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(shaderProgram->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(shaderProgram->ID, "color");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // creates and bind the EBO
    createElementArrayBuffer(indices);

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



void cursorInNdc(float screenX, float screenY, int screenW, int screenH, float &x, float &y){
    float xNdc = (float) screenX / (float) screenW * 2.0f - 1.0f;
    float yNdc = (float) screenY / (float) screenH * 2.0f - 1.0f;
    x = xNdc;
    y = -yNdc;
}


void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        int screenW, screenH;
        glfwGetWindowSize(window, &screenW, &screenH);
        cursorInNdc(posX, posY, screenW, screenH, clickEnd.x, clickEnd.y);
    }
}


void button_input_callback(GLFWwindow* window, int button, int action, int mods){
    double screenX, screenY;
    int screenW, screenH;
    glfwGetCursorPos(window, &screenX, &screenY);
    glfwGetWindowSize(window, &screenW, &screenH);

    // TODO 4.1 and 4.2 - you may wish to update some of your global variables here

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // set the start position
        cursorInNdc(screenX, screenY, screenW, screenH, clickStart.x, clickStart.y);
        // reset the end position
        cursorInNdc(screenX, screenY, screenW, screenH, clickEnd.x, clickEnd.y);

        pressing = true;
        planePosition = clickEnd;
        startingPosition = planePosition;
        hasTarget = false;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        // set the end position
        cursorInNdc(screenX, screenY, screenW, screenH, clickEnd.x, clickEnd.y);
        // reset the start position
        cursorInNdc(screenX, screenY, screenW, screenH, clickStart.x, clickStart.y);
        hasTarget = true;
        targetPosition = clickEnd;
        launchTime = currentTime;
        pressing = false;
    }

}


void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods){
    if (button == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}