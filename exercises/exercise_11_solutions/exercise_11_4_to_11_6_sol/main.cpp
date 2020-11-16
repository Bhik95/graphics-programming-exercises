#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_access.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// function declarations
// ---------------------
void renderScene(GLFWwindow* window);
void drawScene(Shader *shader, bool isShadowPass = false);
void drawGui();
void drawQuad();
void drawCube();

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
Model* carPaint;
Model* carBody;
Model* carInterior;
Model* carLight;
Model* carWindow;
Model* carWheel;
Model* floorModel;

unsigned int gBuffer;
unsigned int gPosition, gNormal, gAlbedoSpec;
Shader* shaderGeometryPass;
Shader* shaderLightingPass;
Shader* shaderLightBox;
// version of the shader using a forward pass, only for the sake of comparison
Shader* shaderForwardShading;

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
    // shading model
    bool usingDeferredShading = true;


    // light attributes
    bool lightsAreOn = true;
    float lightIntensity = 1.0f;
    float normalMappingMix = 1.0f;
    float attenuationConstant = 1.0f;
    float attenuationLinear = 0.5f;
    float attenuationQuadratic = 1.0f;
    float specularOffset = 0.5f;
    float rotation = 0.0f;

    // post processing
    bool sharpen = true;
    bool edgeDetection = true;

    // lights
    const unsigned int NR_LIGHTS = 128;
    std::vector<glm::vec3> lightPositions;
    std::vector<glm::vec3> lightColors;

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
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Exercise 11 - Deferred Shading", NULL, NULL);
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
	carPaint = new Model("car/Paint_LOD0.obj");
	carBody = new Model("car/Body_LOD0.obj");
	carLight = new Model("car/Light_LOD0.obj");
	carInterior = new Model("car/Interior_LOD0.obj");
	carWindow = new Model("car/Windows_LOD0.obj");
	carWheel = new Model("car/Wheel_LOD0.obj");
	floorModel = new Model("floor/floor.obj");

    shaderGeometryPass = new Shader("shaders/g_buffer.vert", "shaders/g_buffer.frag");
    shaderLightingPass = new Shader("shaders/deferred_shading.vert", "shaders/deferred_shading.frag");
    shaderLightBox = new Shader("shaders/deferred_light_box.vert", "shaders/deferred_light_box.frag");
    shaderForwardShading = new Shader("shaders/forward_shading.vert", "shaders/forward_shading.frag");

    // configure g-buffer framebuffer
    // ------------------------------
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);

    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // lighting info
    // -------------
    srand(13);
    for (unsigned int i = 0; i < config.NR_LIGHTS; i++)
    {
        // calculate slightly random offsets
        float xPos = ((rand() % 100) / 100.0) * 8.0 - 4.0;
        float yPos = ((rand() % 100) / 100.0) * 3.0;
        float zPos = ((rand() % 100) / 100.0) * 8.0 - 4.0;
        config.lightPositions.push_back(glm::vec3(xPos, yPos, zPos));
        // also calculate random color
        float rColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float gColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        float bColor = ((rand() % 100) / 200.0f) + 0.5; // between 0.5 and 1.0
        config.lightColors.push_back(glm::vec3(rColor, gColor, bColor));
    }

    // shader configuration
    // --------------------
    shaderLightingPass->use();
    shaderLightingPass->setInt("gPosition", 0);
    shaderLightingPass->setInt("gNormal", 1);
    shaderLightingPass->setInt("gAlbedoSpec", 2);


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
    delete shaderGeometryPass;
    delete shaderLightingPass;
    delete shaderLightBox;
    delete shaderForwardShading;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}




void renderScene(GLFWwindow* window){

    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 lightRotation(1.0f);
    lightRotation = glm::mat3(glm::rotate(lightRotation, glm::radians(config.rotation), glm::vec3(0.0f , 1.0f, 0.0f)));
    glm::mat3 lightRotationM3 = glm::mat3(lightRotation);

    if(!config.usingDeferredShading) {

        // FORWARD SHADING (how we have been rendering so far)
        // ---------------------------------------------------
        // 1. geometry and lighting pass: this is the only pass in forward shading
        // ----------------------------------------------------------------------

        shaderForwardShading->use();

        // send light relevant uniforms
        for (unsigned int i = 0; i < config.lightPositions.size(); i++) {
            shaderForwardShading->setVec3("lights[" + std::to_string(i) + "].Position",
                                         lightRotationM3 * config.lightPositions[i]);
            shaderForwardShading->setVec3("lights[" + std::to_string(i) + "].Color", config.lightColors[i]);
            // update attenuation parameters and calculate radius
            // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
            shaderForwardShading->setFloat("lights[" + std::to_string(i) + "].Constant", config.attenuationConstant);
            shaderForwardShading->setFloat("lights[" + std::to_string(i) + "].Linear", config.attenuationLinear);
            shaderForwardShading->setFloat("lights[" + std::to_string(i) + "].Quadratic",
                                          config.attenuationQuadratic);
        }
        shaderForwardShading->setBool("lightsAreOn", config.lightsAreOn);
        shaderForwardShading->setVec3("viewPos", camera.Position);
        shaderForwardShading->setFloat("specularOffset", config.specularOffset);
        shaderForwardShading->setFloat("lightIntensity", config.lightIntensity);

        shaderForwardShading->setMat4("projection", projection);
        shaderForwardShading->setMat4("view", view);
        shaderForwardShading->setFloat("normalMappingMix", config.normalMappingMix);

        // render car
        drawScene(shaderForwardShading, false);
    }
    else {

        // DEFERRED SHADING
        // ----------------
        // 1. geometry pass: render scene's geometry/color data into gbuffer
        // -----------------------------------------------------------------

        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaderGeometryPass->use();
        shaderGeometryPass->setMat4("projection", projection);
        shaderGeometryPass->setMat4("view", view);
        shaderGeometryPass->setFloat("normalMappingMix", config.normalMappingMix);

        drawScene(shaderGeometryPass, false);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // 2. lighting pass: calculate lighting by iterating over a screen filled quad pixel-by-pixel using the gbuffer's content.
        // -----------------------------------------------------------------------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaderLightingPass->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
//        glActiveTexture(GL_TEXTURE0);
//        glBindTexture(GL_TEXTURE_2D, gPosition);

        // send light relevant uniforms
        for (unsigned int i = 0; i < config.lightPositions.size(); i++) {
            shaderLightingPass->setVec3("lights[" + std::to_string(i) + "].Position",
                                       lightRotationM3 * config.lightPositions[i]);
            shaderLightingPass->setVec3("lights[" + std::to_string(i) + "].Color", config.lightColors[i]);
            // update attenuation parameters and calculate radius
            // note that we don't send this to the shader, we assume it is always 1.0 (in our case)
            shaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Constant", config.attenuationConstant);
            shaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Linear", config.attenuationLinear);
            shaderLightingPass->setFloat("lights[" + std::to_string(i) + "].Quadratic", config.attenuationQuadratic);
        }
        shaderLightingPass->setBool("lightsAreOn", config.lightsAreOn);
        shaderLightingPass->setBool("sharpen", config.sharpen);
        shaderLightingPass->setBool("edgeDetection", config.edgeDetection);
        shaderLightingPass->setVec3("viewPos", camera.Position);
        shaderLightingPass->setFloat("specularOffset", config.specularOffset);
        shaderLightingPass->setFloat("lightIntensity", config.lightIntensity);

        // finally render quad
        drawQuad();

        // 2.5. copy content of geometry's depth buffer to default framebuffer's depth buffer
        // ----------------------------------------------------------------------------------
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
        // blit to default framebuffer. Note that this may or may not work as the internal formats of both the FBO and default framebuffer have to match.
        // the internal formats are implementation defined. This works on all of my systems, but if it doesn't on yours you'll likely have to write to the
        // depth buffer in another shader stage (or somehow see to match the default framebuffer's internal format with the FBO's internal format).
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_DEPTH_BUFFER_BIT,
                          GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // optional step
    // draw debug boxes to show where the lights are located in te scene
    // -----------------------------------------------------------------
    shaderLightBox->use();
    shaderLightBox->setMat4("projection", projection);
    shaderLightBox->setMat4("view", view);
    for (unsigned int i = 0; i < config.lightPositions.size(); i++)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, config.lightPositions[i]);
        model = glm::scale(model, glm::vec3(0.05f));
        shaderLightBox->setMat4("model", lightRotation * model);
        shaderLightBox->setVec3("lightColor", config.lightColors[i]);
        drawCube();
    }
}

// drawQuad() renders a 2x2 XY quad in NDC
// ---------------------------------------
void drawQuad()
{
    static unsigned int quadVAO = 0, quadVBO;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
                -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
                1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
                1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// drawCube() renders a 1x1 3D cube in NDC.
// ----------------------------------------
void drawCube()
{
    static unsigned int cubeVAO = 0, cubeVBO = 0;
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// draw dear imGUI
void drawGui(){
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    {
        ImGui::Begin("Settings");

        ImGui::Text("Shading mode: ");
        if(ImGui::RadioButton("Forward", !config.usingDeferredShading)) {config.usingDeferredShading = false;} ImGui::SameLine();
        if(ImGui::RadioButton("Deferred", config.usingDeferredShading)) {config.usingDeferredShading = true;}
        ImGui::Separator();

        ImGui::Text("Light: ");
        if(ImGui::RadioButton("ON", config.lightsAreOn)) {config.lightsAreOn = true;} ImGui::SameLine();
        if(ImGui::RadioButton("OFF", !config.lightsAreOn)) {config.lightsAreOn = false;}
        ImGui::SliderFloat("light intensity", &config.lightIntensity, 0.0f, 2.0f);
        ImGui::SliderFloat("normal mapping mix", &config.normalMappingMix, 0.0f, 1.0f);
        ImGui::SliderFloat("constant attenuation", &config.attenuationConstant, 0.0f, 5.0f);
        ImGui::SliderFloat("linear attenuation", &config.attenuationLinear, 0.0f, 5.0f);
        ImGui::SliderFloat("quadratic attenuation", &config.attenuationQuadratic, 0.0f, 5.0f);
        ImGui::SliderFloat("specular offset", &config.specularOffset, 0.0f, 1.0f);
        ImGui::SliderFloat("rotation", &config.rotation, 0.0f, 360.0f);
        ImGui::Separator();

        ImGui::Text("Post processing: ");
        ImGui::Checkbox("Sharpen", &config.sharpen);
        ImGui::Checkbox("Edge detection", &config.edgeDetection);
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

//    // we don't draw the transparent objects to the shadow map so that they don't cast shadows
//    glEnable(GL_BLEND);
//    carWindow->Draw(*sceneShader);
//    glDisable(GL_BLEND);
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