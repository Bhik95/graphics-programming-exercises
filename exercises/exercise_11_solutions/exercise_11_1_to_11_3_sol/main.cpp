#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_access.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "skybox.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// function declarations
// ---------------------
void renderScene(GLFWwindow* window);
void drawScene(Shader *shader, bool isShadowPass = false);
void drawGui();
void drawSkybox();

// glfw and input functions
// ------------------------
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_input_callback(GLFWwindow* window, int button, int other, int action, int mods);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 1280, SCR_HEIGHT = 720;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

// global variables used for rendering
// -----------------------------------
Shader* sceneShader;
Model* carPaint;
Model* carBody;
Model* carInterior;
Model* carLight;
Model* carWindow;
Model* carWheel;
Model* floorModel;

Shader* skyboxShader;
unsigned int skyboxVAO;
unsigned int cubemapTexture;

Shader* simpleDepthShader;

Camera camera(glm::vec3(0.0f, 1.6f, 5.0f));

unsigned int depthMap, depthMapFBO;

// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;
bool isPaused = false; // used to stop camera movement when GUI is open

// parameters that can be set in our GUI
// -------------------------------------
struct Config {
    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 1.0f, 1.0f};
    float ambientLightIntensity = 0.25f;

    // light 1
    glm::vec3 lightPosition = {-3.0f, 3.0f, -1.0f};
    glm::vec3 lightDirection = {2.7f, -1.0f, 0.7f};
    glm::vec3 lightColor = {0.85f, 0.8f, 0.6f};
    float lightIntensity = 0.75f;

    // material
    float specularExponent = 27.0f;
    float ambientOcclusionMix = 1.0f;
    float normalMappingMix = 1.0f;
    float reflectionMix = 0.15f;
    int orthoTangentSpace = 0;

    // shadow parameters
    bool softShadows = false;
    float shadowBias = 0.07f;
    float shadowMapSize = 5.0f;
    float shadowMapDepthRange = 20.0f;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 11 - Shadow mapping", NULL, NULL);
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
    sceneShader = new Shader("shaders/shader.vert", "shaders/shader.frag");
	carPaint = new Model("car/Paint_LOD0.obj");
	carBody = new Model("car/Body_LOD0.obj");
	carLight = new Model("car/Light_LOD0.obj");
	carInterior = new Model("car/Interior_LOD0.obj");
	carWindow = new Model("car/Windows_LOD0.obj");
	carWheel = new Model("car/Wheel_LOD0.obj");
	floorModel = new Model("floor/floor.obj");
    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
    simpleDepthShader = new Shader("shaders/shadowmapping_depth.vert", "shaders/shadowmapping_depth.frag");

    // init skybox
    vector<std::string> faces {
                    "skybox/right.tga",
                    "skybox/left.tga",
                    "skybox/top.tga",
                    "skybox/bottom.tga",
                    "skybox/front.tga",
                    "skybox/back.tga"
            };
    cubemapTexture = Skybox::loadCubemap(faces);
    skyboxVAO = Skybox::initSkyboxBuffers();


    // configure depth map FBO
    // -----------------------

    // create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // set up the z-buffer
    glDepthRange(0,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC


    // IMGUI init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);



    // render loop
    while (!glfwWindowShouldClose(window))
    {
        static float lastFrame = 0.0f;
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // clear buffers
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        renderScene(window);


        // render GUI (if paused)
        // --------------------------------------------------------------
        if (isPaused) {
			drawGui();
		}

        // show the frame buffer
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	//delete carModel;
	delete floorModel;
	delete carWindow;
	delete carPaint;
	delete carInterior;
	delete carLight;
	delete carBody;
    delete carWheel;
    delete sceneShader;
    delete skyboxShader;
    delete simpleDepthShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}




void renderScene(GLFWwindow* window){
    // render depth of scene to texture from the light's perspective
    // -------------------------------------------------------------

    // We use an ortographic projection since it is a directional light.
    // left, right, bottom, top, near and far values define the 3D volume relative to
    // the light position and direction that will be rendered to produce the depth texture.
    // Geometry outside of this range will not be considered when computing shadows.
    float near_plane = 1.0f;
    float half = config.shadowMapSize/2.0f;
    glm::mat4 lightProjection = glm::ortho(-half, half, -half, half, near_plane, near_plane + config.shadowMapDepthRange);
    glm::mat4 lightView = glm::lookAt(config.lightPosition, config.lightPosition+config.lightDirection, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // setup depth shader
    simpleDepthShader->use();
    simpleDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // setup framebuffer size
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // bind our depth texture to the frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    // clear the depth texture/depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw scene from the light's perspective into the depth texture
    drawScene(simpleDepthShader, true);

    // unbind the depth texture from the frame buffer, now we can render to the screen (frame buffer) again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // render the scene and use the depth from the light's perspective to compute shadows
    // --------------------------------------------------------------

    // reset the render window size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    // clear the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the skybox
    drawSkybox();

    // setup scene shader
    sceneShader->use();

    // shadow uniforms
    sceneShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    sceneShader->setInt("shadowMap", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    sceneShader->setFloat("shadowBias", config.shadowBias * 0.01f);
    sceneShader->setBool("softShadows", config.softShadows);

    // light uniforms
    sceneShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    sceneShader->setVec3("lightDirection", config.lightDirection);
    sceneShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    sceneShader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    sceneShader->setFloat("normalMappingMix", config.normalMappingMix);
    sceneShader->setFloat("reflectionMix", config.reflectionMix);
    sceneShader->setFloat("specularExponent", config.specularExponent);

    // set up skybox texture uniform
    sceneShader->setInt("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // draw the scene
    drawScene(sceneShader, false);
}

// draw dear imGUI
void drawGui(){
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Settings");

        ImGui::Text("Ambient light: ");
        ImGui::ColorEdit3("ambient light color", (float*)&config.ambientLightColor);
        ImGui::SliderFloat("ambient light intensity", &config.ambientLightIntensity, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Light 1: ");
        ImGui::DragFloat3("light 1 position", (float*)&config.lightPosition, .1, -20, 20);
        ImGui::DragFloat3("light 1 direction", (float*)&config.lightDirection, .1, -20, 20);
        ImGui::ColorEdit3("light 1 color", (float*)&config.lightColor);
        ImGui::SliderFloat("light 1 intensity", &config.lightIntensity, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Material: ");
        ImGui::SliderFloat("ambient occlusion mix", &config.ambientOcclusionMix, 0.0f, 1.0f);
        ImGui::SliderFloat("normal mapping mix", &config.normalMappingMix, 0.0f, 1.0f);
        ImGui::SliderFloat("reflection mix", &config.reflectionMix, 0.0f, 1.0f);
        ImGui::SliderFloat("specular exponent", &config.specularExponent, 0.0f, 150.0f);
        ImGui::Separator();

        ImGui::Text("Shadow map: ");

        ImGui::Text("Render soft shadows: ");
        if(ImGui::RadioButton("YES", config.softShadows == 1)) {config.softShadows = 1;} ImGui::SameLine();
        if(ImGui::RadioButton("NO", config.softShadows == 0)) {config.softShadows = 0;}
        ImGui::SliderFloat("bias x 100", &config.shadowBias, 0.0f, 1.0f);
        ImGui::SliderFloat("size", &config.shadowMapSize, 1.0f, 20.0f);
        ImGui::SliderFloat("depth range", &config.shadowMapDepthRange, 1.0f, 100.0f);
        ImGui::Separator();


        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// draw the scene geometry
void drawScene(Shader *shader, bool isShadowPass){

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);
    shader->setVec3("viewPosition", camera.Position);
    shader->setMat4("view", view);

    // draw floor,
    // notice that we overwrite the value of one of the uniform variables to set a different floor color
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(1.f, 1.f, 1.f));
    shader->setMat4("model", model);
    shader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));
    shader->setMat4("view", view);
    floorModel->Draw(*shader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, 1.39));
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
    carInterior->Draw(*shader);
    carPaint->Draw(*shader);
    carLight->Draw(*shader);

    if(isShadowPass)
        return;

    // we don't draw the transparent objects to the shadow map so that they don't cast shadows
    glEnable(GL_BLEND);
    carWindow->Draw(*sceneShader);
    glDisable(GL_BLEND);
}

void drawSkybox(){
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

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