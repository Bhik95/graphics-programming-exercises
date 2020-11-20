#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <glm/gtc/matrix_access.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// function declarations
// ---------------------
unsigned int initSkyboxBuffers();
unsigned int loadCubemap(vector<std::string> faces);
void drawScene();
void drawGui();

// glfw and input functions
// ------------------------
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// global variables used for rendering
// -----------------------------------
Shader* shader;
Model* carPaint;
Model* carBody;
Model* carWindow;
Model* carWheel;

Shader* skyboxShader;
unsigned int skyboxVAO; // skybox handle
unsigned int cubemapTexture; // skybox texture handle

Camera camera(glm::vec3(0.0f, 1.6f, 5.0f));

// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;
bool isPaused = false; // used to stop camera movement when GUI is open

// parameters that can be set in our GUI
// -------------------------------------
struct Config {
    float reflectRefractMix = 0;
} config;



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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 11", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
    glfwSetKeyCallback(window, key_input_callback);
	glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // init shaders and models
    shader = new Shader("shaders/shader.vert", "shaders/shader.frag");
	carPaint = new Model("car/Paint_LOD0.obj");
	carBody = new Model("car/Body_LOD0.obj");
	carWindow = new Model("car/Windows_LOD0.obj");
	carWheel = new Model("car/Wheel_LOD0.obj");
    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");

    // init skybox
    vector<std::string> faces
            {
                    "skybox/right.tga",
                    "skybox/left.tga",
                    "skybox/top.tga",
                    "skybox/bottom.tga",
                    "skybox/front.tga",
                    "skybox/back.tga"
            };
    cubemapTexture = loadCubemap(faces);
    skyboxVAO = initSkyboxBuffers();


    // opengl settings
    glDepthRange(-1,1);
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE); // enable backface culling


    // IMGUI init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


    // render loop
    while (!glfwWindowShouldClose(window))
    {
        static float lastFrame = 0.0f;
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        drawScene();

		if (isPaused) {
			drawGui();
		}


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	delete carWindow;
	delete carPaint;
	delete carBody;
    delete carWheel;
    delete shader;
    delete skyboxShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// init the VAO of the skybox
// --------------------------
unsigned int initSkyboxBuffers(){
    // triangles forming the six faces of a cube
    // note that the camera is placed inside of the cube, so the winding order
    // is selected to make the triangles visible from the inside
    float skyboxVertices[108]  {
            // x, y and z coordinates
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    return skyboxVAO;
}


// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front)
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


void drawGui(){
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Settings");

        ImGui::Text("Shader option: ");
        ImGui::SliderFloat("light 1 intensity", &config.reflectRefractMix, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}



void drawScene(){
    shader->use();

    shader->setFloat("reflectRefractMix", config.reflectRefractMix);

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);
    shader->setVec3("cameraPos", camera.Position);
    shader->setMat4("view", view);

    // set up skybox texture
    shader->setInt("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);


    // draw wheel
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, 1.39));
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    carWheel->Draw(*shader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, -1.296));
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    carWheel->Draw(*shader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, 1.296));
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    carWheel->Draw(*shader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, -1.39));
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    carWheel->Draw(*shader);

    // draw the rest of the car
    model = glm::mat4(1.0f);
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    carBody->Draw(*shader);
    carPaint->Draw(*shader);
    carWindow->Draw(*shader);

    // draw skybox as last
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    skyboxShader->setMat4("view", view);
    skyboxShader->setInt("skybox", 0);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}


void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

	if (isPaused)
		return;

	// movement commands
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

}


void cursor_input_callback(GLFWwindow* window, double posX, double posY){

	// camera rotation
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = posX;
        lastY = posY;
        firstMouse = false;
    }

    float xoffset = posX - lastX;
    float yoffset = lastY - posY; // reversed since y-coordinates go from bottom to top

    lastX = posX;
    lastY = posY;

	if (isPaused)
		return;

    camera.ProcessMouseMovement(xoffset, yoffset);
}


void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods){

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        isPaused = !isPaused;
        glfwSetInputMode(window, GLFW_CURSOR, isPaused ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}