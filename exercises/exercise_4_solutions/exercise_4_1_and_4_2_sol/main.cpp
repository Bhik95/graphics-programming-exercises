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

    void drawSceneObject(){
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);
    }
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
Shader* shaderProgram;

// global variables used for control
// -----------------------------------
float currentTime;
glm::vec2 clickStart(0.0f), clickEnd(0.0f);

// TODO 4.1 and 4.2 - global variables you might need
bool isClicking = false;
SceneObject arrow;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 4.1 and 4.2", NULL, NULL);
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
    // TODO - 4.2 implement the draw arrow
    if(!isClicking)
        return;
    // implement the draw arrow
    // distance vector from clickStart to clickEnd
    glm::vec2 clickDiff = clickEnd - clickStart;
    // do not draw if the distance is too small
    if(glm::dot(clickDiff, clickDiff) < -1e5f)
        return;

    // set position at click start, set z value to draw it behind the plane
    glm::mat4 position = glm::translate(clickStart.x, clickStart.y, -.5f);

    // ROTATION METHOD 1
    // the local coordinates of the plane model are centralized and its nose is facing its local +Y component
    // we use this knowledge to build the (0, 1) vector, not unlike the slides, as if we were subtracting the points (0,1) - (0,0) (0 because the model is centralized)
    glm::vec2 planeForward (0.0f, 1.0f);
    // clickDiff defines the forward vector that we want the plane to align with
    // we compute the angle between the model forward and the forward we want the plane to face
    float cos = glm::dot(planeForward, glm::normalize(clickDiff));
    // define the rotation axis using the cross product
    glm::vec3 axis = glm::cross(glm::vec3(planeForward, .0f), glm::vec3(glm::normalize(clickDiff), 0.f));
    // we given sin and cos to the atan2 function, remember that |A x B| = |A||B|sin(angle) and that all our vectors have length 1
    float angle = glm::atan(glm::length(axis), cos);
    // set rotation matrix
    glm::mat4 rotation = glm::rotate(angle, axis);

    // set scale matrix
    glm::mat4 scale = glm::scale(.1f, glm::length(clickDiff), 1.0f);
    // set shader "model" matrix
    shaderProgram->setMat4("model", position * rotation * scale);

    drawSceneObject(arrow);
}

void drawPlane(){
    // TODO - 4.1 translate and rotate the plane

    static bool wasClicking = false;
    static glm::vec2 clickDiff(0.0f);
    static glm::vec2 planePosition(0.0f);
    static float planeRotationAngle = 0.0f;
    static float planeSpeed = 0.0f;

    // test for new click
    if (isClicking && !wasClicking){
        // stop plane
        planeSpeed = 0.0f ;
        // set plane position
        planePosition = clickStart;
    }
    else if (!isClicking && wasClicking){
        // speedup the plane
        planeSpeed = glm::length(clickDiff) * 0.03f ;
    }

    if(isClicking) {
        clickDiff = clickEnd - clickStart;
        // rotate the plane if mouseDir is not a zero vector
        if (glm::dot(clickDiff, clickDiff) > 1e-5f) {
            // ROTATION METHOD 2
            // the local coordinates of the plane model are centralized and its nose is facing its local +Y component
            // we use this knowledge to build the (0, 1) vector, not unlike the slides, as if we were subtracting the points (0,1) - (0,0) (0 because the model is centralized)
            glm::vec2 planeForward(0.0f, 1.0f);
            // we compute the angle between the model forward and the forward we want the plane to face
            planeRotationAngle = acos(glm::dot(glm::normalize(clickDiff), planeForward));
            // differentiate clockwise (positive) and counter clockwise (negative) rotations (the angle is always positive in the range [0,pi], we want it in [-pi,pi])
            if (clickDiff.x > 0) {
                planeRotationAngle *= -1.0f;
            }
        }
    }

    wasClicking = isClicking;

    // rotation matrix based on current planeRotationAngle
    // notice that we use a different method to achieve the exact same rotation as in the drawArrow function
    // here we make more assumptions, but will never take the cross product of colinear vectors (which would give an undefined direction vector as the rotation axis)
    glm::mat4 rotation = glm::rotate(planeRotationAngle, glm::vec3(0,0,1));

    // add rotated translation step in the xy plane to planePosition
    planePosition.x += (rotation * glm::vec4(0, planeSpeed, 0, 1)).x;
    planePosition.y += (rotation * glm::vec4(0, planeSpeed, 0, 1)).y;

    // wrap position
    // not asked in the exercise, but makes for a cool effect :)
    // let's use the glm float modulo, this particular implementation of the modulo can guarantee that our result is
    // the range [0, 2] (e.g. it won't return negative numbers, while certain implementations will)
    planePosition.x = glm::mod(planePosition.x + 1.0f, 2.0f) -1.0f;
    planePosition.y = glm::mod(planePosition.y + 1.0f, 2.0f) -1.0f;

    // position matrix based on current planPosition
    glm::mat4 translation = glm::translate(planePosition.x, planePosition.y, 0);

    // scale matrix to make the plane 10 times smaller
    glm::mat4 scale = glm::scale(.1f, .1f, .1f);

    // final plane transformation, matrices are applied in the right to left order in the convention we use in the class
    // 10 times smaller -> leaning toward the turn direction -> plane rotation -> plane position
    glm::mat4 model = translation * rotation * scale;

    // draw plane body and right wing
    shaderProgram->setMat4("model", model);
    planeBody.drawSceneObject();
    planeWing.drawSceneObject();

    // propeller,
    // half size -> make perpendicular to plane forward axis -> rotate around plane forward axis -> move to the tip of the plane
    glm::mat4 propeller = model * glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgram->setMat4("model", propeller);
    planePropeller.drawSceneObject();

    // right wing back,
    // half size -> move to the back
    glm::mat4 wingRightBack = model * glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingRightBack);
    planeWing.drawSceneObject();

    // left wing,
    // mirror in x
    glm::mat4 wingLeft = model * glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgram->setMat4("model", wingLeft);
    planeWing.drawSceneObject();

    // left wing back,
    // half size + mirror in x -> move to the back
    glm::mat4 wingLeftBack =  model *  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgram->setMat4("model", wingLeftBack);
    planeWing.drawSceneObject();

}



void drawSceneObject(SceneObject obj){
    glBindVertexArray(obj.VAO);
    glDrawElements(GL_TRIANGLES,  obj.vertexCount, GL_UNSIGNED_INT, 0);
}

void setup(){
    // initialize shaders
    shaderProgram = new Shader("shaders/shader.vert", "shaders/shader.frag");

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

        isClicking = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        // set the end position
        cursorInNdc(screenX, screenY, screenW, screenH, clickEnd.x, clickEnd.y);
        // reset the start position
        cursorInNdc(screenX, screenY, screenW, screenH, clickStart.x, clickStart.y);

        isClicking = false;
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