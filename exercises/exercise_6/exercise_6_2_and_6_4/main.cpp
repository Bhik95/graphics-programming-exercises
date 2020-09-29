#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <math.h>
#include <chrono>

#include "glmutils.h"
#include "shader.h"
#include "software_renderer_lib/srl_frame_buffer.h"
#include "software_renderer_lib/srl_line_renderer.h"
#include "software_renderer_lib/srl_point_renderer.h"
#include "software_renderer_lib/srl_triangle_renderer.h"
#include "models.h"


// glfw callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void button_input_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);

// other function declarations
unsigned int setup();
glm::mat4 trackballRotation();
void cursorInNdc(float screenX, float screenY, int screenW, int screenH, float &x, float &y);

// screen settings
const unsigned int SCR_WIDTH = 512;
const unsigned int SCR_HEIGHT = 512;

// used to render two triangles with OurGL frame buffer as a texture
Shader * shader;
unsigned int VAO;
int vertexCount;

// global variables used for control
glm::vec3 clickStart(0.0f), clickEnd(0.0f);
glm::mat4 storedRotation(1.0f);

// create instances of the renderers
srl::PointRenderer pointR;
srl::LineRenderer lineR;
srl::TriangleRenderer triangleR;
// set current renderer
srl::Renderer *srlRenderer = &triangleR;

bool drawPlane = true;


int main()
{
    // glfw: initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Software renderer", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // setup frame buffer size callback
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, button_input_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetKeyCallback(window, key_input_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile the shader programs
    shader = new Shader("shaders/shader.vert", "shaders/shader.frag");

    // NEW!
    // create and initialize a frame buffer
    int width = 128, height = 128;
    srl::FrameBuffer<uint32_t> buffer(width, height);
    srl::FrameBuffer<float> zBuffer(width, height);

    // NEW!
    // create a texture
    unsigned int srlTexture;

    glGenTextures(1, &srlTexture);
    glBindTexture(GL_TEXTURE_2D, srlTexture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // give our frame buffer to opengl
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.buffer());


    unsigned int depthTexture;

    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // give our frame buffer to opengl
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, zBuffer.buffer());


    // associate texture uniform in the shader to our texture
    shader->setInt("srlTexture", 0);

    // create square mesh
    VAO = setup();

    // NEW!
    // load objects into vectors
    std::vector<srl::vertex> vtsBody, vtsWing, vtsProp;
    std::vector<float> points;
    std::vector<float> colors;
    indicesToValueArray(planeBodyVertices, planeBodyIndices, 3, points);
    indicesToValueArray(planeBodyColors, planeBodyIndices, 4, colors);
    for (unsigned int i = 0; i < points.size()/3; i++){
        srl::vertex v;
        v.pos = glm::vec4(points[i*3], points[i*3+1], points[i*3+2], 1.0f);
        v.col = {colors[i*4], colors[i*4+1], colors[i*4+2], colors[i*4+3]};
        vtsBody.push_back(v);
    }

    points.clear(); colors.clear();
    indicesToValueArray(planeWingVertices, planeWingIndices, 3, points);
    indicesToValueArray(planeWingColors, planeWingIndices, 4, colors);
    for (unsigned int i = 0; i < points.size()/3; i++){
        srl::vertex v;
        v.pos = glm::vec4(points[i*3], points[i*3+1], points[i*3+2], 1.0f);
        v.col = {colors[i*4], colors[i*4+1], colors[i*4+2], colors[i*4+3]};
        vtsWing.push_back(v);
    }
    // TODO load the propeller



    // create our camera pose and projection matrix
    glm::mat4 viewProj = glm::perspectiveFovRH_NO<float>(glm::radians(50.0f),
                                                         (float)width , (float)height, .5f, 5.0f)
                         * glm::lookAt<float>(glm::vec3(.0f, .0f, 2.5f),
                                            glm::vec3(.0f, .0f, .0f),
                                            glm::vec3(.0f, 1.f, .0f));


    // render every loopInterval seconds
    float loopInterval = 0.01667f;
    auto begin = std::chrono::high_resolution_clock::now();


    // render loop
    while (!glfwWindowShouldClose(window)) {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;

        // SOFTWARE RENDERER LIB part
        // clear buffers
        srl::color clearColor = srl::color::grey();
        buffer.clearBuffer(clearColor.getRGBA32());
        zBuffer.clearBuffer(1.0f);

        // set model view projection (mvp) transformation
        glm::mat4 mvp = viewProj * trackballRotation() * storedRotation;

        // render the body and right wing of the plane to our frame buffer using our graphics library.
        srlRenderer->render(vtsBody, mvp, buffer, zBuffer);
        srlRenderer->render(vtsWing, mvp, buffer, zBuffer);

        // TODO render the propeller




        // OPENGL PART! This is only used to display the software rendered image in the screen!
        // background color
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        // notice that now we are clearing two buffers, the color and the z-buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set the color buffer as the active texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srlTexture);
        // upload the color buffer
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.buffer());
        // render as a square of the size of the screen
        shader->use();
        shader->setMat4("mvp", glm::mat4(1.0f));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);

        // set the depth buffer as the active texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);
        // upload the depth buffer
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, zBuffer.buffer());
        // render on the top right corner
        shader->use();
        shader->setMat4("mvp", glm::translate(0.7f, 0.7f, 0.0f) * glm::scale(0.3f, 0.3f, 0.3f));
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_INT, 0);


        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();


        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }
    delete shader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

unsigned int setup(){
    // this is used to setup a quad of the size of the screen using two triangles
    // position and texture coordinates (x, y, z u, v)
    std::vector<float> vertices = {-1.0f, -1.f, 0.f, 0.f, 0.f,
                                    1.0f, -1.f, 0.f, 1.f, 0.f,
                                    1.0f,  1.f, 0.f, 1.f, 1.f,
                                   -1.0f,  1.f, 0.f, 0.f, 1.f};
    std::vector<unsigned int> indices = {0, 1, 2, 0, 2, 3};

    // declare and generate a VAO, VBO and EBO
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // bind and set the VAO, VBO and EBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLfloat), &indices[0], GL_STATIC_DRAW);

    // store the number of vertices in the mesh
    vertexCount = indices.size();

    // set the position and color attribute pointers in the shader
    int posSize = 3;
    int texCoordSize = 2;

    int posAttributeLocation = glGetAttribLocation(shader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, posSize, GL_FLOAT, GL_FALSE, sizeof(float) * (posSize + texCoordSize), 0);

    // set vertex shader attribute "uvCoord"
    int colorAttributeLocation = glGetAttribLocation(shader->ID, "uvCoord");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, texCoordSize, GL_FLOAT, GL_FALSE, sizeof(float) * (posSize + texCoordSize) , (void*)(posSize * sizeof(float)));

    return VAO;
}

glm::mat4 trackballRotation(){
    glm::vec2 mouseVec =clickStart-clickEnd;
    if (glm::length(mouseVec) < 1e-5)
        return glm::mat4(1.0f);

    float dotProd = 0;
    float angle = 0;
    glm::vec3 u;
    glm::vec3 crossProd;
    float r = 1.0f; // trackball radius

    glm::vec3 pa;
    glm::vec3 pc;

    // Anderson trackball
    pa = glm::length(clickStart) <= r/sqrt(2.0f) ?
         glm::vec3(clickStart.x, clickStart.y, sqrt(r*r - (clickStart.x*clickStart.x + clickStart.y*clickStart.y))) :
         glm::vec3(clickStart.x, clickStart.y, r*r/(glm::length(clickStart) * 2.0f));

    pc = glm::length(clickEnd) <= r/sqrt(2.0f) ?
         glm::vec3(clickEnd.x, clickEnd.y, sqrt(r*r - (clickEnd.x*clickEnd.x + clickEnd.y*clickEnd.y))) :
         glm::vec3(clickEnd.x, clickEnd.y, r*r/(glm::length(clickEnd) * 2.0f ));

    dotProd = glm::dot(pa, pc);
    crossProd = glm::cross(pa, pc);
    u = crossProd / glm::length(crossProd);
    angle = atan(glm::length(crossProd) / dotProd);

    // correction to the rotation angle
    angle += dotProd < 0.f ? glm::pi<float>() : 0.f;
    glm::mat4 rotation = glm::rotate(abs(angle), u);

    return rotation;
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
        cursorInNdc((float)posX, (float)posY, screenW, screenH, clickEnd.x, clickEnd.y);
    }
}


void button_input_callback(GLFWwindow* window, int button, int action, int mods){
    double screenX, screenY;
    int screenW, screenH;
    glfwGetCursorPos(window, &screenX, &screenY);
    glfwGetWindowSize(window, &screenW, &screenH);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        cursorInNdc((float)screenX, (float)screenY, screenW, screenH, clickStart.x, clickStart.y);
        // reset clickEnd position
        cursorInNdc((float)screenX, (float)screenY, screenW, screenH, clickEnd.x, clickEnd.y);
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        cursorInNdc((float)screenX, (float)screenY, screenW, screenH, clickEnd.x, clickEnd.y);
        // store current rotation at the end of a click
        storedRotation = trackballRotation() * storedRotation;
        // reset click positions
        clickStart.x = clickStart.y = clickEnd.x = clickEnd.y = 0;
    }
}


void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods){
    if (button == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    // set the renderer using keys 1, 2 and 3
    if (button == GLFW_KEY_1 && action == GLFW_PRESS) {
        srlRenderer = &pointR;
    }
    if (button == GLFW_KEY_2 && action == GLFW_PRESS) {
        srlRenderer = &lineR;
    }
    if (button == GLFW_KEY_3 && action == GLFW_PRESS){
        srlRenderer = &triangleR;
    }
    if (button == GLFW_KEY_4 && action == GLFW_PRESS)
        drawPlane = true;
    if (button == GLFW_KEY_5 && action == GLFW_PRESS)
        drawPlane = false;

}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}