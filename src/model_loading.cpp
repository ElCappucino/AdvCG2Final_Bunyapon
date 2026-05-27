#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader_m.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <SoftBody.h>
#include <SmokeSimulation.h>


#include <learnopengl/animator.h>
#include <learnopengl/model_animation.h>
#include <glm/gtx/string_cast.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadHDRTexture(const char* path);
void renderCube();
void renderQuad();
void UpdateImGui();

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool isAttackPending = false;
float attackTimer = 0.0f;
const float ATTACK_IMPACT_DELAY = 0.5f;

// Slime hit-flash effect tracking
bool isSlimeHit = false;
float slimeHitTimer = 0.0f;
const float SLIME_HIT_DURATION = 1.0f;

// movement
glm::vec3 charPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 charFront = glm::vec3(0.0f, 0.0f, -1.0f); // initial forward
glm::vec3 charFrontTarget = glm::vec3(0.0f, 0.0f, -1.0f); // initial forward

std::vector<glm::vec3> firePositions = {
    glm::vec3(5.5f,  0.9f, -1.0f),
    glm::vec3(-5.5f, 0.9f, -1.0f),
    glm::vec3(-5.5f,  0.9f, -8.0f),
    glm::vec3(5.5f,  0.9f, -8.0f)
};

// Player Shader
float playerReflectionIntensity = 0.03f;

bool hasMovementInput = false;
float charSpeed = 2.5f;
bool isMoving = false;
bool lastMouseDown = false;

bool movementLocked = false;

enum AnimState {
    IDLE = 1,
    IDLE_SLASH,
    SLASHING,
    SLASH_IDLE,
    IDLE_RUN,
    RUN_IDLE,
    RUN
};
enum AnimState charState = IDLE;

float cameraRadius = 10.0f;       
float orbitYaw = 0.0f;        
float orbitPitch = 20.0f;    
float smoothSpeed = 8.0f; 

// Depth of Field Parameters
float dofFocusDistance = 15.0f;
float dofFocusRange = 5.0f;
float dofMaxBlur = 0.002f;

float targetYaw = orbitYaw;
float targetPitch = orbitPitch;

unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

unsigned int quadVAO = 0;
unsigned int quadVBO = 0;

// Configure XPBD parameters
glm::vec3 spawnPosition(-25.0f, 2.0f, -20.0f);
glm::vec3 layoutScale(2.4f);
float edgeCompliance = 0.0001f; // 0.0f = perfectly rigid

// Slime Dash Parameter
float slimeTimer = 0.0f;
float slimeDashInterval = 2.0f;  // Time between dashes (seconds)
float slimeDashImpulse = 5.0f;  // Strength of the dash
float slimeLaunchPitch = 2.5f;   // Slight upward push 

SoftBody jellyCube;

SmokeSimulation smokeSim(50, 50, 50, 0.25f);
glm::vec3 smokeGridOrigin(-4.0f, 0.0f, -4.0f);
float smokeDotTimer = 0.0f;
const float SMOKE_DOT_INTERVAL = 0.15f;

struct SmokeParticleInstance {
    glm::vec3 position;
    float density;
    glm::vec3 scale;   
    float rotation;    
};

std::vector<SmokeParticleInstance> visibleSmokeCells;
bool triggerSmokeBurst = false;
bool isContinuousSmoke = false;
bool rKeyWasPressed = false;

float hash3D(int x, int y, int z) {
    return glm::fract(sin(dot(glm::vec3(x, y, z), glm::vec3(12.9898f, 78.233f, 137.180f))) * 43758.5453f);
}

int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    // tell GLFW to capture our mouse
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    Shader ourShader("1.model_loading.vs", "1.model_loading.fs");
    Shader equirectangularToCubemapShader("cubemap.vs", "equirectangular_to_cubemap.fs");
    Shader skyboxShader("skybox.vs", "skybox.fs");
    Shader animShader("anim_model.vs", "anim_model.fs");
    Shader fireShader("fire.vs", "fire.fs");
    Shader slimeShader("slime.vs", "slime.fs");
    Shader floorShader("floor.vs", "floor.fs");
    Shader simpleDepthShader("shadow_depth.vs", "shadow_depth.fs");
    Shader dofShader("dof.vs", "dof.fs");
    Shader smokeShader("smoke.vs", "smoke.fs");

    // load models
    Model ourModel(FileSystem::getPath("resources/objects/Pillar/pillar.obj"));
    Model pillarFloorModel(FileSystem::getPath("resources/objects/Pillar/pillarFloor.obj"));
    Model slimeModel(FileSystem::getPath("resources/objects/Slime/slime5.obj"));

    // Initialize Soft body Model
    jellyCube.InitializeFromModel(slimeModel, spawnPosition, layoutScale, edgeCompliance);

    Model playerModel(FileSystem::getPath("resources/objects/Skeletal/playerModel/playerTPose.dae"));
    Animation idleAnimation(FileSystem::getPath("resources/objects/Skeletal/playerIdle/playerIdle.dae"), &playerModel);
    Animation runAnimation(FileSystem::getPath("resources/objects/Skeletal/playerRun/playerRun.dae"), &playerModel);
    Animation slashAnimation(FileSystem::getPath("resources/objects/Skeletal/playerSlash/playerSlash.dae"), &playerModel);

    Animator animator(&idleAnimation);

    float blendAmount = 0.0f;
    float blendRate = 0.055f;

    unsigned int hdrTexture = loadHDRTexture("resources/textures/hdr/kiara_1_dawn_4k.hdr");

    // Cubemap Texture Frame Buffer(Skybox)
    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512); 
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // Initialize 6 empty floating-point color buffers (512x512)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Build 6 camera matrices pointing to each side of the cube
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    equirectangularToCubemapShader.use();
    equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // Shadow Map Texture Frame Buffer
    const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048; 
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);

    unsigned int depthMapTextureID;
    glGenTextures(1, &depthMapTextureID);
    glBindTexture(GL_TEXTURE_2D, depthMapTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTextureID, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Reset viewport to window dimensions for your real loop
    glViewport(0, 0, display_w, display_h);

    // Initialize Depth of Field Frame Buffer
    unsigned int dofFBO;
    glGenFramebuffers(1, &dofFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, dofFBO);

    // Depth of Field : Color Buffer Texture
    unsigned int dofColorTex;
    glGenTextures(1, &dofColorTex);
    glBindTexture(GL_TEXTURE_2D, dofColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, display_w, display_h, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dofColorTex, 0);

    // Depth of Field : Depth Texture Map 
    unsigned int dofDepthTex;
    glGenTextures(1, &dofDepthTex);
    glBindTexture(GL_TEXTURE_2D, dofDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, display_w, display_h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, dofDepthTex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: DoF Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Animation State Machine Handling
        switch (charState) {
            case IDLE:
                if (isMoving) {
                    animator.CrossFade(&runAnimation, 0.2f);
                    charState = IDLE_RUN;
                }
                break;
            case IDLE_RUN:
                if (!animator.IsBlending()) {
                    charState = RUN;
                }
                break;
            case RUN:
                if (!isMoving) {
                    animator.CrossFade(&idleAnimation, 0.2f);
                    charState = RUN_IDLE;
                }
                break;
            case RUN_IDLE:
                if (!animator.IsBlending()) {
                    charState = IDLE;
                }
                break;
            case IDLE_SLASH:
                animator.CrossFade(&slashAnimation, 0.2f);
                charState = SLASHING;
                break;
            case SLASHING:
                if (animator.AnimationFinished())
                {
                    if (isMoving)
                        animator.CrossFade(&runAnimation, 0.2f);
                    else
                        animator.CrossFade(&idleAnimation, 0.2f);

                    charState = SLASH_IDLE;
                }
                break;
            case SLASH_IDLE:
                if (!animator.IsBlending()) {
                    charState = isMoving ? RUN : IDLE;
                    movementLocked = false;
                }
                break;
        }

        // Attack with delay
        if (isAttackPending)
        {
            attackTimer += deltaTime;
            if (attackTimer >= ATTACK_IMPACT_DELAY)
            {
                if (!jellyCube.particles.empty())
                {
                    glm::vec3 slimeCenter(0.0f);
                    for (const auto& p : jellyCube.particles) {
                        slimeCenter += p.pos;
                    }
                    slimeCenter /= static_cast<float>(jellyCube.particles.size());

                    glm::vec3 toSlime = slimeCenter - charPosition;
                    float distanceToSlime = glm::length(toSlime);
                    float attackRange = 4.5f;

                    if (distanceToSlime <= attackRange)
                    {
                        bool isInsideSlime = (distanceToSlime < 2.0f);
                        bool isFacingSlime = false;

                        if (!isInsideSlime)
                        {
                            glm::vec3 attackDir = (glm::length(charFront) > 0.001f) ? glm::normalize(charFront) : glm::normalize(charFrontTarget);
                            glm::vec3 dirToSlime = glm::normalize(toSlime);

                            float dotProduct = glm::dot(attackDir, dirToSlime);
                            if (dotProduct > 0.707f)
                            {
                                isFacingSlime = true;
                            }
                        }
                        std::cout << "is inside slime = " << isInsideSlime << " distanceToSlime = " << distanceToSlime << std::endl;
                        if (isInsideSlime || isFacingSlime)
                        {
                            float playerAttackImpulse = 8.0f;
                            float upwardLift = 1.5f;

                            glm::vec3 pushDir = (glm::length(charFront) > 0.001f) ? glm::normalize(charFront) : glm::normalize(charFrontTarget);
                            glm::vec3 knockbackVector = pushDir * playerAttackImpulse + glm::vec3(0.0f, upwardLift, 0.0f);

                            for (auto& p : jellyCube.particles)
                            {
                                if (p.invMass > 0.0f) {
                                    p.vel += knockbackVector;
                                }
                            }
                            isSlimeHit = true;
                            slimeHitTimer = 0.0f;
                            std::cout << "Hit Connected! (Inside: " << isInsideSlime << ")" << std::endl;
                        }
                    }
                }
                isAttackPending = false;
                attackTimer = 0.0f;
            }
        }

        if (isSlimeHit)
        {
            slimeHitTimer += deltaTime;
            if (slimeHitTimer >= SLIME_HIT_DURATION)
            {
                isSlimeHit = false;
                slimeHitTimer = 0.0f;
            }
        }

        float t = 0.1f;
        charFront = glm::mix(charFront, charFrontTarget, t);

        animator.UpdateAnimation(deltaTime);

        if (!jellyCube.particles.empty())
        {
            slimeTimer += deltaTime;
            if (slimeTimer >= slimeDashInterval)
            {
                glm::vec3 slimeCenter(0.0f);
                for (const auto& p : jellyCube.particles) {
                    slimeCenter += p.pos;
                }
                slimeCenter /= static_cast<float>(jellyCube.particles.size());

                glm::vec3 toCharacter = charPosition - slimeCenter;
                toCharacter.y = 0.0f;

                float targetDistance = glm::length(toCharacter);
                float stopRadius = 2.0f;

                if (targetDistance > stopRadius)
                {
                    glm::vec3 dashDirection = glm::normalize(toCharacter);
                    glm::vec3 impulseVector = dashDirection * slimeDashImpulse + glm::vec3(0.0f, slimeLaunchPitch, 0.0f);

                    for (auto& p : jellyCube.particles)
                    {
                        if (p.invMass > 0.0f) {
                            p.vel += impulseVector;
                        }
                    }
                    std::cout << "Slime AI Dashed towards character!" << std::endl;
                }
                slimeTimer = 0.0f;
            }
        }

        float simDeltaTime = std::min(deltaTime, 0.033f);
        glm::vec3 globalGravity = glm::vec3(0.0f, -9.81f, 0.0f);

        jellyCube.Step(simDeltaTime, 3, globalGravity);

        glm::vec3 sceneCenterPos = glm::vec3(0.0f, 0.5f, 0.0f);

        if (triggerSmokeBurst || isContinuousSmoke)
        {
            // 1. SNAP THE GRID CONTAINER TO THE PLAYER
            // Center the simulation box horizontally (X and Z) around the player's current position.
            // We keep the Y grid origin resting on the floor (charPosition.y) or zero.
            float gridHalfWidth = (smokeSim.nx * smokeSim.dx) / 2.0f;
            float gridHalfDepth = (smokeSim.nz * smokeSim.dx) / 2.0f;

            smokeGridOrigin.x = charPosition.x - gridHalfWidth;
            smokeGridOrigin.y = charPosition.y; // Or keep at 0.0f if your floor is perfectly flat
            smokeGridOrigin.z = charPosition.z - gridHalfDepth;

            // 2. RESET THE FLUID GRID ARRAYS (Optional but highly recommended)
            // Teleporting a grid containing old moving smoke looks jarring. 
            // Clearing the old grid data ensures a completely fresh explosion at the new spot.
            std::fill(smokeSim.density.begin(), smokeSim.density.end(), 0.0f);
            // If your SmokeSimulation class tracks velocity arrays (e.g., u, v, w), reset them here too:
            // std::fill(smokeSim.u.begin(), smokeSim.u.end(), 0.0f); 
            // std::fill(smokeSim.v.begin(), smokeSim.v.end(), 0.0f);
            // std::fill(smokeSim.w.begin(), smokeSim.w.end(), 0.0f);
        }

        // Convert world positions to localized grid space indices
        /*int gridX = static_cast<int>((sceneCenterPos.x - smokeGridOrigin.x) / smokeSim.dx);
        int gridY = static_cast<int>((sceneCenterPos.y - smokeGridOrigin.y) / smokeSim.dx);
        int gridZ = static_cast<int>((sceneCenterPos.z - smokeGridOrigin.z) / smokeSim.dx);*/
        int gridX = static_cast<int>((charPosition.x - smokeGridOrigin.x) / smokeSim.dx);
        int gridY = static_cast<int>(((charPosition.y + 0.5f) - smokeGridOrigin.y) / smokeSim.dx);
        int gridZ = static_cast<int>((charPosition.z - smokeGridOrigin.z) / smokeSim.dx);

        if (isContinuousSmoke || triggerSmokeBurst)
        {
            // This boundary safety check will now always pass because we forced the player to the center!
            if (gridX > 4 && gridX < smokeSim.nx - 4 &&
                gridY > 0 && gridY < smokeSim.ny - 4 &&
                gridZ > 4 && gridZ < smokeSim.nz - 4)
            {
                int radius = 8;
                for (int offsetX = -radius; offsetX <= radius; ++offsetX) {
                    for (int offsetZ = -radius; offsetZ <= radius; ++offsetZ) {
                        if ((offsetX * offsetX + offsetZ * offsetZ) <= radius * radius) {
                            int targetX = gridX + offsetX;
                            int targetZ = gridZ + offsetZ;

                            float variation = 1.0f - (float)(offsetX * offsetX + offsetZ * offsetZ) / (radius * radius);
                            float upwardPush = 0.5f;
                            float outwardSpread = 6.5f;
                            float burstDensity = 15.0f * variation;

                            glm::vec3 explosionVelocity(
                                offsetX * outwardSpread,
                                upwardPush,
                                offsetZ * outwardSpread
                            );

                            smokeSim.EmitSmoke(targetX, gridY, targetZ, burstDensity, explosionVelocity);
                        }
                    }
                }
            }
            triggerSmokeBurst = false;
        }

        // Tick the fluid dynamics simulation forward
        float smokeSpeedMultiplier = 2.5f;
        smokeSim.Step(simDeltaTime * smokeSpeedMultiplier);

        if (!jellyCube.particles.empty())
        {
            smokeDotTimer += simDeltaTime;
            if (smokeDotTimer >= SMOKE_DOT_INTERVAL)
            {
                // 1. Calculate the center position of the soft-body slime
                glm::vec3 slimeCenter(0.0f);
                for (const auto& p : jellyCube.particles) {
                    slimeCenter += p.pos;
                }
                slimeCenter /= static_cast<float>(jellyCube.particles.size());

                slimeCenter.y -= 0.5f;

                // Map the slime's world position into your smoke grid coordinates
                int slimeGridX = static_cast<int>((slimeCenter.x - smokeGridOrigin.x) / smokeSim.dx);
                int slimeGridY = static_cast<int>((slimeCenter.y - smokeGridOrigin.y) / smokeSim.dx);
                int slimeGridZ = static_cast<int>((slimeCenter.z - smokeGridOrigin.z) / smokeSim.dx);

                // Array boundary safety check to make sure the slime is inside the simulation box
                if (slimeGridX >= 1 && slimeGridX < smokeSim.nx - 1 &&
                    slimeGridY >= 1 && slimeGridY < smokeSim.ny - 1 &&
                    slimeGridZ >= 1 && slimeGridZ < smokeSim.nz - 1)
                {
                    // Look up the localized density value from the solver
                    int cellIndex = smokeSim.dIdx(slimeGridX, slimeGridY, slimeGridZ);
                    float currentSmokeDensity = smokeSim.density[cellIndex];

                    // If there's thick enough smoke, trigger the hit flash effect
                    if (currentSmokeDensity > 0.01f)
                    {
                        isSlimeHit = true;
                        slimeHitTimer = 0.0f;
                    }
                }

                smokeDotTimer = 0.0f; // Reset tick interval
            }
        }
        // Light Parameters
        glm::vec3 lightPos(10.0f, 20.0f, 10.0f);
        glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 0.1f, 40.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // Base Model Transform for player
        glm::mat4 playerModelMatrix = glm::mat4(1.0f);
        playerModelMatrix = glm::translate(playerModelMatrix, charPosition);
        if (glm::length(charFront) > 0.001f)
        {
            glm::vec3 lookDir = glm::normalize(glm::vec3(charFront.x, 0.0f, charFront.z));
            float angle = atan2(lookDir.x, lookDir.z);
            playerModelMatrix = glm::rotate(playerModelMatrix, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        }
        playerModelMatrix = glm::scale(playerModelMatrix, glm::vec3(0.1f));

        auto transforms = animator.GetFinalBoneMatrices();

        // Shadows
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        simpleDepthShader.use();
        simpleDepthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        // Draw Player Shadow
        simpleDepthShader.setMat4("model", playerModelMatrix);
        for (int i = 0; i < transforms.size(); ++i)
            simpleDepthShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);
        playerModel.Draw(simpleDepthShader);

        // Draw Environment Shadow
        glm::mat4 model = glm::mat4(1.0f);
        simpleDepthShader.setMat4("model", model);
        //ourModel.Draw(simpleDepthShader);
        //pillarFloorModel.Draw(simpleDepthShader);

        // Draw Slime Shadow
        glm::mat4 softBodyModelMatrix = glm::mat4(1.0f);
        simpleDepthShader.setMat4("model", softBodyModelMatrix);
        jellyCube.Draw(simpleDepthShader);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, dofFBO);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Calculate Camera uniform views
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)display_w / (float)display_h, 0.1f, 100.0f);
        float lerpFactor = 1.0f - expf(-smoothSpeed * deltaTime);
        orbitYaw = glm::mix(orbitYaw, targetYaw, lerpFactor);
        orbitPitch = glm::mix(orbitPitch, targetPitch, lerpFactor);

        float smoothYawRad = glm::radians(orbitYaw);
        float smoothPitchRad = glm::radians(orbitPitch);
        float camX = cameraRadius * cos(smoothPitchRad) * sin(smoothYawRad);
        float camY = cameraRadius * sin(smoothPitchRad);
        float camZ = cameraRadius * cos(smoothPitchRad) * cos(smoothYawRad);

        glm::vec3 cameraPos = charPosition + glm::vec3(camX, camY, camZ);
        glm::vec3 charPosWithOffset = charPosition + glm::vec3(0.0f, 1.0f, 0.0f);
        glm::mat4 view = glm::lookAt(cameraPos, charPosWithOffset, glm::vec3(0.0f, 1.0f, 0.0f));

        // Render Player Model with shadow bindings mapped
        animShader.use();
        animShader.setMat4("projection", projection);
        animShader.setMat4("view", view);
        animShader.setVec3("cameraPos", cameraPos);
        animShader.setVec3("lightPos", lightPos);
        animShader.setVec3("lightColor", glm::vec3(1.01f, 1.01f, 1.01f));

        animShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, depthMapTextureID);
        animShader.setInt("shadowMap", 6);
        animShader.setFloat("reflectionIntensity", playerReflectionIntensity);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        animShader.setInt("environmentMap", 5);

        for (int i = 0; i < transforms.size(); ++i)
            animShader.setMat4("finalBonesMatrices[" + std::to_string(i) + "]", transforms[i]);

        animShader.setMat4("model", playerModelMatrix);
        playerModel.Draw(animShader);

        // Render Environments (Ruins)
        ourShader.use();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("cameraPos", cameraPos);
        ourShader.setVec3("lightPos", lightPos);
        ourShader.setVec3("lightColor", glm::vec3(1.01f, 1.01f, 1.01f));
        ourShader.setFloat("roughnessModifier", 1.0f);
        ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        ourShader.setInt("environmentMap", 5);

        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, depthMapTextureID);
        ourShader.setInt("shadowMap", 6);

        model = glm::mat4(1.0f);
        ourShader.setMat4("model", model);
        ourModel.Draw(ourShader);

        // Render Environments (Floor)
        floorShader.use();
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);
        floorShader.setVec3("cameraPos", cameraPos);
        floorShader.setVec3("lightPos", lightPos);
        floorShader.setVec3("lightColor", glm::vec3(1.01f, 1.01f, 1.01f));
        floorShader.setFloat("roughnessModifier", 1.0f);

        floorShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, depthMapTextureID);
        floorShader.setInt("shadowMap", 6);

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        floorShader.setInt("environmentMap", 5);

        model = glm::mat4(1.0f);
        floorShader.setMat4("model", model);
        pillarFloorModel.Draw(floorShader);

        // Render Skybox
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view", view);
        skyboxShader.setMat4("projection", projection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        skyboxShader.setInt("skybox", 0);
        renderCube();
        glDepthFunc(GL_LESS);

        // Render Slime
        slimeShader.use();
        softBodyModelMatrix = glm::mat4(1.0f);
        slimeShader.setMat4("projection", projection);
        slimeShader.setMat4("view", view);
        slimeShader.setVec3("cameraPos", cameraPos);
        slimeShader.setMat4("model", softBodyModelMatrix);
        slimeShader.setVec3("lightPos", lightPos);
        slimeShader.setVec3("lightColor", glm::vec3(0.0f, 0.0f, 0.0f));
        slimeShader.setFloat("roughnessModifier", 0.0f);
        slimeShader.setBool("isHit", isSlimeHit);
        slimeShader.setFloat("hitProgress", slimeHitTimer / SLIME_HIT_DURATION);

        if (!slimeModel.meshes.empty() && !slimeModel.meshes[0].textures.empty())
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, slimeModel.meshes[0].textures[0].id);
            slimeShader.setInt("texture_diffuse1", 0);
        }

        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        slimeShader.setInt("environmentMap", 5);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        jellyCube.Draw(slimeShader);
        glDisable(GL_BLEND);

        // Render Fire
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);

        fireShader.use();
        fireShader.setMat4("projection", projection);
        fireShader.setMat4("view", view);
        float currentTime = static_cast<float>(glfwGetTime());
        fireShader.setFloat("time", currentTime);

        glm::vec3 fireScale = glm::vec3(2.0f, 2.0f, 1.0f);
        float angles[] = { 0.0f, 45.0f, 90.0f, 135.0f };

        // Iterate over every fire instance position
        for (unsigned int f = 0; f < firePositions.size(); ++f)
        {
            glm::vec3 currentFirePos = firePositions[f];

            // Render the 4 interlocking billboard quads for this specific fire
            for (int i = 0; i < 4; ++i)
            {
                glm::mat4 modelCrossed = glm::mat4(1.0f);
                modelCrossed = glm::translate(modelCrossed, currentFirePos);
                modelCrossed = glm::rotate(modelCrossed, glm::radians(angles[i]), glm::vec3(0.0f, 1.0f, 0.0f));
                modelCrossed = glm::scale(modelCrossed, fireScale);

                fireShader.setMat4("model", modelCrossed);

                fireShader.setFloat("planeOffset", (static_cast<float>(i) * 15.5f) + (static_cast<float>(f) * 100.0f));

                renderQuad();
            }
        }

        glDepthMask(GL_TRUE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);

        // --- RENDER 3D SMOKE ---
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Smooth alpha blending for smoke
        glDepthMask(GL_FALSE); // Don't write to depth buffer so particles don't clip each other

        // Gather all visible cells from the grid
        visibleSmokeCells.clear();
        for (int k = 1; k < smokeSim.nz - 1; ++k) {
            for (int j = 1; j < smokeSim.ny - 1; ++j) {
                for (int i = 1; i < smokeSim.nx - 1; ++i) {
                    float d = smokeSim.density[smokeSim.dIdx(i, j, k)];

                    if (d > 0.02f) { // Visibility cutoff threshold
                        // 1. Calculate positional jitter (up to half a cell size)
                        float maxJitter = smokeSim.dx * 0.5f;
                        glm::vec3 jitter(
                            (hash3D(i, j, k) - 0.5f) * maxJitter,
                            (hash3D(j, k, i) - 0.5f) * maxJitter,
                            (hash3D(k, i, j) - 0.5f) * maxJitter
                        );

                        glm::vec3 worldPos = smokeGridOrigin + glm::vec3(i, j, k) * smokeSim.dx + jitter;

                        // 2. Dynamic scale: higher density = slightly larger quads to ensure smooth overlapping
                        float baseSize = smokeSim.dx * 1.5f; // Make quads slightly larger than a cell so they blend
                        glm::vec3 dynamicScale = glm::vec3(baseSize * (0.7f + d * 0.5f));

                        // 3. Random rotation angle (0 to 360 degrees)
                        float randomRotation = hash3D(k, j, i) * 360.0f;

                        visibleSmokeCells.push_back({ worldPos, d, dynamicScale, randomRotation });
                    }
                }
            }
        }

        // Sort smoke cells back-to-front relative to camera position to avoid alpha artifacts
        std::sort(visibleSmokeCells.begin(), visibleSmokeCells.end(),
            [&cameraPos](const SmokeParticleInstance& a, const SmokeParticleInstance& b) {
                return glm::distance2(cameraPos, a.position) > glm::distance2(cameraPos, b.position);
            }
        );

        smokeShader.use();
        smokeShader.setMat4("projection", projection);
        smokeShader.setMat4("view", view);
        smokeShader.setVec3("lightPos", lightPos);
        smokeShader.setVec3("cameraPos", cameraPos);
        smokeShader.setVec3("lightColor", glm::vec3(1.01f, 1.01f, 1.01f));

        glm::vec3 smokeScale = glm::vec3(3.0f);
        for (const auto& smokeCell : visibleSmokeCells) {
            for (int i = 0; i < 2; ++i) {
                glm::mat4 modelSmoke = glm::mat4(1.0f);

                // 1. Move to the jittered cell position
                modelSmoke = glm::translate(modelSmoke, smokeCell.position);

                // 2. Rotate 90 degrees for the interlocking cross shape, 
                //    AND inject the unique random rotation to break up the texture pattern
                float totalRotation = (i * 90.0f) + smokeCell.rotation;
                modelSmoke = glm::rotate(modelSmoke, glm::radians(totalRotation), glm::vec3(0.0f, 1.0f, 0.0f));

                // 3. Apply the density-based scale
                modelSmoke = glm::scale(modelSmoke, smokeCell.scale);

                smokeShader.setMat4("model", modelSmoke);
                smokeShader.setFloat("cellDensity", smokeCell.density);

                renderQuad();
            }
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        dofShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dofColorTex);
        dofShader.setInt("sceneColor", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dofDepthTex);
        dofShader.setInt("sceneDepth", 1);

        // Pass essential camera projections & settings down
        dofShader.setFloat("nearPlane", 0.1f);
        dofShader.setFloat("farPlane", 100.0f);
        dofShader.setFloat("focusDistance", dofFocusDistance);
        dofShader.setFloat("focusRange", dofFocusRange);
        dofShader.setFloat("maxBlurSize", dofMaxBlur);

        glDisable(GL_DEPTH_TEST);
        renderQuad();
        glEnable(GL_DEPTH_TEST);

        UpdateImGui();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float smoothYawRad = glm::radians(orbitYaw);
    glm::vec3 camForwardXZ = glm::vec3(-sin(smoothYawRad), 0.0f, -cos(smoothYawRad));
    if (glm::length(camForwardXZ) > 0.0f)
        camForwardXZ = glm::normalize(camForwardXZ);

    glm::vec3 camRightXZ = glm::normalize(glm::cross(camForwardXZ, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 moveDir(0.0f);

    bool mouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseDown && !lastMouseDown)
    {
        if (charState == IDLE || charState == RUN)
        {
            charState = IDLE_SLASH;
            movementLocked = true;

            // Start the delayed attack sequence
            isAttackPending = true;
            attackTimer = 0.0f;
        }
    }
    lastMouseDown = mouseDown;

    // Movement Input
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += camForwardXZ;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= camForwardXZ;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= camRightXZ;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += camRightXZ;

    if (glm::length(moveDir) > 0.0f)
    {
        if (movementLocked)
        {
            isMoving = false;
        }
        else
        {
            moveDir = glm::normalize(moveDir);
            charPosition += moveDir * charSpeed * deltaTime;
            charFrontTarget = moveDir;
            isMoving = true;
        }
    }
    else
    {
        isMoving = false;
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        // Only trigger if it wasn't already being held down on the last frame
        if (!rKeyWasPressed)
        {
            triggerSmokeBurst = true;
            rKeyWasPressed = true; // Lock it until the player releases the key
            std::cout << "R Key Pressed: Smoke burst triggered!" << std::endl;
        }
    }
    else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
    {
        // Unlock the trigger once the player lets go of the key
        rKeyWasPressed = false;
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
void UpdateImGui()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(550, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_Once);

    
    

    ImGui::Begin("Shader Setting");

    ImGui::Text("WASD - Walk");
    ImGui::Text("Left Click - Melee Attack");
    ImGui::Text("R - Smoke Attack");

    ImGui::Separator();

    ImGui::Text("Player Shader Settings");
    ImGui::SliderFloat("playerReflectionIntensity", &playerReflectionIntensity, 0.0f, 1.0f);

    ImGui::Separator();

    ImGui::Text("Depth Of Field");

    extern float dofFocusDistance;
    extern float dofFocusRange;
    extern float dofMaxBlur;

    ImGui::SliderFloat("Focus Distance",
        &dofFocusDistance,
        0.1f,
        50.0f);

    ImGui::SliderFloat("Focus Range",
        &dofFocusRange,
        0.1f,
        20.0f);

    ImGui::SliderFloat("Max Blur",
        &dofMaxBlur,
        0.0f,
        0.03f);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    // Apply to target rotation only
    targetYaw -= xoffset;
    targetPitch -= yoffset;

    // Clamp pitch
    if (targetPitch > 89.0f) targetPitch = 89.0f;
    if (targetPitch < -89.0f) targetPitch = -89.0f;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

unsigned int loadHDRTexture(const char* path)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float* data = stbi_loadf(path, &width, &height, &nrComponents, 0);
    unsigned int hdrTexture = 0;

    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Successfully loaded HDR texture map!" << std::endl;
    }
    else
    {
        std::cout << "Failed to load HDR texture at path: " << path << std::endl;
    }

    return hdrTexture;
}

void renderCube()
{
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
            // front face
            -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
            // right face
            1.0f,  1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,
            // bottom face
            -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,
            // top face
            -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void renderQuad() 
{
    if (quadVAO == 0) 
    {
        float quadVertices[] = 
        {
            // positions   // texCoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
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
