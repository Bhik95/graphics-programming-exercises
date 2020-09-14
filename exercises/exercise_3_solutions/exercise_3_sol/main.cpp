#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <shader_s.h>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glmutils.h"

// the plane model is stored in the file so that we do not need to deal with model loading yet
#include "plane_model.h"

// structure to hold the info necessary to render an object
struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(std::vector<float> &array);
unsigned int createElementArrayBuffer(std::vector<unsigned int> &array);
unsigned int createVertexArray(std::vector<float> &positions, std::vector<float> &colors, std::vector<unsigned int> &indices);
void setup();
void drawSceneObject(SceneObject obj);
void drawPlane();

// glfw functions
// --------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
// --------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// plane parts
// -----------
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;

float currentTime;
Shader* shaderProgram;

// global variables used to set the plane and communicate
// its state between the input and draw functions
float planeSpeed = 0.005f;
glm::vec2 planePosition = glm::vec2(0.0,0.0);
float planeRotation = 0.0f;
float tilt = 0;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 3", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // build and compile our shader program
    // ------------------------------------
    shaderProgram = new Shader("shader.vert", "shader.frag");

    // the model was originally baked with lights for a left handed coordinate system, we are "fixing" the z-coordinate
    // so we can work with a right handed coordinate system
    invertModelZ(planeBodyVertices);
    invertModelZ(planeWingVertices);
    invertModelZ(planePropellerVertices);

    // setup mesh objects
    // ---------------------------------------
    setup();

    // NEW!
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

        processInput(window);

        glClearColor(0.5f, 0.5f, 1.0f, 1.0f);

        // NEW!
        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram->use();
        drawPlane();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}


void drawPlane(){
    // TODO 3.all create and apply your transformation matrices here
    //  you will need to transform the pose of the pieces of the plane by manipulating glm matrices and uploading a
    //  uniform mat4 model matrix to the vertex shader

    // rotation matrix based on current planeRotation
    glm::mat4 rotation = glm::rotateZ(planeRotation);

    // add rotated translation step in the xy plane to planePosition
    planePosition.x += (rotation * glm::vec4(0, planeSpeed, 0, 1)).x;
    planePosition.y += (rotation * glm::vec4(0, planeSpeed, 0, 1)).y;

    // wrap position
    planePosition.x *= (abs(planePosition.x) > 1.f) ? -1.f : 1.0;
    planePosition.y *= (abs(planePosition.y) > 1.f) ? -1.f : 1.0;

    // position matrix based on current planePosition
    glm::mat4 translation = glm::translate(planePosition.x, planePosition.y, 0);

    // scale matrix to make the plane 10 times smaller
    glm::mat4 scale = glm::scale(.1f, .1f, .1f);

    auto planeLeaning = glm::rotateY(glm::radians<float>(tilt));

    // final plane transformation, matrices are applied in the right to left order in the convention we use in the class
    // 10 times smaller -> leaning toward the turn direction -> rotation -> position
    glm::mat4 model = translation * rotation * planeLeaning * scale;

    // plane assembly matrices
    glm::mat4 mirrorX = glm::scale(-1, 1, 1);
    glm::mat4 translateWings = glm::translate(0, -0.5, 0);
    glm::mat4 translateProp = glm::translate(0, 0.5, 0);
    glm::mat4 rotateProp = glm::rotateX(glm::half_pi<float>());
    glm::mat4 scaleHalf = glm::scale(0.5, 0.5, 0.5);
    glm::mat4 animateProp = glm::rotateY(currentTime * 10);

    // get the id of the uniform called model
    unsigned int uniformID = glGetUniformLocation(shaderProgram->ID, "model");
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &model[0][0]);
    // body
    drawSceneObject(planeBody);
    // right wing
    drawSceneObject(planeWing);

    // back right wing
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &(model * translateWings * scaleHalf)[0][0]);
    drawSceneObject(planeWing);

    // left wing
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &(model * mirrorX)[0][0]);
    drawSceneObject(planeWing);

    // back left wing
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &(model * translateWings * scaleHalf * mirrorX)[0][0]);
    drawSceneObject(planeWing);

    // propeller
    glUniformMatrix4fv(uniformID, 1, GL_FALSE, &(model * translateProp * animateProp * rotateProp * scaleHalf)[0][0]);
    drawSceneObject(planePropeller);

}

void drawSceneObject(SceneObject obj){
    glBindVertexArray(obj.VAO);
    glDrawElements(GL_TRIANGLES,  obj.vertexCount, GL_UNSIGNED_INT, 0);
}

void setup(){

    // TODO 3.3 you will need to load one additional object.

    // initialize plane body mesh objects
    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    // initialize plane wing mesh objects
    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    // initialize plane propeller mesh object
    planePropeller.VAO = createVertexArray(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();
}


unsigned int createVertexArray(std::vector<float> &positions, std::vector<float> &colors, std::vector<unsigned int> &indices){
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

unsigned int createArrayBuffer(std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

    return VBO;
}

unsigned int createElementArrayBuffer(std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

    return EBO;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // TODO 3.4 control the plane (turn left and right) using the A and D keys
    // you will need to read A and D key press inputs
    // if GLFW_KEY_A is GLFW_PRESS, plane turn left
    // if GLFW_KEY_D is GLFW_PRESS, plane turn right
    tilt =  0;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        planeRotation += 0.02f;
        tilt = -45;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        planeRotation -= 0.02f;
        tilt = +45;
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