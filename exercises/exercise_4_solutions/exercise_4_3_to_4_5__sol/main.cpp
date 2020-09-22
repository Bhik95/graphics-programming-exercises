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
    void drawSceneObject(){
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES,  vertexCount, GL_UNSIGNED_INT, 0);
    }
};

// function declarations
// ---------------------
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void setup();
void drawObject();
void viewProjection(glm::mat4 &viewProj, bool perspective);

// glfw and input functions
// ------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void button_input_callback(GLFWwindow* window, int button, int action, int mods);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
// -----------------------------------
SceneObject cube;
SceneObject planeBody, planeWing, planePropeller;
Shader* shaderProgram;

// global variables used for control
// ---------------------------------
float currentTime;
glm::vec3 clickStart(0.0f), clickEnd(0.0f);
glm::mat4 storedRotation(1.0f);

bool g_andersonTrackball = false;
bool g_perspectiveProjection = false;
bool g_airplane = false;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 4.2 to 4.5", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetMouseButtonCallback(window, button_input_callback);
    glfwSetKeyCallback(window, key_input_callback);

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
    // NEW!
    // Notice that the depth range is now set to glDepthRange(-1,1), that is, a left handed coordinate system.
    // That is because the default openGL's NDC is in a left handed coordinate system (even though the default
    // glm and legacy openGL camera implementations expect the world to be in a right handed coordinate system);
    // so let's conform to that
    glDepthRange(-1,1); // make the NDC a LEFT handed coordinate system, with the camera pointing towards +z
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
    glm::vec3 pa;
    glm::vec3 pc;

    if(!g_andersonTrackball) {
        // TODO - Shoemake trackball
        pa = glm::length(clickStart) <= r ?
             glm::vec3(clickStart.x, clickStart.y,
                       sqrt(r * r - (clickStart.x * clickStart.x + clickStart.y * clickStart.y))) :
             r / glm::length(clickStart) * glm::vec3(clickStart.x, clickStart.y, 0);

        pc = glm::length(clickEnd) <= r ?
             glm::vec3(clickEnd.x, clickEnd.y, sqrt(r * r - (clickEnd.x * clickEnd.x + clickEnd.y * clickEnd.y))) :
             r / glm::length(clickEnd) * glm::vec3(clickEnd.x, clickEnd.y, 0);
    }else {
        // TODO - Anderson trackball
        pa = glm::length(clickStart) <= r / sqrt(2.0f) ?
             glm::vec3(clickStart.x, clickStart.y,
                       sqrt(r * r - (clickStart.x * clickStart.x + clickStart.y * clickStart.y))) :
             glm::vec3(clickStart.x, clickStart.y, r * r / (glm::length(clickStart) * 2.0f));

        pc = glm::length(clickEnd) <= r / sqrt(2.0f) ?
             glm::vec3(clickEnd.x, clickEnd.y, sqrt(r * r - (clickEnd.x * clickEnd.x + clickEnd.y * clickEnd.y))) :
             glm::vec3(clickEnd.x, clickEnd.y, r * r / (glm::length(clickEnd) * 2.0f));
    }

    // TODO - compute rotation axis and rotation angle, can use the same for both trackballs
    dotProd = glm::dot(pa, pc);
    crossProd = glm::cross(pa, pc);
    u = crossProd / glm::length(crossProd);
    angle = glm::atan(glm::length(crossProd), dotProd);

    // correction to the rotation angle - not needed when we use atan with two parameters (atan2)
    // angle = atan(glm::length(crossProd) / dotProd);
    // angle += dotProd < 0.f ? glm::pi<float>() : 0.f;

    glm::mat4 rotation = glm::rotate(abs(angle), u);

    return rotation;
}

void viewProjection(glm::mat4 &viewProj, bool perspective){
    if(!perspective) {
        // ortographic in the ndc range
        // we only need to mirror in z to compensate for the left handed coord. system in the ndc
        // (as the scene itself is still in a right coordinate system)
        glm::mat4 orthoProjection = glm::scale(1.f, 1.f, -1.f);
        viewProj = orthoProjection;
        return;
    }

    // TODO 4.5 - create a projection using the glm::perspectiveFov function,
    //  and use it to view the object (i.e. multiply with model)
    glm::mat4 projection = glm::perspectiveFov(glm::radians(70.0f), (float)SCR_WIDTH, (float)SCR_HEIGHT, .01f, 10.0f);
    // glm::lookat to define the pose of the camera, set position to (0,0,2) and the camera forward to (0,0,-1)
    glm::vec3 cameraPos(0,0,2); // or vrp
    glm::vec3 forward(0,0,-1); // or -vpn
    glm::vec3 vup(0,1,0);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + forward, vup);

    // set the matrix that take points in the world coordinate system and project them
    // world_to_view -> view_to_perspective_projection
    // or if we want ot match the multiplication order, we could read
    // perspective_projection_from_view <- view_from_world
    viewProj = projection * view;

}


void drawObject(){


    // final object transformation
    // scale -> rotations
    glm::mat4 model = trackballRotation() * storedRotation;


    glm::mat4 viewProj = glm::identity<glm::mat4>();
    viewProjection(viewProj, g_perspectiveProjection);
    glm::mat4 MVP = viewProj * model;

    if(!g_airplane) {
        // scale the cube to half the size (not needed for the plane)
        glm::mat4 scale = glm::scale(.5f, .5f, .5f);
        // draw cube
        shaderProgram->setMat4("model", MVP * scale);
        cube.drawSceneObject();
        return;
    }


    // TODO 4.4 - replace the cube with the plane from exercise 4.1/4.2
    // draw plane body and right wing

    shaderProgram->setMat4("model", MVP);
    planeBody.drawSceneObject();
    planeWing.drawSceneObject();

    // propeller,
    glm::mat4 propeller = glm::translate(.0f, .5f, .0f) *
                          glm::rotate(currentTime * 10.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(1.0,0.0,0.0)) *
                          glm::scale(.5f, .5f, .5f);

    shaderProgram->setMat4("model", MVP * propeller);
    planePropeller.drawSceneObject();

    // right wing back,
    glm::mat4 wingRightBack = glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(.5f,.5f,.5f);
    shaderProgram->setMat4("model", MVP * wingRightBack);
    planeWing.drawSceneObject();

    // left wing,
    glm::mat4 wingLeft = glm::scale(-1.0f, 1.0f, 1.0f);
    shaderProgram->setMat4("model", MVP * wingLeft);
    planeWing.drawSceneObject();

    // left wing back,
    glm::mat4 wingLeftBack =  glm::translate(0.0f, -0.5f, 0.0f) * glm::scale(-.5f,.5f,.5f);
    shaderProgram->setMat4("model", MVP * wingLeftBack);
    planeWing.drawSceneObject();

}


void setup(){
    // initialize shaders
    shaderProgram = new Shader("shaders/shader.vert", "shaders/shader.frag");

    cube.VAO = createVertexArray(cubeVertices, cubeColors, cubeIndices);
    cube.vertexCount = cubeIndices.size();

    planeBody.VAO = createVertexArray(planeBodyVertices, planeBodyColors, planeBodyIndices);
    planeBody.vertexCount = planeBodyIndices.size();

    planeWing.VAO = createVertexArray(planeWingVertices, planeWingColors, planeWingIndices);
    planeWing.vertexCount = planeWingIndices.size();

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


void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS){
        cursorInNdc(window, clickEnd.x, clickEnd.y);
    }
}

void button_input_callback(GLFWwindow* window, int button, int action, int mods){
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cursorInNdc(window, clickStart.x, clickStart.y);
        // reset clickEnd position
        cursorInNdc(window, clickEnd.x, clickEnd.y);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        cursorInNdc(window, clickEnd.x, clickEnd.y);
        // store current rotation at the end of a click
        storedRotation = trackballRotation() * storedRotation;
        // reset click positions
        clickStart.x = clickStart.y = clickEnd.x = clickEnd.y = 0;
    }
}

void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods){
    if (button == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (button == GLFW_KEY_1 && action == GLFW_PRESS)
        g_andersonTrackball = false;
    if (button == GLFW_KEY_2 && action == GLFW_PRESS)
        g_andersonTrackball = true;
    if (button == GLFW_KEY_3 && action == GLFW_PRESS)
        g_airplane = false;
    if (button == GLFW_KEY_4 && action == GLFW_PRESS)
        g_airplane = true;
    if (button == GLFW_KEY_5 && action == GLFW_PRESS)
        g_perspectiveProjection = false;
    if (button == GLFW_KEY_6 && action == GLFW_PRESS)
        g_perspectiveProjection = true;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}