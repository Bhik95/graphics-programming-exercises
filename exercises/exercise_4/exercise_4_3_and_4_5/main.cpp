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

#include "primitives.h"
#include "plane_model.h"

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
void drawObject();

// glfw and input functions
// ------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void button_input_callback(GLFWwindow* window, int button, int action, int mods);
void processInput(GLFWwindow *window);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject planeBody;
SceneObject planeWing;
SceneObject planePropeller;
Shader* shaderProgram;

// global variables used for control
// ---------------------------------
float currentTime;
glm::vec3 clickStart(0.0f), clickEnd(0.0f);
glm::mat4 storedRotation(1.0f);


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
    glfwSetMouseButtonCallback(window, button_input_callback);

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

        glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderProgram->use();
        drawObject();

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

glm::vec3 shoemakeM(glm::vec2 input, float R){
    float XXplusYY = glm::dot(input, input);
    float R2 = R*R;
    if(XXplusYY <= R2){
        return glm::vec3(input.x, input.y, glm::sqrt(R2 - XXplusYY));
    }
    else{
        return (R / (glm::sqrt(XXplusYY))) * glm::vec3(input.x, input.y, 0);
    }
}

glm::vec3 andersonM(glm::vec2 input, float R){
    float XXplusYY = glm::dot(input, input); //X^2+Y^2
    float R2 = R*R; // R^2
    if(2 * XXplusYY <= R2){
        return glm::vec3(input.x, input.y, glm::sqrt(R2 - XXplusYY));
    }
    else{
        return glm::vec3(input.x, input.y, R2 / (2.0f * glm::sqrt(XXplusYY)));
    }
}

glm::mat4 trackballRotation(){
    glm::vec2 mouseVec =clickStart-clickEnd;
    if (glm::length(mouseVec) < 1.e-5f)
        return glm::mat4(1.0f);

    float dotProd = 0;
    float angle = 0;
    glm::vec3 u;
    glm::vec3 crossProd;
    float r = 1.0f; // trackball radius

    // TODO 4.3 - implement the trackball rotations here

    // TODO - prepare the values you will use for the trackball

    // TODO - Shoemake trackball
    /*glm::vec3 clickStartProj = shoemakeM(clickStart, r);
    glm::vec3 clickEndProj = shoemakeM(clickEnd, r);*/


    // TODO - Anderson trackball
    glm::vec3 clickStartProj = andersonM(clickStart, r);
    glm::vec3 clickEndProj = andersonM(clickEnd, r);

    dotProd = glm::dot(clickStartProj, clickEndProj);
    crossProd = glm::cross(clickStartProj, clickEndProj);
    float crossProdLength = glm::length(crossProd);
    u = glm::normalize(crossProd);
    angle = glm::atan(crossProdLength, dotProd);
    // correction to the rotation angle
    //angle += dotProd < 0.f ? glm::pi<float>() : 0.f; //Why does it have to be corrected, it works correctly without it
    glm::mat4 rotation = glm::rotate(abs(angle), u);

    return rotation;
}

void drawObject(){

    // scale matrix to make the plane 10 times smaller
    glm::mat4 scale = glm::scale(.1f, .1f, .1f);

    // final object transformation
    // scale -> rotations
    glm::mat4 model = trackballRotation() * storedRotation * scale;

    // TODO 4.5 - create a project using the glm::perspectiveFov function,
    //  and use it to view the object (i.e. multiply with model)
    glm::mat4 perspectiveMatrix = glm::perspectiveFov(glm::radians(70.0f), (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.01f, 10.0f);
    glm::vec3 cameraPos(0,0,1); // or vrp
    glm::vec3 forward(0,0,-1); // or -vpn
    glm::vec3 vup(0,1,0);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + forward, vup);

    model = perspectiveMatrix * view * model; //Perspective

    // TODO 4.4 - replace the cube with the plane from exercise 4.1/4.2
    // draw object
    shaderProgram->setMat4("model", model);
    drawSceneObject(planeBody);
    drawSceneObject(planeWing);

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

    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    // initialize plane body mesh objects
    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    // initialize plane wing mesh objects
    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

    // initialize plane wing mesh objects
    planePropeller.VAO = createVertexArray(planePropellerVertices, planePropellerColors, planePropellerIndices);
    planePropeller.vertexCount = planePropellerIndices.size();
}


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind the VBO
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


void cursorInNdc(GLFWwindow* window, float &x, float &y){
    double xPos, yPos;
    int xScreen, yScreen;
    glfwGetCursorPos(window, &xPos, &yPos);
    glfwGetWindowSize(window, &xScreen, &yScreen);
    float xNdc = (float) xPos / (float) xScreen * 2.0f - 1.0f;
    float yNdc = (float) yPos / (float) yScreen * 2.0f - 1.0f;
    yNdc = -yNdc;
    x = xNdc;
    y = yNdc;
}




void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        cursorInNdc(window, clickEnd.x, clickEnd.y);
    }
}



void button_input_callback(GLFWwindow* window, int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cursorInNdc(window, clickStart.x, clickStart.y);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        cursorInNdc(window, clickEnd.x, clickEnd.y);
        // store current rotation at the end of a click
        storedRotation = trackballRotation() * storedRotation;
        // reset click positions
        clickStart.x = clickStart.y = clickEnd.x = clickEnd.y = 0;
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