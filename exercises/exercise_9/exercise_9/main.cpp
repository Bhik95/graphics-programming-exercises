#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// function declarations
// ---------------------
void loadFloorTexture();
void drawCar();
void drawFloor();
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
Shader* carShader;
Shader* floorShader;
Model* carPaint;
Model* carBody;
Model* carInterior;
Model* carLight;
Model* carWindow;
Model* carWheel;
Model* floorModel;
unsigned int floorTextureId;
Camera camera(glm::vec3(0.0f, 1.6f, 5.0f));

// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;
bool isPaused = false; // stop camera movement when GUI is open

// parameters that can be set in our GUI
// -------------------------------------
struct Config {

    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 1.0f, 1.0f};
    float ambientLightIntensity = 0.25f;

    // light 1
    glm::vec3 lightPosition = {1.2f, 2.5f, 0.8f};
    glm::vec3 lightColor = {1.0f, 1.0f, 1.0f};
    float lightIntensity = 0.75f;

    // material
    float specularExponent = 80.0f;
    float ambientOcclusionMix = 1.0f;

    // attenuation (c0, c1 and c2 on the slides)
    float attenuationC0 = 0.25;
    float attenuationC1 = 0.1;
    float attenuationC2 = 0.1;

    // TODO exercise 9.2 scale config variable


    // floor texture mode
    unsigned int wrapSetting = GL_REPEAT;
    unsigned int minFilterSetting = GL_LINEAR_MIPMAP_LINEAR;
    unsigned int magFilterSetting = GL_LINEAR;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 9", NULL, NULL);
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


    // ---------
    // floor texture
    glGenTextures(1, &floorTextureId);
    loadFloorTexture();


    carShader = new Shader("shaders/car_shader.vert", "shaders/car_shader.frag");
    floorShader = new Shader("shaders/floor_Shader.vert", "shaders/floor_Shader.frag");
	carPaint = new Model("car/Paint_LOD0.obj");
	carBody = new Model("car/Body_LOD0.obj");
	carLight = new Model("car/Light_LOD0.obj");
	carInterior = new Model("car/Interior_LOD0.obj");
	carWindow = new Model("car/Windows_LOD0.obj");
	carWheel = new Model("car/Wheel_LOD0.obj");
	floorModel = new Model("floor/floor_no_material.obj");

    // set up the z-buffer
    glDepthRange(-1,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
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


        drawFloor();
        drawCar();
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

	//delete carModel;
	delete floorModel;
	delete carWindow;
	delete carPaint;
	delete carInterior;
	delete carLight;
	delete carBody;
    delete carWheel;
    delete floorShader;
    delete carShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// ------------------------------------
// TODO EXERCISE 9.1 LOAD FLOOR TEXTURE
// ------------------------------------

void loadFloorTexture(){
    // TODO this is mostly a copy and paste of the function 'TextureFromFile' in the 'model.h' file
    //  however, you should use the min/mag/wrap settings that you can control in the user interface
    //  and load the texture 'floor/checkboard_texture.png'

}

// --------------
// DRAW FUNCTIONS
// --------------

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
        ImGui::ColorEdit3("light 1 color", (float*)&config.lightColor);
        ImGui::SliderFloat("light 1 intensity", &config.lightIntensity, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Material: ");
        ImGui::SliderFloat("ambient occlusion mix", &config.ambientOcclusionMix, 0.0f, 1.0f);
        ImGui::SliderFloat("specular exponent", &config.specularExponent, 0.1f, 300.0f);
        ImGui::Separator();

        ImGui::Text("Attenuation: ");
        ImGui::SliderFloat("attenuation c0", &config.attenuationC0, 0.0f, 1.0f);
        ImGui::SliderFloat("attenuation c1", &config.attenuationC1, 0.0f, 1.0f);
        ImGui::SliderFloat("attenuation c2", &config.attenuationC2, 0.0f, 1.0f);
        ImGui::Separator();

        ImGui::Text("Wrap setting: ");
        if(ImGui::RadioButton("CLAMP_TO_EDGE", config.wrapSetting == GL_CLAMP_TO_EDGE)) {config.wrapSetting = GL_CLAMP_TO_EDGE;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("CLAMP_TO_BORDER", config.wrapSetting == GL_CLAMP_TO_BORDER)) {config.wrapSetting = GL_CLAMP_TO_BORDER;loadFloorTexture();}
        if(ImGui::RadioButton("MIRRORED_REPEAT", config.wrapSetting == GL_MIRRORED_REPEAT)) {config.wrapSetting = GL_MIRRORED_REPEAT;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("REPEAT", config.wrapSetting == GL_REPEAT)) {config.wrapSetting = GL_REPEAT;loadFloorTexture();};
        ImGui::Separator();

        ImGui::Text("Minifying setting: ");
        if(ImGui::RadioButton("NEAREST", config.minFilterSetting == GL_NEAREST )) {config.minFilterSetting = GL_NEAREST ;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR", config.minFilterSetting == GL_LINEAR )) {config.minFilterSetting = GL_LINEAR ;loadFloorTexture();}
        if(ImGui::RadioButton("NEAREST_MIPMAP_NEAREST", config.minFilterSetting == GL_NEAREST_MIPMAP_NEAREST )) {config.minFilterSetting = GL_NEAREST_MIPMAP_NEAREST ;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR_MIPMAP_NEAREST", config.minFilterSetting == GL_LINEAR_MIPMAP_NEAREST )) {config.minFilterSetting = GL_LINEAR_MIPMAP_NEAREST ;loadFloorTexture();}
        if(ImGui::RadioButton("NEAREST_MIPMAP_LINEAR", config.minFilterSetting == GL_NEAREST_MIPMAP_LINEAR )) {config.minFilterSetting = GL_NEAREST_MIPMAP_LINEAR ;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR_MIPMAP_LINEAR", config.minFilterSetting == GL_LINEAR_MIPMAP_LINEAR )) {config.minFilterSetting = GL_LINEAR_MIPMAP_LINEAR ;loadFloorTexture();}
        ImGui::Separator();

        ImGui::Text("Magnifying setting: ");
        if(ImGui::RadioButton("NEAREST##2", config.magFilterSetting == GL_NEAREST )) {config.magFilterSetting = GL_NEAREST ;loadFloorTexture();} ImGui::SameLine();
        if(ImGui::RadioButton("LINEAR##2", config.magFilterSetting == GL_LINEAR )) {config.magFilterSetting = GL_LINEAR ;loadFloorTexture();}
        ImGui::Separator();

        // TODO exercise 9.2 add slider to control uvScale

        ImGui::Separator();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void drawFloor(){
    floorShader->use();
    // light uniforms
    floorShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    floorShader->setVec3("lightPosition", config.lightPosition);
    floorShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    floorShader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    floorShader->setFloat("attenuationC0", config.attenuationC0);
    floorShader->setFloat("attenuationC1", config.attenuationC1);
    floorShader->setFloat("attenuationC2", config.attenuationC2);

    // TODO exercise 9.2 send uvScale to the shader as a uniform variable



    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    floorShader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    floorShader->setInt("texture_diffuse1", 0);
    glBindTexture(GL_TEXTURE_2D, floorTextureId);
    // draw floor,
    // notice that we overwrite the value of one of the uniform variables to set a different floor color
    floorShader->setVec3("reflectionColor", .2, .5, .2);
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(5.f, 5.f, 5.f));
    floorShader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    floorShader->setMat4("invTranspMV", invTranspose);
    floorShader->setMat4("view", view);
    floorModel->Draw(*floorShader);
}


void drawCar(){
    carShader->use();
    // light uniforms
    carShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    carShader->setVec3("lightPosition", config.lightPosition);
    carShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    carShader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    carShader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    carShader->setFloat("attenuationC0", config.attenuationC0);
    carShader->setFloat("attenuationC1", config.attenuationC1);
    carShader->setFloat("attenuationC2", config.attenuationC2);


    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    carShader->setMat4("projection", projection);

    // draw wheel
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, 1.39));
    carShader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, -1.296));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, 1.296));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, -1.39));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw the rest of the car
    model = glm::mat4(1.0f);
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carBody->Draw(*carShader);
    carInterior->Draw(*carShader);
    carPaint->Draw(*carShader);
    carLight->Draw(*carShader);
    glEnable(GL_BLEND);
    carWindow->Draw(*carShader);
    glDisable(GL_BLEND);

}

// ---------------
// INPUT FUNCTIONS
// ---------------

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