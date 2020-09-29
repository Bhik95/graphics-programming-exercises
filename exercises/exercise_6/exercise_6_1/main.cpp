#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader_s.h"
#include "glmutils.h"

#include "trianglerasterizer.h"

struct SceneObject{
    unsigned int VAO;
    unsigned int VBO;
    unsigned int vertexCount;
    bool shouldUpdate = false;
    void drawSceneObject(uint16_t glMode){
        glBindVertexArray(VAO);
        glDrawArrays(glMode, 0, vertexCount);
    }
};

unsigned int createArrayBuffer(const std::vector<float> &array);
void modifyArrayBuffer(const std::vector<float> &array, unsigned int VBO);
unsigned int createVertexArray(const Shader& shader, unsigned int VBO);
void setup();
void drawObjects();
void cursorInNdc(float screenX, float screenY, int screenW, int screenH, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);

// settings
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

// global variables used for rendering
SceneObject triangleSO;
SceneObject gridSO;
SceneObject trianglePointsSO;


float currentTime;
Shader shaderProgramLines;
Shader shaderProgramDots;

// grid size
int min_x  = -9;
int min_y  = -9;
int max_x  =  9;
int max_y  =  9;
// triangle vertices
int x_1  = -5;
int y_1  = -6;
int x_2  =  6;
int y_2  =  4;
int x_3  = -2;
int y_3  =  3;

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
    glEnable(GL_PROGRAM_POINT_SIZE);
    setup();

    // set up the z-buffer
    glDepthRange(-1,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC


    // The main loop
    std::cout << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << "* Use the arrow keys to manipulate the vertices of the triangle *" << std::endl;
    std::cout << "* All three vertices can be modified.                           *" << std::endl;
    std::cout << "*                                                               *" << std::endl;
    std::cout << "*      vertex 1      |      vertex 2      |      vertex 3       *" << std::endl;
    std::cout << "*                    |                    |                     *" << std::endl;
    std::cout << "*         W          |         T          |         I           *" << std::endl;
    std::cout << "*       A S D        |       F G H        |       J K L         *" << std::endl;
    std::cout << "*                                                               *" << std::endl;
    std::cout << "* Press ESC to finish the program                               *" << std::endl;
    std::cout << "*****************************************************************" << std::endl;
    std::cout << std::endl;

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

        drawObjects();

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


std::vector<float> GenerateTrianglePixels(int x_1, int y_1, int x_2, int y_2, int x_3, int y_3)
{
    std::vector<glm::ivec2> pixels;
    triangle_rasterizer triangle(x_1, y_1, x_2, y_2, x_3, y_3);
    pixels = triangle.all_pixels();
    std::vector<float> pixelsIn3DCoord;
    for (auto it = pixels.begin(); it < pixels.end(); it++){
        pixelsIn3DCoord.push_back((float)it->x);
        pixelsIn3DCoord.push_back((float)it->y);
        pixelsIn3DCoord.push_back(.0f); // add z coord
    }
    //CoordinatesChanged = false;

    return pixelsIn3DCoord;
}

std::vector<float> GenerateTriangle(int x_1, int y_1, int x_2, int y_2, int x_3, int y_3)
{
    std::vector<float> trianglePos = {float(x_1), float(y_1), -.1f,
                                      float(x_2), float(y_2), -.1f,
                                      float(x_3), float(y_3), -.1f};
    return trianglePos;
}

void drawObjects(){
    // draw grid
    shaderProgramLines.use();
    float scale = 1.9f / float (max_x - min_x);
    shaderProgramLines.setFloat("Scale",  scale);
    float pointsize = SCR_WIDTH * scale * 0.5f;
    shaderProgramLines.setFloat("PointSize", pointsize);
    shaderProgramLines.setVec3("Color", 0.0f, 0.0f, 1.0f);
    gridSO.drawSceneObject(GL_LINES);

    // draw triangle wireframe
    if(triangleSO.shouldUpdate){
        std::vector<float> trianglePositions = GenerateTriangle(x_1, y_1, x_2, y_2, x_3, y_3);
        modifyArrayBuffer(trianglePositions, triangleSO.VBO);
        triangleSO.vertexCount = trianglePositions.size()/3;
        triangleSO.shouldUpdate = false;
    }
    shaderProgramLines.setVec3("Color", 1.0f, 1.0f, 1.0f);
    triangleSO.drawSceneObject(GL_LINE_LOOP);


    // draw points
    if (trianglePointsSO.shouldUpdate) {
        std::vector<float> trianglePointsPositions = GenerateTrianglePixels(x_1, y_1, x_2, y_2, x_3, y_3);
        modifyArrayBuffer(trianglePointsPositions, trianglePointsSO.VBO);
        trianglePointsSO.vertexCount = trianglePointsPositions.size() / 3;
        trianglePointsSO.shouldUpdate = false;
    }
    shaderProgramDots.use();
    shaderProgramDots.setFloat("Scale", scale);
    shaderProgramDots.setFloat("PointSize", pointsize);
    shaderProgramDots.setVec3("Color", 0.0f, 0.0f, 0.0f);
    trianglePointsSO.drawSceneObject(GL_POINTS);
}


void setup(){
    shaderProgramLines.initialize("vertexscale.vert", "linefragment.frag");
    shaderProgramDots.initialize("vertexscale.vert", "dotfragment.frag");

    std::vector<float> linesPos;
    for (float i = min_x; i <= max_x; i += 1.0f) {
        linesPos.push_back(min_x);
        linesPos.push_back(i);
        linesPos.push_back(-.05f);
        linesPos.push_back(max_x);
        linesPos.push_back(i);
        linesPos.push_back(-.05f);

        linesPos.push_back(i);
        linesPos.push_back(min_y);
        linesPos.push_back(-.05f);
        linesPos.push_back(i);
        linesPos.push_back(max_y);
        linesPos.push_back(-.05f);
    }
    gridSO.VBO = createArrayBuffer(linesPos);
    gridSO.VAO = createVertexArray(shaderProgramLines, gridSO.VBO);
    gridSO.vertexCount = linesPos.size()/3;

    std::vector<float> trianglePos = GenerateTriangle(x_1, y_1, x_2, y_2, x_3, y_3);
    triangleSO.VBO = createArrayBuffer(trianglePos);
    triangleSO.VAO = createVertexArray(shaderProgramLines, triangleSO.VBO);
    triangleSO.vertexCount = trianglePos.size()/3;

    std::vector<float> pointsPos = GenerateTrianglePixels(x_1, y_1, x_2, y_2, x_3, y_3);
    trianglePointsSO.VBO = createArrayBuffer(pointsPos);
    trianglePointsSO.VAO = createVertexArray(shaderProgramDots, trianglePointsSO.VBO);
    trianglePointsSO.vertexCount = pointsPos.size() / 3;

}


unsigned int createVertexArray(const Shader &shader, unsigned int VBO){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // set vertex shader attribute "VertexPosition"
    int posAttributeLocation = glGetAttribLocation(shader.ID, "VertexPosition");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    return VAO;
}


unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    if (array.size() != 0)
        glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);
    return VBO;
}


void modifyArrayBuffer(const std::vector<float> &array, unsigned int VBO){
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    if (array.size() != 0)
        glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);
}

void key_input_callback(GLFWwindow* window, int button, int other,int action, int mods){
    if (button == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (button == GLFW_KEY_A && action == GLFW_PRESS)
        x_1 -= 1;
    if (button == GLFW_KEY_D && action == GLFW_PRESS)
        x_1 += 1;
    if (button == GLFW_KEY_S && action == GLFW_PRESS)
        y_1 -= 1;
    if (button == GLFW_KEY_W && action == GLFW_PRESS)
        y_1 += 1;

    if (button == GLFW_KEY_F && action == GLFW_PRESS)
        x_2 -= 1;
    if (button == GLFW_KEY_H && action == GLFW_PRESS)
        x_2 += 1;
    if (button == GLFW_KEY_G && action == GLFW_PRESS)
        y_2 -= 1;
    if (button == GLFW_KEY_T && action == GLFW_PRESS)
        y_2 += 1;

    if (button == GLFW_KEY_J && action == GLFW_PRESS)
        x_3 -= 1;
    if (button == GLFW_KEY_L && action == GLFW_PRESS)
        x_3 += 1;
    if (button == GLFW_KEY_K && action == GLFW_PRESS)
        y_3 -= 1;
    if (button == GLFW_KEY_I && action == GLFW_PRESS)
        y_3 += 1;

    x_1 = x_1 > max_x ? max_x : (x_1 < min_x ? min_x : x_1);
    y_1 = y_1 > max_x ? max_x : (y_1 < min_x ? min_x : y_1);
    x_2 = x_2 > max_x ? max_x : (x_2 < min_x ? min_x : x_2);
    y_2 = y_2 > max_x ? max_x : (y_2 < min_x ? min_x : y_2);
    x_3 = x_3 > max_x ? max_x : (x_3 < min_x ? min_x : x_3);
    y_3 = y_3 > max_x ? max_x : (y_3 < min_x ? min_x : y_3);

    triangleSO.shouldUpdate = true;
    trianglePointsSO.shouldUpdate = true;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}