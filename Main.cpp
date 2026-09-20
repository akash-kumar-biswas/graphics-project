#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
// 3D CABLE CAR SIMULATION
// Computer Graphics Project - 3D Transformations
//
// Features:
// - One global/free camera
// - Two stations
// - One cable car travels one station-to-station trip when G is pressed
// - Q/E rotate cable car around its OWN local Y-axis
// - Station pulleys rotate while cable car moves
// - Keyboard-controlled ship
// - Ship remains on water and cannot enter islands/mountains/stations
// - Reusable low-poly mountains, trees and primitive objects
// - Model, View, Projection matrices + perspective + depth testing
//
// NO lighting, NO textures, NO imported models.
// ============================================================


// ============================================================
// WINDOW
// ============================================================

const unsigned int INITIAL_WIDTH = 1280;
const unsigned int INITIAL_HEIGHT = 720;

int framebufferWidth = INITIAL_WIDTH;
int framebufferHeight = INITIAL_HEIGHT;

const float PI = 3.14159265359f;


// ============================================================
// SHADERS
// ============================================================

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 objectColor;

void main()
{
    FragColor = vec4(objectColor, 1.0);
}
)";


// ============================================================
// SIMPLE MESH TYPE
// ============================================================

struct Mesh
{
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    int vertexCount = 0;
};

Mesh cubeMesh;
Mesh planeMesh;
Mesh cylinderMesh;
Mesh coneMesh;

unsigned int shaderProgram = 0;

int modelLoc = -1;
int viewLoc = -1;
int projectionLoc = -1;
int colorLoc = -1;


// ============================================================
// COLORS
// ============================================================

const glm::vec3 SKY_COLOR(0.035f, 0.035f, 0.045f);
const glm::vec3 WATER_COLOR(0.08f, 0.52f, 0.72f);
const glm::vec3 WATER_EDGE_COLOR(0.07f, 0.43f, 0.61f);

const glm::vec3 ISLAND_COLOR(0.29f, 0.43f, 0.20f);
const glm::vec3 ISLAND_EDGE(0.37f, 0.29f, 0.18f);

const glm::vec3 MOUNTAIN_GREEN(0.33f, 0.43f, 0.24f);
const glm::vec3 MOUNTAIN_GREEN_2(0.39f, 0.47f, 0.27f);
const glm::vec3 MOUNTAIN_DARK(0.28f, 0.36f, 0.20f);
const glm::vec3 ROCK_COLOR(0.49f, 0.46f, 0.39f);

const glm::vec3 TREE_TRUNK(0.36f, 0.19f, 0.08f);
const glm::vec3 TREE_GREEN(0.07f, 0.39f, 0.14f);
const glm::vec3 TREE_GREEN_2(0.10f, 0.49f, 0.18f);

const glm::vec3 STATION_WALL(0.73f, 0.76f, 0.76f);
const glm::vec3 STATION_WALL_2(0.65f, 0.69f, 0.71f);
const glm::vec3 STATION_DARK(0.21f, 0.24f, 0.27f);
const glm::vec3 STATION_FLOOR(0.44f, 0.35f, 0.26f);
const glm::vec3 STATION_ACCENT_A(0.74f, 0.16f, 0.13f);
const glm::vec3 STATION_ACCENT_B(0.14f, 0.34f, 0.70f);

const glm::vec3 CABLE_COLOR(0.72f, 0.76f, 0.82f);
const glm::vec3 PULLEY_COLOR(0.15f, 0.17f, 0.19f);
const glm::vec3 PULLEY_SPOKE(0.69f, 0.71f, 0.72f);

const glm::vec3 CABIN_RED(0.80f, 0.09f, 0.11f);
const glm::vec3 CABIN_DARK_RED(0.55f, 0.05f, 0.07f);
const glm::vec3 CABIN_WHITE(0.92f, 0.93f, 0.92f);
const glm::vec3 CABIN_WINDOW(0.16f, 0.49f, 0.68f);
const glm::vec3 CABIN_DARK(0.11f, 0.13f, 0.15f);

// Light metallic color for the cable-car hanger and cable grip.
// Visible on the dark background, but still mechanically neutral.
const glm::vec3 CABIN_HANGER_COLOR(0.64f, 0.68f, 0.74f);

const glm::vec3 SHIP_HULL(0.48f, 0.12f, 0.08f);
const glm::vec3 SHIP_WHITE(0.91f, 0.92f, 0.89f);
const glm::vec3 SHIP_RED(0.78f, 0.12f, 0.10f);
const glm::vec3 SHIP_WINDOW(0.12f, 0.38f, 0.57f);
const glm::vec3 SHIP_DARK(0.10f, 0.12f, 0.14f);


// ============================================================
// GLOBAL CAMERA
// ============================================================

glm::vec3 cameraPosition(0.0f, 10.5f, 32.0f);

float cameraYaw = -90.0f;
float cameraPitch = -17.0f;

float cameraMoveSpeed = 8.0f;
float cameraLookSpeed = 70.0f;
float mouseSensitivity = 0.10f;

bool firstMouse = true;

double lastMouseX = INITIAL_WIDTH / 2.0;
double lastMouseY = INITIAL_HEIGHT / 2.0;




// Camera mode:
// false -> global/free camera
// true  -> camera attached to the front of the cable car
bool cableCarCameraActive = false;
// ============================================================
// WATER / LAND LAYOUT
// ============================================================

// Water plane:
// x = [-16, 16]
// z = [-10, 10]
const float WATER_HALF_X = 22.0f;
const float WATER_HALF_Z = 11.5f;

// The ship has a conservative collision circle.
// This prevents its visible geometry from clipping into land.
const float SHIP_COLLISION_RADIUS = 1.70f;


// ============================================================
// COLLISION HELPERS
// ============================================================

struct RectObstacle
{
    float minX;
    float maxX;
    float minZ;
    float maxZ;
};

struct CircleObstacle
{
    glm::vec2 center;
    float radius;
};

// Islands are non-water areas.
const RectObstacle LEFT_ISLAND =
{
    -20.0f, -8.4f,
    -5.3f,   5.3f
};

const RectObstacle RIGHT_ISLAND =
{
     8.4f, 20.0f,
    -5.3f,  5.3f
};

// Stations extend slightly toward the water.
// They are also collision obstacles for the ship.
const RectObstacle STATION_A_OBSTACLE =
{
    -12.3f, -6.2f,
     -2.2f,  2.2f
};

const RectObstacle STATION_B_OBSTACLE =
{
     6.2f, 12.3f,
    -2.2f,  2.2f
};

// Some mountains extend beyond the rectangular island edge.
// Circular footprints stop the ship from clipping through those slopes.
const std::vector<CircleObstacle> MOUNTAIN_OBSTACLES =
{
    { glm::vec2(-14.2f, -0.7f), 4.15f },
    { glm::vec2(-17.3f, -3.6f), 3.00f },
    { glm::vec2(-16.4f,  3.8f), 2.70f },

    { glm::vec2(14.2f, -0.5f), 4.15f },
    { glm::vec2(17.3f, -3.6f), 3.00f },
    { glm::vec2(16.4f,  3.8f), 2.70f }
};


// ============================================================
// CABLE CAR / STATIONS
// ============================================================

// Station floor centers.
// They intentionally sit near the inner edges of the two mountain areas.
const glm::vec3 stationAPosition(-9.20f, 4.55f, 0.0f);
const glm::vec3 stationBPosition(9.20f, 5.05f, 0.0f);

// Pulley centers / cable endpoints.
// Keep both wheels in the original clear station-centered positions.
const glm::vec3 cableStart(-9.20f, 6.85f, 0.0f);
const glm::vec3 cableEnd(9.20f, 7.35f, 0.0f);

// The cabin does NOT go all the way to either pulley center.
// It docks slightly toward the INNER side of each station.
// This keeps the original wheel appearance but prevents overlap.
//
// Physical cable:
//   t ~= 0.145 -> Station A cabin docking point
//   t ~= 0.855 -> Station B cabin docking point
const float CABLE_DOCK_T_A = 0.10f;
const float CABLE_DOCK_T_B = 0.90f;

// Cabin center hangs below the cable.
const float CABIN_DROP_FROM_CABLE = 1.35f;

// 0 = docked at Station A, 1 = docked at Station B.
double cableT = 0.0;

// +1 = next trip A -> B, -1 = next trip B -> A.
float cableDirection = 1.0f;

// Starts STOPPED at Station A. Press G to start one complete trip.
bool cableMoving = false;

float cableSpeed = 0.105f;

float cabinYaw = 0.0f;
float cabinRotateSpeed = 75.0f;

// Pulleys rotate only while the cabin is actually moving.
float pulleyAngle = 0.0f;

// Visible pulley radius in world units:
// base cylinder radius 0.5 * scale 1.25 = 0.625.
const float PULLEY_RADIUS_WORLD = 0.59f;


// ============================================================
// SHIP
// ============================================================

// Start in clear water between the two islands.
glm::vec3 shipPosition(0.0f, 0.26f, 6.4f);

// Local forward = +Z.
// 180 degrees initially points toward -Z / center of the scene.
float shipYaw = 180.0f;

float shipMoveSpeed = 3.2f;
float shipRotateSpeed = 70.0f;


// ============================================================
// TIMING
// ============================================================

float deltaTime = 0.0f;
float lastFrameTime = 0.0f;

// Fixed-step animation removes visible speed jitter when frame time varies.
const double FIXED_ANIMATION_STEP = 1.0 / 120.0;
double animationAccumulator = 0.0;

// Previous physics state is used for smooth render interpolation.
double previousCableT = 0.0;
float previousPulleyAngle = 0.0f;


// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
void mouseCallback(GLFWwindow* window, double xpos, double ypos);
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

void processContinuousInput(GLFWwindow* window);
void printControls();

unsigned int createShaderProgram(
    const char* vertexSource,
    const char* fragmentSource
);

void checkShaderCompile(
    unsigned int shader,
    const char* name
);

void checkProgramLink(
    unsigned int program
);

Mesh createMesh(
    const std::vector<float>& vertices
);

Mesh createCubeMesh();
Mesh createPlaneMesh();
Mesh createCylinderMesh(int segments = 16);
Mesh createConeMesh(int segments = 8);

void deleteMesh(Mesh& mesh);

void drawMesh(
    const Mesh& mesh,
    const glm::mat4& model,
    const glm::vec3& color
);

void drawCube(
    const glm::mat4& model,
    const glm::vec3& color
);

void drawPlane(
    const glm::mat4& model,
    const glm::vec3& color
);

void drawCylinder(
    const glm::mat4& model,
    const glm::vec3& color
);

void drawCone(
    const glm::mat4& model,
    const glm::vec3& color
);

void drawWater();
void drawIslands();

void drawMountain(
    const glm::vec3& basePosition,
    const glm::vec3& scale,
    float yRotation,
    const glm::vec3& color
);

float mountainSurfaceY(
    const glm::vec3& mountainBase,
    const glm::vec3& mountainScale,
    float worldX,
    float worldZ
);

void drawTree(
    const glm::vec3& position,
    float scale,
    float yRotation
);

void drawTrees();

void drawStation(
    const glm::vec3& position,
    bool stationA
);

void drawCable();

void drawPulley(
    const glm::vec3& position,
    float angleDegrees
);

void drawCableCar(
    const glm::vec3& position,
    float yawDegrees
);

void drawShip();
void drawScene();

void updateAnimations();

glm::vec3 getCableCarPosition();
glm::vec3 getCameraFront();

glm::mat4 getCableCarParentMatrix();
glm::mat4 getCableCarCameraView();

double getRenderedCableT();
float getRenderedPulleyAngle();

bool circleIntersectsRectangle(
    const glm::vec2& center,
    float radius,
    const RectObstacle& rect
);

bool shipPositionIsValid(
    const glm::vec3& candidate
);

void tryMoveShip(float signedDistance);


// ============================================================
// MAIN
// ============================================================

int main()
{
    // --------------------------------------------------------
    // GLFW
    // --------------------------------------------------------

    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW.\n";
        return -1;
    }

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
    );

    // Simple anti-aliasing helps reduce visible edge shimmer.
    glfwWindowHint(
        GLFW_SAMPLES,
        4
    );

    // Open maximized so the complete scene is easy to view.
    glfwWindowHint(
        GLFW_MAXIMIZED,
        GLFW_TRUE
    );

    GLFWwindow* window =
        glfwCreateWindow(
            INITIAL_WIDTH,
            INITIAL_HEIGHT,
            "3D Cable Car Simulation - Transformations",
            nullptr,
            nullptr
        );

    if (!window)
    {
        std::cout << "Failed to create GLFW window.\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Explicit maximize call for Windows/Visual Studio.
    glfwMaximizeWindow(window);

    // VSync
    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(
        window,
        framebufferSizeCallback
    );

    glfwSetCursorPosCallback(
        window,
        mouseCallback
    );

    glfwSetKeyCallback(
        window,
        keyCallback
    );

    // Keep the normal computer cursor visible and usable.
    // Hold RIGHT mouse button while moving the mouse to look around.
    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        GLFW_CURSOR_NORMAL
    );

    // --------------------------------------------------------
    // GLAD
    // --------------------------------------------------------

    if (!gladLoadGL())
    {
        std::cout << "Failed to initialize GLAD.\n";

        glfwDestroyWindow(window);
        glfwTerminate();

        return -1;
    }

    // Get the REAL framebuffer size after maximization.
    glfwGetFramebufferSize(
        window,
        &framebufferWidth,
        &framebufferHeight
    );

    glViewport(
        0,
        0,
        framebufferWidth,
        framebufferHeight
    );

    // Correct front/behind object visibility.
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_MULTISAMPLE);

    // --------------------------------------------------------
    // Shader + primitive geometry
    // --------------------------------------------------------

    shaderProgram =
        createShaderProgram(
            vertexShaderSource,
            fragmentShaderSource
        );

    modelLoc =
        glGetUniformLocation(
            shaderProgram,
            "model"
        );

    viewLoc =
        glGetUniformLocation(
            shaderProgram,
            "view"
        );

    projectionLoc =
        glGetUniformLocation(
            shaderProgram,
            "projection"
        );

    colorLoc =
        glGetUniformLocation(
            shaderProgram,
            "objectColor"
        );

    cubeMesh = createCubeMesh();
    planeMesh = createPlaneMesh();
    cylinderMesh = createCylinderMesh(16);
    coneMesh = createConeMesh(8);

    printControls();
    std::cout << "Cable car is docked at Station A. Press G to depart.\n";

    // Initialize timing after all setup.
    lastFrameTime =
        static_cast<float>(
            glfwGetTime()
            );

    // --------------------------------------------------------
    // Render loop
    // --------------------------------------------------------

    while (!glfwWindowShouldClose(window))
    {
        float currentTime =
            static_cast<float>(
                glfwGetTime()
                );

        deltaTime =
            currentTime -
            lastFrameTime;

        lastFrameTime =
            currentTime;

        // Prevent large jumps after dragging/debug pauses.
        deltaTime =
            std::min(
                deltaTime,
                0.05f
            );

        processContinuousInput(window);
        updateAnimations();

        glClearColor(
            SKY_COLOR.r,
            SKY_COLOR.g,
            SKY_COLOR.b,
            1.0f
        );

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        glUseProgram(
            shaderProgram
        );

        // ----------------------------------------------------
        // VIEW MATRIX
        // ----------------------------------------------------

        glm::mat4 view(1.0f);

        if (cableCarCameraActive)
        {
            // Camera is attached to the cable car.
            // It translates and rotates with the cabin.
            view =
                getCableCarCameraView();
        }
        else
        {
            glm::vec3 cameraFront =
                getCameraFront();

            view =
                glm::lookAt(
                    cameraPosition,
                    cameraPosition + cameraFront,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );
        }

        // ----------------------------------------------------
        // PROJECTION MATRIX
        // ----------------------------------------------------

        float aspect =
            static_cast<float>(framebufferWidth) /
            static_cast<float>(
                std::max(framebufferHeight, 1)
                );

        glm::mat4 projection =
            glm::perspective(
                glm::radians(45.0f),
                aspect,
                0.1f,
                100.0f
            );

        glUniformMatrix4fv(
            viewLoc,
            1,
            GL_FALSE,
            glm::value_ptr(view)
        );

        glUniformMatrix4fv(
            projectionLoc,
            1,
            GL_FALSE,
            glm::value_ptr(projection)
        );

        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    deleteMesh(cubeMesh);
    deleteMesh(planeMesh);
    deleteMesh(cylinderMesh);
    deleteMesh(coneMesh);

    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}


// ============================================================
// INPUT
// ============================================================

void keyCallback(
    GLFWwindow* window,
    int key,
    int,
    int action,
    int)
{
    if (action != GLFW_PRESS)
        return;

    if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(
            window,
            true
        );
    }

    // G = start exactly one station-to-station trip.
    else if (key == GLFW_KEY_G)
    {
        if (!cableMoving)
        {
            // Start from an exact docked state.
            previousCableT = cableT;
            previousPulleyAngle = pulleyAngle;
            animationAccumulator = 0.0;

            cableMoving = true;

            if (cableDirection > 0.0f)
            {
                std::cout
                    << "Cable car departing Station A -> Station B\n";
            }
            else
            {
                std::cout
                    << "Cable car departing Station B -> Station A\n";
            }
        }
    }

    // C = switch between global camera and cable-car front camera.
    else if (key == GLFW_KEY_C)
    {
        cableCarCameraActive =
            !cableCarCameraActive;

        firstMouse = true;

        std::cout
            << "Camera: "
            << (
                cableCarCameraActive
                ? "CABLE CAR FRONT"
                : "GLOBAL"
                )
            << "\n";
    }

    else if (
        key == GLFW_KEY_EQUAL ||
        key == GLFW_KEY_KP_ADD
        )
    {
        cableSpeed =
            std::min(
                cableSpeed + 0.02f,
                0.35f
            );

        std::cout
            << "Cable + pulley speed: "
            << cableSpeed
            << "\n";
    }

    else if (
        key == GLFW_KEY_MINUS ||
        key == GLFW_KEY_KP_SUBTRACT
        )
    {
        cableSpeed =
            std::max(
                cableSpeed - 0.02f,
                0.03f
            );

        std::cout
            << "Cable + pulley speed: "
            << cableSpeed
            << "\n";
    }

    else if (key == GLFW_KEY_U)
    {
        shipMoveSpeed =
            std::min(
                shipMoveSpeed + 0.4f,
                7.0f
            );

        std::cout
            << "Ship speed: "
            << shipMoveSpeed
            << "\n";
    }

    else if (key == GLFW_KEY_O)
    {
        shipMoveSpeed =
            std::max(
                shipMoveSpeed - 0.4f,
                0.8f
            );

        std::cout
            << "Ship speed: "
            << shipMoveSpeed
            << "\n";
    }
}

void processContinuousInput(
    GLFWwindow* window)
{
    // --------------------------------------------------------
    // GLOBAL CAMERA MOVEMENT / ROTATION
    //
    // Disabled while cable-car camera is active.
    // The cable-car camera itself does NOT rotate independently.
    // --------------------------------------------------------

    if (!cableCarCameraActive)
    {
        glm::vec3 front =
            getCameraFront();

        glm::vec3 worldUp(
            0.0f,
            1.0f,
            0.0f
        );

        glm::vec3 right =
            glm::normalize(
                glm::cross(
                    front,
                    worldUp
                )
            );

        float cameraStep =
            cameraMoveSpeed *
            deltaTime;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            cameraPosition += front * cameraStep;

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            cameraPosition -= front * cameraStep;

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            cameraPosition -= right * cameraStep;

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            cameraPosition += right * cameraStep;

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            cameraPosition += worldUp * cameraStep;

        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
            cameraPosition -= worldUp * cameraStep;

        float lookStep =
            cameraLookSpeed *
            deltaTime;

        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cameraYaw -= lookStep;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cameraYaw += lookStep;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cameraPitch += lookStep;

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cameraPitch -= lookStep;

        cameraPitch =
            std::clamp(
                cameraPitch,
                -89.0f,
                89.0f
            );
    }

    // --------------------------------------------------------
    // CABLE-CAR LOCAL Y ROTATION
    //
    // Q/E rotates the cabin.
    // If C-camera is active, the attached camera rotates too.
    // --------------------------------------------------------

    float cabinStep =
        cabinRotateSpeed *
        deltaTime;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cabinYaw += cabinStep;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cabinYaw -= cabinStep;

    if (cabinYaw > 360.0f)
        cabinYaw -= 360.0f;

    if (cabinYaw < -360.0f)
        cabinYaw += 360.0f;

    // --------------------------------------------------------
    // SHIP LOCAL ROTATION
    // --------------------------------------------------------

    float shipTurnStep =
        shipRotateSpeed *
        deltaTime;

    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
        shipYaw += shipTurnStep;

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
        shipYaw -= shipTurnStep;

    if (shipYaw > 360.0f)
        shipYaw -= 360.0f;

    if (shipYaw < -360.0f)
        shipYaw += 360.0f;

    // --------------------------------------------------------
    // SHIP FORWARD / BACKWARD
    // --------------------------------------------------------

    float shipStep =
        shipMoveSpeed *
        deltaTime;

    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
        tryMoveShip(shipStep);

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
        tryMoveShip(-shipStep);

    shipPosition.y = 0.26f;
}


// ============================================================
// MOUSE / WINDOW / CAMERA
// ============================================================

void mouseCallback(
    GLFWwindow* window,
    double xpos,
    double ypos)
{
    // The cable-car camera is a fixed child of the cabin.
    // It has no independent mouse rotation.
    if (cableCarCameraActive)
    {
        firstMouse = true;
        return;
    }

    // Keep the normal Windows cursor visible.
    // Hold RIGHT mouse button only when you want camera look.
    if (
        glfwGetMouseButton(
            window,
            GLFW_MOUSE_BUTTON_RIGHT
        )
        != GLFW_PRESS
        )
    {
        firstMouse = true;
        return;
    }

    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
        return;
    }

    float xOffset =
        static_cast<float>(
            xpos - lastMouseX
            );

    float yOffset =
        static_cast<float>(
            lastMouseY - ypos
            );

    lastMouseX = xpos;
    lastMouseY = ypos;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    cameraPitch =
        std::clamp(
            cameraPitch,
            -89.0f,
            89.0f
        );
}

void framebufferSizeCallback(
    GLFWwindow*,
    int width,
    int height)
{
    framebufferWidth =
        std::max(
            width,
            1
        );

    framebufferHeight =
        std::max(
            height,
            1
        );

    glViewport(
        0,
        0,
        framebufferWidth,
        framebufferHeight
    );
}


glm::vec3 getCameraFront()
{
    glm::vec3 front;

    front.x =
        std::cos(
            glm::radians(cameraYaw)
        )
        *
        std::cos(
            glm::radians(cameraPitch)
        );

    front.y =
        std::sin(
            glm::radians(cameraPitch)
        );

    front.z =
        std::sin(
            glm::radians(cameraYaw)
        )
        *
        std::cos(
            glm::radians(cameraPitch)
        );

    return glm::normalize(
        front
    );
}


void printControls()
{
    std::cout << "\n=============================================\n";
    std::cout << "        3D CABLE CAR SIMULATION\n";
    std::cout << "=============================================\n";

    std::cout << "CABLE CAR\n";
    std::cout << "  G           : Start one station-to-station trip\n";
    std::cout << "                A -> B, stop; next G -> B -> A, stop\n";
    std::cout << "  Q / E       : Rotate cabin around LOCAL Y-axis\n";
    std::cout << "  + / -       : Increase / decrease cable + pulley speed\n";
    std::cout << "  C           : Global camera <-> cable-car front camera\n\n";

    std::cout << "SHIP\n";
    std::cout << "  I / K       : Forward / backward\n";
    std::cout << "  J / L       : Turn left / right\n";
    std::cout << "  U / O       : Increase / decrease ship speed\n";
    std::cout << "  Collision   : Water only; islands, mountains and stations blocked\n\n";

    std::cout << "GLOBAL CAMERA\n";
    std::cout << "  W / S       : Forward / backward\n";
    std::cout << "  A / D       : Left / right\n";
    std::cout << "  R / F       : Up / down\n";
    std::cout << "  Arrow Keys  : Look around\n";
    std::cout << "  Right Mouse : Hold + move to look around\n\n";

    std::cout << "CABLE-CAR CAMERA\n";
    std::cout << "  No independent rotation\n";
    std::cout << "  Q / E rotates the cabin and attached camera together\n\n";

    std::cout << "  Cursor      : Visible and usable\n";
    std::cout << "  ESC         : Exit\n";
    std::cout << "=============================================\n\n";
}

// ============================================================
// CABLE CAR ANIMATION
// ============================================================

void updateAnimations()
{
    // Accumulate real frame time, but simulate movement using
    // a constant 120 Hz step. This makes cable motion stable even
    // when individual rendered frames take slightly different times.
    animationAccumulator +=
        static_cast<double>(
            std::min(
                deltaTime,
                0.05f
            )
            );

    while (
        animationAccumulator >=
        FIXED_ANIMATION_STEP
        )
    {
        previousCableT =
            cableT;

        previousPulleyAngle =
            pulleyAngle;

        if (cableMoving)
        {
            const float movementDirection =
                cableDirection;

            // ------------------------------------------------
            // CABLE-CAR MOVEMENT
            // ------------------------------------------------
            cableT +=
                static_cast<double>(cableDirection)
                *
                static_cast<double>(cableSpeed)
                *
                FIXED_ANIMATION_STEP;

            // Station B
            if (cableT >= 1.0)
            {
                cableT = 1.0;
                cableMoving = false;
                cableDirection = -1.0f;

                std::cout
                    << "Cable car arrived at Station B and stopped. "
                    << "Press G to return to Station A.\n";
            }

            // Station A
            else if (cableT <= 0.0)
            {
                cableT = 0.0;
                cableMoving = false;
                cableDirection = 1.0f;

                std::cout
                    << "Cable car arrived at Station A and stopped. "
                    << "Press G to travel to Station B.\n";
            }

            // ------------------------------------------------
            // PULLEY ROTATION
            // Same physical relation as before:
            // angular speed = linear speed / radius
            // ------------------------------------------------
            if (
                std::abs(
                    cableT -
                    previousCableT
                )
    >
                0.0000001
                )
            {
                const float fullCableLength =
                    glm::length(
                        cableEnd -
                        cableStart
                    );

                const float cabinPathFraction =
                    CABLE_DOCK_T_B -
                    CABLE_DOCK_T_A;

                const float cabinPathLength =
                    fullCableLength *
                    cabinPathFraction;

                const float cableLinearSpeed =
                    cabinPathLength *
                    cableSpeed;

                const float pulleyAngularSpeedRadians =
                    cableLinearSpeed /
                    PULLEY_RADIUS_WORLD;

                const float pulleyAngularSpeedDegrees =
                    glm::degrees(
                        pulleyAngularSpeedRadians
                    );

                pulleyAngle +=
                    movementDirection
                    *
                    pulleyAngularSpeedDegrees
                    *
                    static_cast<float>(
                        FIXED_ANIMATION_STEP
                        );
            }
        }
        else
        {
            // No interpolation drift while docked.
            previousCableT =
                cableT;

            previousPulleyAngle =
                pulleyAngle;
        }

        animationAccumulator -=
            FIXED_ANIMATION_STEP;
    }
}

double getRenderedCableT()
{
    if (!cableMoving)
        return cableT;

    const double alpha =
        std::clamp(
            animationAccumulator /
            FIXED_ANIMATION_STEP,
            0.0,
            1.0
        );

    return
        previousCableT
        +
        (
            cableT -
            previousCableT
            )
        *
        alpha;
}


float getRenderedPulleyAngle()
{
    if (!cableMoving)
        return pulleyAngle;

    const float alpha =
        static_cast<float>(
            std::clamp(
                animationAccumulator /
                FIXED_ANIMATION_STEP,
                0.0,
                1.0
            )
            );

    return
        previousPulleyAngle
        +
        (
            pulleyAngle -
            previousPulleyAngle
            )
        *
        alpha;
}


glm::vec3 getCableCarPosition()
{
    // Smooth render position between fixed physics states.
    const double renderCableT =
        getRenderedCableT();

    const double lineTDouble =
        static_cast<double>(
            CABLE_DOCK_T_A
            )
        +
        renderCableT
        *
        static_cast<double>(
            CABLE_DOCK_T_B -
            CABLE_DOCK_T_A
            );

    const float lineT =
        static_cast<float>(
            lineTDouble
            );

    const glm::vec3 cablePoint =
        cableStart
        +
        lineT
        *
        (cableEnd - cableStart);

    return
        cablePoint
        +
        glm::vec3(
            0.0f,
            -CABIN_DROP_FROM_CABLE,
            0.0f
        );
}

glm::mat4 getCableCarParentMatrix()
{
    // Exactly the same parent transform concept as the cabin:
    // TranslationAlongCable * LocalYRotation

    glm::mat4 cabinParent(1.0f);

    cabinParent =
        glm::translate(
            cabinParent,
            getCableCarPosition()
        );

    cabinParent =
        glm::rotate(
            cabinParent,
            glm::radians(cabinYaw),
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );

    return cabinParent;
}


glm::mat4 getCableCarCameraView()
{
    // ========================================================
    // FRONT CAMERA
    //
    // Cabin local +X is treated as the front/travel-facing side.
    //
    // Camera is placed just outside the front wall so the solid
    // window geometry cannot block the view.
    //
    // No independent yaw/pitch is used here.
    // Q/E rotates the cabin parent, therefore this camera rotates
    // naturally with the cable car.
    // ========================================================

    const glm::mat4 cabinParent =
        getCableCarParentMatrix();

    const glm::vec3 localCameraPosition(
        1.03f,
        0.18f,
        0.20f
    );

    const glm::vec3 localForward(
        1.0f,
        0.0f,
        0.0f
    );

    const glm::vec3 localUp(
        0.0f,
        1.0f,
        0.0f
    );

    const glm::vec3 worldCameraPosition =
        glm::vec3(
            cabinParent *
            glm::vec4(
                localCameraPosition,
                1.0f
            )
        );

    const glm::vec3 worldForward =
        glm::normalize(
            glm::mat3(cabinParent) *
            localForward
        );

    const glm::vec3 worldUp =
        glm::normalize(
            glm::mat3(cabinParent) *
            localUp
        );

    return
        glm::lookAt(
            worldCameraPosition,
            worldCameraPosition + worldForward,
            worldUp
        );
}


// ============================================================
// SHIP COLLISION
// ============================================================

bool circleIntersectsRectangle(
    const glm::vec2& center,
    float radius,
    const RectObstacle& rect)
{
    float closestX =
        std::clamp(
            center.x,
            rect.minX,
            rect.maxX
        );

    float closestZ =
        std::clamp(
            center.y,
            rect.minZ,
            rect.maxZ
        );

    float dx =
        center.x -
        closestX;

    float dz =
        center.y -
        closestZ;

    return
        dx * dx +
        dz * dz
        <
        radius * radius;
}


bool shipPositionIsValid(
    const glm::vec3& candidate)
{
    // --------------------------------------------------------
    // 1. WATER BOUNDARY
    // The whole ship must stay inside the water plane.
    // --------------------------------------------------------

    if (
        candidate.x <
        -WATER_HALF_X +
        SHIP_COLLISION_RADIUS
        ||
        candidate.x >
        WATER_HALF_X -
        SHIP_COLLISION_RADIUS
        ||
        candidate.z <
        -WATER_HALF_Z +
        SHIP_COLLISION_RADIUS
        ||
        candidate.z >
        WATER_HALF_Z -
        SHIP_COLLISION_RADIUS
        )
    {
        return false;
    }

    glm::vec2 shipXZ(
        candidate.x,
        candidate.z
    );

    // --------------------------------------------------------
    // 2. ISLANDS
    // --------------------------------------------------------

    if (
        circleIntersectsRectangle(
            shipXZ,
            SHIP_COLLISION_RADIUS,
            LEFT_ISLAND
        )
        )
    {
        return false;
    }

    if (
        circleIntersectsRectangle(
            shipXZ,
            SHIP_COLLISION_RADIUS,
            RIGHT_ISLAND
        )
        )
    {
        return false;
    }

    // --------------------------------------------------------
    // 3. STATIONS
    //
    // Their platforms slightly extend toward the water,
    // so they need explicit collision areas.
    // --------------------------------------------------------

    if (
        circleIntersectsRectangle(
            shipXZ,
            SHIP_COLLISION_RADIUS,
            STATION_A_OBSTACLE
        )
        )
    {
        return false;
    }

    if (
        circleIntersectsRectangle(
            shipXZ,
            SHIP_COLLISION_RADIUS,
            STATION_B_OBSTACLE
        )
        )
    {
        return false;
    }

    // --------------------------------------------------------
    // 4. MOUNTAINS
    // --------------------------------------------------------

    for (
        const CircleObstacle& mountain :
        MOUNTAIN_OBSTACLES
        )
    {
        float safeDistance =
            mountain.radius +
            SHIP_COLLISION_RADIUS;

        float actualDistance =
            glm::length(
                shipXZ -
                mountain.center
            );

        if (
            actualDistance <
            safeDistance
            )
        {
            return false;
        }
    }

    return true;
}


void tryMoveShip(
    float signedDistance)
{
    float yawRadians =
        glm::radians(
            shipYaw
        );

    // Local forward direction.
    glm::vec3 direction(
        std::sin(yawRadians),
        0.0f,
        std::cos(yawRadians)
    );

    glm::vec3 candidate =
        shipPosition
        +
        direction *
        signedDistance;

    // Only commit movement after collision test succeeds.
    if (
        shipPositionIsValid(candidate)
        )
    {
        shipPosition =
            candidate;
    }
}


// ============================================================
// SHADER HELPERS
// ============================================================

unsigned int createShaderProgram(
    const char* vertexSource,
    const char* fragmentSource)
{
    unsigned int vertexShader =
        glCreateShader(
            GL_VERTEX_SHADER
        );

    glShaderSource(
        vertexShader,
        1,
        &vertexSource,
        nullptr
    );

    glCompileShader(
        vertexShader
    );

    checkShaderCompile(
        vertexShader,
        "VERTEX SHADER"
    );

    unsigned int fragmentShader =
        glCreateShader(
            GL_FRAGMENT_SHADER
        );

    glShaderSource(
        fragmentShader,
        1,
        &fragmentSource,
        nullptr
    );

    glCompileShader(
        fragmentShader
    );

    checkShaderCompile(
        fragmentShader,
        "FRAGMENT SHADER"
    );

    unsigned int program =
        glCreateProgram();

    glAttachShader(
        program,
        vertexShader
    );

    glAttachShader(
        program,
        fragmentShader
    );

    glLinkProgram(
        program
    );

    checkProgramLink(
        program
    );

    glDeleteShader(
        vertexShader
    );

    glDeleteShader(
        fragmentShader
    );

    return program;
}


void checkShaderCompile(
    unsigned int shader,
    const char* name)
{
    int success = 0;
    char infoLog[1024];

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &success
    );

    if (!success)
    {
        glGetShaderInfoLog(
            shader,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        std::cout
            << "ERROR: "
            << name
            << " failed to compile:\n";

        std::cout
            << infoLog
            << "\n";
    }
}


void checkProgramLink(
    unsigned int program)
{
    int success = 0;
    char infoLog[1024];

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &success
    );

    if (!success)
    {
        glGetProgramInfoLog(
            program,
            sizeof(infoLog),
            nullptr,
            infoLog
        );

        std::cout
            << "ERROR: SHADER PROGRAM failed to link:\n";

        std::cout
            << infoLog
            << "\n";
    }
}


// ============================================================
// PRIMITIVE MESH CREATION
// ============================================================

Mesh createMesh(
    const std::vector<float>& vertices)
{
    Mesh mesh;

    mesh.vertexCount =
        static_cast<int>(
            vertices.size() / 3
            );

    glGenVertexArrays(
        1,
        &mesh.VAO
    );

    glGenBuffers(
        1,
        &mesh.VBO
    );

    glBindVertexArray(
        mesh.VAO
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        mesh.VBO
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() *
            sizeof(float)
            ),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        reinterpret_cast<void*>(0)
    );

    glEnableVertexAttribArray(
        0
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    glBindVertexArray(
        0
    );

    return mesh;
}


Mesh createCubeMesh()
{
    std::vector<float> vertices =
    {
        // Back
        -0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,   0.5f,-0.5f,-0.5f,
         0.5f, 0.5f,-0.5f,  -0.5f,-0.5f,-0.5f,  -0.5f, 0.5f,-0.5f,

         // Front
         -0.5f,-0.5f, 0.5f,   0.5f,-0.5f, 0.5f,   0.5f, 0.5f, 0.5f,
          0.5f, 0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,  -0.5f,-0.5f, 0.5f,

          // Left
          -0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,  -0.5f,-0.5f,-0.5f,
          -0.5f,-0.5f,-0.5f,  -0.5f,-0.5f, 0.5f,  -0.5f, 0.5f, 0.5f,

          // Right
           0.5f, 0.5f, 0.5f,   0.5f,-0.5f,-0.5f,   0.5f, 0.5f,-0.5f,
           0.5f,-0.5f,-0.5f,   0.5f, 0.5f, 0.5f,   0.5f,-0.5f, 0.5f,

           // Bottom
           -0.5f,-0.5f,-0.5f,   0.5f,-0.5f,-0.5f,   0.5f,-0.5f, 0.5f,
            0.5f,-0.5f, 0.5f,  -0.5f,-0.5f, 0.5f,  -0.5f,-0.5f,-0.5f,

            // Top
            -0.5f, 0.5f,-0.5f,   0.5f, 0.5f, 0.5f,   0.5f, 0.5f,-0.5f,
             0.5f, 0.5f, 0.5f,  -0.5f, 0.5f,-0.5f,  -0.5f, 0.5f, 0.5f
    };

    return createMesh(
        vertices
    );
}


Mesh createPlaneMesh()
{
    std::vector<float> vertices =
    {
        -0.5f, 0.0f,-0.5f,
         0.5f, 0.0f,-0.5f,
         0.5f, 0.0f, 0.5f,

         0.5f, 0.0f, 0.5f,
        -0.5f, 0.0f, 0.5f,
        -0.5f, 0.0f,-0.5f
    };

    return createMesh(
        vertices
    );
}


Mesh createCylinderMesh(
    int segments)
{
    // Cylinder axis = local Y.
    // Radius = 0.5, height = 1.

    std::vector<float> vertices;

    const float yBottom = -0.5f;
    const float yTop = 0.5f;
    const float radius = 0.5f;

    for (
        int i = 0;
        i < segments;
        ++i
        )
    {
        float a0 =
            2.0f *
            PI *
            static_cast<float>(i) /
            static_cast<float>(segments);

        float a1 =
            2.0f *
            PI *
            static_cast<float>(i + 1) /
            static_cast<float>(segments);

        float x0 =
            radius *
            std::cos(a0);

        float z0 =
            radius *
            std::sin(a0);

        float x1 =
            radius *
            std::cos(a1);

        float z1 =
            radius *
            std::sin(a1);

        // Side
        vertices.insert(
            vertices.end(),
            {
                x0,yBottom,z0,
                x1,yBottom,z1,
                x1,yTop,z1
            }
        );

        vertices.insert(
            vertices.end(),
            {
                x1,yTop,z1,
                x0,yTop,z0,
                x0,yBottom,z0
            }
        );

        // Top
        vertices.insert(
            vertices.end(),
            {
                0.0f,yTop,0.0f,
                x1,yTop,z1,
                x0,yTop,z0
            }
        );

        // Bottom
        vertices.insert(
            vertices.end(),
            {
                0.0f,yBottom,0.0f,
                x0,yBottom,z0,
                x1,yBottom,z1
            }
        );
    }

    return createMesh(
        vertices
    );
}


Mesh createConeMesh(
    int segments)
{
    // Cone base y = -0.5
    // Cone apex y = +0.5
    // Base radius = 0.5

    std::vector<float> vertices;

    const float baseY = -0.5f;
    const float apexY = 0.5f;
    const float radius = 0.5f;

    for (
        int i = 0;
        i < segments;
        ++i
        )
    {
        float a0 =
            2.0f *
            PI *
            static_cast<float>(i) /
            static_cast<float>(segments);

        float a1 =
            2.0f *
            PI *
            static_cast<float>(i + 1) /
            static_cast<float>(segments);

        float x0 =
            radius *
            std::cos(a0);

        float z0 =
            radius *
            std::sin(a0);

        float x1 =
            radius *
            std::cos(a1);

        float z1 =
            radius *
            std::sin(a1);

        // Side
        vertices.insert(
            vertices.end(),
            {
                0.0f,apexY,0.0f,
                x0,baseY,z0,
                x1,baseY,z1
            }
        );

        // Base
        vertices.insert(
            vertices.end(),
            {
                0.0f,baseY,0.0f,
                x1,baseY,z1,
                x0,baseY,z0
            }
        );
    }

    return createMesh(
        vertices
    );
}


void deleteMesh(
    Mesh& mesh)
{
    if (
        mesh.VAO != 0
        )
    {
        glDeleteVertexArrays(
            1,
            &mesh.VAO
        );
    }

    if (
        mesh.VBO != 0
        )
    {
        glDeleteBuffers(
            1,
            &mesh.VBO
        );
    }

    mesh =
        Mesh();
}


// ============================================================
// GENERIC DRAWING FUNCTIONS
// ============================================================

void drawMesh(
    const Mesh& mesh,
    const glm::mat4& model,
    const glm::vec3& color)
{
    glUniformMatrix4fv(
        modelLoc,
        1,
        GL_FALSE,
        glm::value_ptr(model)
    );

    glUniform3fv(
        colorLoc,
        1,
        glm::value_ptr(color)
    );

    glBindVertexArray(
        mesh.VAO
    );

    glDrawArrays(
        GL_TRIANGLES,
        0,
        mesh.vertexCount
    );

    glBindVertexArray(
        0
    );
}


void drawCube(
    const glm::mat4& model,
    const glm::vec3& color)
{
    drawMesh(
        cubeMesh,
        model,
        color
    );
}


void drawPlane(
    const glm::mat4& model,
    const glm::vec3& color)
{
    drawMesh(
        planeMesh,
        model,
        color
    );
}


void drawCylinder(
    const glm::mat4& model,
    const glm::vec3& color)
{
    drawMesh(
        cylinderMesh,
        model,
        color
    );
}


void drawCone(
    const glm::mat4& model,
    const glm::vec3& color)
{
    drawMesh(
        coneMesh,
        model,
        color
    );
}


// ============================================================
// WATER / ISLANDS
// ============================================================

void drawWater()
{
    // Main water surface.
    glm::mat4 water(1.0f);

    water =
        glm::translate(
            water,
            glm::vec3(
                0.0f,
                0.0f,
                0.0f
            )
        );

    water =
        glm::scale(
            water,
            glm::vec3(
                WATER_HALF_X * 2.0f,
                1.0f,
                WATER_HALF_Z * 2.0f
            )
        );

    drawPlane(
        water,
        WATER_COLOR
    );

    // A second slightly-lower plane gives a clean water border
    // when viewed from a low camera angle.
    glm::mat4 lowerWater(1.0f);

    lowerWater =
        glm::translate(
            lowerWater,
            glm::vec3(
                0.0f,
                -0.07f,
                0.0f
            )
        );

    lowerWater =
        glm::scale(
            lowerWater,
            glm::vec3(
                WATER_HALF_X * 2.05f,
                1.0f,
                WATER_HALF_Z * 2.05f
            )
        );

    drawPlane(
        lowerWater,
        WATER_EDGE_COLOR
    );
}


void drawIslands()
{
    // ========================================================
    // TWO SIMPLE ISLAND PLATFORMS
    // Brown soil cuboid + thin green grass cuboid.
    // ========================================================

    // LEFT SOIL
    glm::mat4 leftSoil(1.0f);

    leftSoil =
        glm::translate(
            leftSoil,
            glm::vec3(
                -14.2f,
                0.18f,
                0.0f
            )
        );

    leftSoil =
        glm::scale(
            leftSoil,
            glm::vec3(
                11.6f,
                0.36f,
                10.6f
            )
        );

    drawCube(
        leftSoil,
        ISLAND_EDGE
    );

    // RIGHT SOIL
    glm::mat4 rightSoil(1.0f);

    rightSoil =
        glm::translate(
            rightSoil,
            glm::vec3(
                14.2f,
                0.18f,
                0.0f
            )
        );

    rightSoil =
        glm::scale(
            rightSoil,
            glm::vec3(
                11.6f,
                0.36f,
                10.6f
            )
        );

    drawCube(
        rightSoil,
        ISLAND_EDGE
    );

    // LEFT GRASS
    glm::mat4 leftGrass(1.0f);

    leftGrass =
        glm::translate(
            leftGrass,
            glm::vec3(
                -14.2f,
                0.39f,
                0.0f
            )
        );

    leftGrass =
        glm::scale(
            leftGrass,
            glm::vec3(
                11.4f,
                0.10f,
                10.4f
            )
        );

    drawCube(
        leftGrass,
        ISLAND_COLOR
    );

    // RIGHT GRASS
    glm::mat4 rightGrass(1.0f);

    rightGrass =
        glm::translate(
            rightGrass,
            glm::vec3(
                14.2f,
                0.39f,
                0.0f
            )
        );

    rightGrass =
        glm::scale(
            rightGrass,
            glm::vec3(
                11.4f,
                0.10f,
                10.4f
            )
        );

    drawCube(
        rightGrass,
        ISLAND_COLOR
    );
}


// ============================================================
// MOUNTAINS
// ============================================================

void drawMountain(
    const glm::vec3& basePosition,
    const glm::vec3& scale,
    float yRotation,
    const glm::vec3& color)
{
    // Cone local base = -0.5 and local top = +0.5.
    // Translation by scale.y/2 makes its base sit on basePosition.y.

    glm::mat4 model(1.0f);

    model =
        glm::translate(
            model,
            basePosition
            +
            glm::vec3(
                0.0f,
                scale.y * 0.5f,
                0.0f
            )
        );

    model =
        glm::rotate(
            model,
            glm::radians(yRotation),
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );

    model =
        glm::scale(
            model,
            scale
        );

    drawCone(
        model,
        color
    );
}


float mountainSurfaceY(
    const glm::vec3& mountainBase,
    const glm::vec3& mountainScale,
    float worldX,
    float worldZ)
{
    // Approximate cone surface height.
    // Enough for clean vertical tree placement on low-poly slopes.

    float dx =
        worldX -
        mountainBase.x;

    float dz =
        worldZ -
        mountainBase.z;

    float radialDistance =
        std::sqrt(
            dx * dx +
            dz * dz
        );

    float radius =
        0.5f *
        std::max(
            mountainScale.x,
            mountainScale.z
        );

    float factor =
        std::clamp(
            1.0f -
            radialDistance / radius,
            0.0f,
            1.0f
        );

    return
        mountainBase.y
        +
        mountainScale.y *
        factor;
}


// ============================================================
// TREES
// ============================================================

void drawTree(
    const glm::vec3& position,
    float scale,
    float yRotation)
{
    // ========================================================
    // SIMPLE REUSABLE TREE
    //
    // The whole tree is made from only TWO primitive types:
    //
    // 1. Trunk       -> one CYLINDER
    // 2. Leaves      -> two CONES placed one above another
    //
    // The same tree model is reused throughout the scene.
    // Only its translation, Y rotation and scale are changed.
    //
    // TreeParent = Translation * RotationY * Scaling
    // ========================================================

    glm::mat4 treeParent(1.0f);

    treeParent =
        glm::translate(
            treeParent,
            position
        );

    treeParent =
        glm::rotate(
            treeParent,
            glm::radians(yRotation),
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );

    treeParent =
        glm::scale(
            treeParent,
            glm::vec3(scale)
        );

    // Trunk
    glm::mat4 trunk =
        treeParent;

    trunk =
        glm::translate(
            trunk,
            glm::vec3(
                0.0f,
                0.55f,
                0.0f
            )
        );

    trunk =
        glm::scale(
            trunk,
            glm::vec3(
                0.24f,
                1.10f,
                0.24f
            )
        );

    drawCylinder(
        trunk,
        TREE_TRUNK
    );

    // Lower crown
    glm::mat4 leaves1 =
        treeParent;

    leaves1 =
        glm::translate(
            leaves1,
            glm::vec3(
                0.0f,
                1.45f,
                0.0f
            )
        );

    leaves1 =
        glm::scale(
            leaves1,
            glm::vec3(
                1.10f,
                1.65f,
                1.10f
            )
        );

    drawCone(
        leaves1,
        TREE_GREEN
    );

    // Upper crown
    glm::mat4 leaves2 =
        treeParent;

    leaves2 =
        glm::translate(
            leaves2,
            glm::vec3(
                0.0f,
                2.05f,
                0.0f
            )
        );

    leaves2 =
        glm::scale(
            leaves2,
            glm::vec3(
                0.78f,
                1.25f,
                0.78f
            )
        );

    drawCone(
        leaves2,
        TREE_GREEN_2
    );
}


void drawTrees()
{
    // ========================================================
    // REUSABLE TREE PLACEMENT
    //
    // Every tree uses the SAME drawTree() model:
    //   1 cylinder trunk + 2 cone leaf sections.
    //
    // Only Translation, Y Rotation and Scaling change.
    // ========================================================

    const glm::vec3 leftBase(
        -14.2f,
        0.44f,
        -0.7f
    );

    const glm::vec3 leftScale(
        8.2f,
        9.6f,
        7.8f
    );

    const glm::vec3 rightBase(
        14.2f,
        0.44f,
        -0.5f
    );

    const glm::vec3 rightScale(
        8.2f,
        9.7f,
        7.8f
    );

    struct TreePlacement
    {
        float x;
        float z;
        float scale;
        float yaw;
        bool leftMountain;
    };

    const TreePlacement mountainTrees[] =
    {
        // LEFT MAIN MOUNTAIN
        {-16.30f,  0.90f, 0.72f,  16.0f, true },
        {-15.60f, -2.60f, 0.64f, -24.0f, true },
        {-13.70f,  1.25f, 0.68f,  32.0f, true },
        {-12.55f, -1.65f, 0.58f, -18.0f, true },
        {-15.30f,  2.25f, 0.57f,  40.0f, true },
        {-13.10f, -2.35f, 0.60f, -34.0f, true },

        // RIGHT MAIN MOUNTAIN
        { 16.30f,  0.95f, 0.72f, -16.0f, false },
        { 15.60f, -2.60f, 0.64f,  24.0f, false },
        { 13.70f,  1.25f, 0.68f, -32.0f, false },
        { 12.55f, -1.65f, 0.58f,  18.0f, false },
        { 15.30f,  2.25f, 0.57f, -40.0f, false },
        { 13.10f, -2.35f, 0.60f,  34.0f, false }
    };

    for (
        const TreePlacement& tree :
        mountainTrees
        )
    {
        const glm::vec3& base =
            tree.leftMountain
            ? leftBase
            : rightBase;

        const glm::vec3& mountainScale =
            tree.leftMountain
            ? leftScale
            : rightScale;

        float y =
            mountainSurfaceY(
                base,
                mountainScale,
                tree.x,
                tree.z
            );

        drawTree(
            glm::vec3(
                tree.x,
                y,
                tree.z
            ),
            tree.scale,
            tree.yaw
        );
    }

    // --------------------------------------------------------
    // LOWER ISLAND TREES
    // Kept away from station entrances and cable path.
    // --------------------------------------------------------

    drawTree(glm::vec3(-18.6f, 0.49f, 4.20f), 0.70f, 15.0f);
    drawTree(glm::vec3(-18.7f, 0.49f, -4.10f), 0.64f, -18.0f);
    drawTree(glm::vec3(-15.8f, 0.49f, 4.45f), 0.58f, 30.0f);
    drawTree(glm::vec3(-12.1f, 0.49f, 4.25f), 0.54f, -26.0f);

    drawTree(glm::vec3(18.6f, 0.49f, 4.20f), 0.70f, -15.0f);
    drawTree(glm::vec3(18.7f, 0.49f, -4.10f), 0.64f, 18.0f);
    drawTree(glm::vec3(15.8f, 0.49f, 4.45f), 0.58f, -30.0f);
    drawTree(glm::vec3(12.1f, 0.49f, 4.25f), 0.54f, 26.0f);
}

// ============================================================
// STATIONS
// ============================================================

void drawStation(
    const glm::vec3& position,
    bool stationA)
{
    const glm::vec3 accent =
        stationA
        ? STATION_ACCENT_A
        : STATION_ACCENT_B;

    // ========================================================
    // SIMPLE STATION
    //
    // Basic geometric parts only:
    // - 4 lower support columns
    // - 1 platform
    // - 4 upper roof columns
    // - 2 side safety rails
    // - 1 flat roof
    // - 1 small station sign
    //
    // The X direction stays OPEN so the cabin can enter/exit.
    // ========================================================

    const float groundY = 0.48f;

    // --------------------------------------------------------
    // LOWER SUPPORT COLUMNS
    // --------------------------------------------------------

    const float supportHeight =
        std::max(
            position.y - groundY,
            0.5f
        );

    const float supportCenterY =
        groundY +
        supportHeight * 0.5f;

    const float postX = 2.35f;
    const float postZ = 1.42f;

    const glm::vec3 offsets[] =
    {
        glm::vec3(-postX, 0.0f, -postZ),
        glm::vec3(postX, 0.0f, -postZ),
        glm::vec3(-postX, 0.0f,  postZ),
        glm::vec3(postX, 0.0f,  postZ)
    };

    for (
        const glm::vec3& offset :
        offsets
        )
    {
        glm::mat4 support(1.0f);

        support =
            glm::translate(
                support,
                glm::vec3(
                    position.x + offset.x,
                    supportCenterY,
                    position.z + offset.z
                )
            );

        support =
            glm::scale(
                support,
                glm::vec3(
                    0.24f,
                    supportHeight,
                    0.24f
                )
            );

        drawCube(
            support,
            STATION_DARK
        );
    }

    // --------------------------------------------------------
    // MAIN PLATFORM
    // --------------------------------------------------------

    glm::mat4 floor(1.0f);

    floor =
        glm::translate(
            floor,
            position +
            glm::vec3(
                0.0f,
                -0.15f,
                0.0f
            )
        );

    floor =
        glm::scale(
            floor,
            glm::vec3(
                5.80f,
                0.30f,
                3.65f
            )
        );

    drawCube(
        floor,
        STATION_FLOOR
    );

    // --------------------------------------------------------
    // UPPER ROOF COLUMNS
    // Roof underside is position.y + 2.94.
    // --------------------------------------------------------

    const float upperHeight = 2.78f;
    const float upperCenterY =
        position.y +
        upperHeight * 0.5f;

    for (
        const glm::vec3& offset :
        offsets
        )
    {
        glm::mat4 pillar(1.0f);

        pillar =
            glm::translate(
                pillar,
                glm::vec3(
                    position.x + offset.x,
                    upperCenterY,
                    position.z + offset.z
                )
            );

        pillar =
            glm::scale(
                pillar,
                glm::vec3(
                    0.22f,
                    upperHeight,
                    0.22f
                )
            );

        drawCube(
            pillar,
            STATION_WALL
        );
    }

    // --------------------------------------------------------
    // SIMPLE SIDE SAFETY RAILS
    // They are on Z edges, so X path remains open.
    // --------------------------------------------------------

    for (
        float sideZ :
    { -1.62f, 1.62f }
        )
    {
        glm::mat4 rail(1.0f);

        rail =
            glm::translate(
                rail,
                position +
                glm::vec3(
                    0.0f,
                    0.48f,
                    sideZ
                )
            );

        rail =
            glm::scale(
                rail,
                glm::vec3(
                    4.70f,
                    0.36f,
                    0.10f
                )
            );

        drawCube(
            rail,
            STATION_WALL_2
        );
    }

    // --------------------------------------------------------
    // FLAT ROOF
    // Center = position.y + 3.07
    // Thickness = 0.26
    // Under-side = position.y + 2.94
    // --------------------------------------------------------

    glm::mat4 roof(1.0f);

    roof =
        glm::translate(
            roof,
            position +
            glm::vec3(
                0.0f,
                3.07f,
                0.0f
            )
        );

    roof =
        glm::scale(
            roof,
            glm::vec3(
                5.65f,
                0.26f,
                3.80f
            )
        );

    drawCube(
        roof,
        STATION_DARK
    );

    // --------------------------------------------------------
    // SMALL COLOR SIGN
    // --------------------------------------------------------

    glm::mat4 sign(1.0f);

    sign =
        glm::translate(
            sign,
            position +
            glm::vec3(
                0.0f,
                2.72f,
                -1.83f
            )
        );

    sign =
        glm::scale(
            sign,
            glm::vec3(
                1.25f,
                0.22f,
                0.08f
            )
        );

    drawCube(
        sign,
        accent
    );
}


// ============================================================
// CABLE / PULLEYS
// ============================================================

void drawCable()
{
    glm::vec3 direction =
        cableEnd -
        cableStart;

    glm::vec3 midpoint =
        (
            cableStart +
            cableEnd
            )
        *
        0.5f;

    float length =
        glm::length(
            direction
        );

    // Cable is in the XY plane.
    float angleZ =
        std::atan2(
            direction.y,
            direction.x
        );

    glm::mat4 model(1.0f);

    model =
        glm::translate(
            model,
            midpoint
        );

    model =
        glm::rotate(
            model,
            angleZ,
            glm::vec3(
                0.0f,
                0.0f,
                1.0f
            )
        );

    model =
        glm::scale(
            model,
            glm::vec3(
                length,
                0.055f,
                0.055f
            )
        );

    drawCube(
        model,
        CABLE_COLOR
    );
}


void drawPulley(
    const glm::vec3& position,
    float angleDegrees)
{
    // ========================================================
    // CLEAN CEILING-MOUNTED PULLEY
    //
    // Fixed:
    // ceiling plate + two hangers + two dark bearing blocks + axle
    //
    // Rotating:
    // dark wheel + two light spokes
    //
    // Bearing blocks are deliberately placed OUTSIDE the wheel
    // radius so they do not produce overlapping/z-fighting stripes
    // from the cable-car front camera.
    // ========================================================

    // Ceiling plate
    glm::mat4 ceilingPlate(1.0f);

    ceilingPlate =
        glm::translate(
            ceilingPlate,
            position +
            glm::vec3(
                0.0f,
                0.72f,
                0.0f
            )
        );

    ceilingPlate =
        glm::scale(
            ceilingPlate,
            glm::vec3(
                1.95f,
                0.16f,
                0.40f
            )
        );

    drawCube(
        ceilingPlate,
        STATION_WALL_2
    );

    // Hangers are outside the visible wheel radius.
    const float hangerX = 0.78f;

    for (
        float x :
    { -hangerX, hangerX }
        )
    {
        glm::mat4 hanger(1.0f);

        hanger =
            glm::translate(
                hanger,
                position +
                glm::vec3(
                    x,
                    0.37f,
                    -0.18f
                )
            );

        hanger =
            glm::scale(
                hanger,
                glm::vec3(
                    0.14f,
                    0.56f,
                    0.16f
                )
            );

        drawCube(
            hanger,
            STATION_DARK
        );
    }

    // Dark bearing blocks:
    // no bright coplanar contact with the rotating wheel.
    for (
        float x :
    { -hangerX, hangerX }
        )
    {
        glm::mat4 bearing(1.0f);

        bearing =
            glm::translate(
                bearing,
                position +
                glm::vec3(
                    x,
                    0.08f,
                    -0.18f
                )
            );

        bearing =
            glm::scale(
                bearing,
                glm::vec3(
                    0.20f,
                    0.20f,
                    0.18f
                )
            );

        drawCube(
            bearing,
            STATION_DARK
        );
    }

    // Fixed axle
    glm::mat4 axle(1.0f);

    axle =
        glm::translate(
            axle,
            position +
            glm::vec3(
                0.0f,
                0.0f,
                -0.06f
            )
        );

    axle =
        glm::rotate(
            axle,
            glm::radians(90.0f),
            glm::vec3(
                1.0f,
                0.0f,
                0.0f
            )
        );

    axle =
        glm::scale(
            axle,
            glm::vec3(
                0.16f,
                0.76f,
                0.16f
            )
        );

    drawCylinder(
        axle,
        STATION_DARK
    );

    // Rotating parent
    glm::mat4 wheelParent(1.0f);

    wheelParent =
        glm::translate(
            wheelParent,
            position
        );

    wheelParent =
        glm::rotate(
            wheelParent,
            glm::radians(angleDegrees),
            glm::vec3(
                0.0f,
                0.0f,
                1.0f
            )
        );

    // Wheel
    glm::mat4 wheel =
        wheelParent;

    wheel =
        glm::rotate(
            wheel,
            glm::radians(90.0f),
            glm::vec3(
                1.0f,
                0.0f,
                0.0f
            )
        );

    wheel =
        glm::scale(
            wheel,
            glm::vec3(
                1.18f,
                0.22f,
                1.18f
            )
        );

    drawCylinder(
        wheel,
        PULLEY_COLOR
    );

    // Put spokes clearly in front of the wheel surface.
    glm::mat4 spokeParent =
        wheelParent;

    spokeParent =
        glm::translate(
            spokeParent,
            glm::vec3(
                0.0f,
                0.0f,
                0.15f
            )
        );

    glm::mat4 spoke1 =
        spokeParent;

    spoke1 =
        glm::scale(
            spoke1,
            glm::vec3(
                0.98f,
                0.08f,
                0.08f
            )
        );

    drawCube(
        spoke1,
        PULLEY_SPOKE
    );

    glm::mat4 spoke2 =
        spokeParent;

    spoke2 =
        glm::rotate(
            spoke2,
            glm::radians(90.0f),
            glm::vec3(
                0.0f,
                0.0f,
                1.0f
            )
        );

    spoke2 =
        glm::scale(
            spoke2,
            glm::vec3(
                0.98f,
                0.08f,
                0.08f
            )
        );

    drawCube(
        spoke2,
        PULLEY_SPOKE
    );

    // Center cap also sits clearly in front.
    glm::mat4 centerCap(1.0f);

    centerCap =
        glm::translate(
            centerCap,
            position +
            glm::vec3(
                0.0f,
                0.0f,
                0.20f
            )
        );

    centerCap =
        glm::rotate(
            centerCap,
            glm::radians(90.0f),
            glm::vec3(
                1.0f,
                0.0f,
                0.0f
            )
        );

    centerCap =
        glm::scale(
            centerCap,
            glm::vec3(
                0.16f,
                0.10f,
                0.16f
            )
        );

    drawCylinder(
        centerCap,
        PULLEY_SPOKE
    );
}

// ============================================================
// CABLE CAR
// ============================================================

void drawCableCar(
    const glm::vec3& position,
    float yawDegrees)
{
    // ========================================================
    // SIMPLE CABLE CAR
    //
    // Basic geometric parts only:
    // - red lower body
    // - white upper body
    // - dark roof
    // - blue windows
    // - one hanger
    // - one cable grip
    //
    // Parent transformation:
    // TranslationAlongCable * LocalYRotation
    // ========================================================

    glm::mat4 cabinParent(1.0f);

    cabinParent =
        glm::translate(
            cabinParent,
            position
        );

    cabinParent =
        glm::rotate(
            cabinParent,
            glm::radians(yawDegrees),
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );

    // --------------------------------------------------------
    // LOWER BODY
    // --------------------------------------------------------

    glm::mat4 lowerBody =
        cabinParent;

    lowerBody =
        glm::translate(
            lowerBody,
            glm::vec3(
                0.0f,
                -0.25f,
                0.0f
            )
        );

    lowerBody =
        glm::scale(
            lowerBody,
            glm::vec3(
                1.82f,
                0.44f,
                1.08f
            )
        );

    drawCube(
        lowerBody,
        CABIN_RED
    );

    // --------------------------------------------------------
    // UPPER BODY
    // --------------------------------------------------------

    glm::mat4 upperBody =
        cabinParent;

    upperBody =
        glm::translate(
            upperBody,
            glm::vec3(
                0.0f,
                0.16f,
                0.0f
            )
        );

    upperBody =
        glm::scale(
            upperBody,
            glm::vec3(
                1.82f,
                0.50f,
                1.08f
            )
        );

    drawCube(
        upperBody,
        CABIN_WHITE
    );

    // --------------------------------------------------------
    // ROOF
    // --------------------------------------------------------

    glm::mat4 roof =
        cabinParent;

    roof =
        glm::translate(
            roof,
            glm::vec3(
                0.0f,
                0.47f,
                0.0f
            )
        );

    roof =
        glm::scale(
            roof,
            glm::vec3(
                1.98f,
                0.12f,
                1.18f
            )
        );

    drawCube(
        roof,
        CABIN_DARK_RED
    );

    // --------------------------------------------------------
    // FRONT / BACK WINDOWS
    // --------------------------------------------------------

    for (
        float z :
    { -0.558f, 0.558f }
        )
    {
        glm::mat4 window =
            cabinParent;

        window =
            glm::translate(
                window,
                glm::vec3(
                    0.0f,
                    0.17f,
                    z
                )
            );

        window =
            glm::scale(
                window,
                glm::vec3(
                    1.02f,
                    0.30f,
                    0.022f
                )
            );

        drawCube(
            window,
            CABIN_WINDOW
        );
    }

    // --------------------------------------------------------
    // LEFT / RIGHT SIDE WINDOWS
    // --------------------------------------------------------

    for (
        float x :
    { -0.930f, 0.930f }
        )
    {
        glm::mat4 sideWindow =
            cabinParent;

        sideWindow =
            glm::translate(
                sideWindow,
                glm::vec3(
                    x,
                    0.17f,
                    0.0f
                )
            );

        sideWindow =
            glm::scale(
                sideWindow,
                glm::vec3(
                    0.022f,
                    0.30f,
                    0.58f
                )
            );

        drawCube(
            sideWindow,
            CABIN_WINDOW
        );
    }

    // --------------------------------------------------------
    // HANGER
    //
    // Top reaches the cable exactly at +1.35.
    // --------------------------------------------------------

    glm::mat4 hanger =
        cabinParent;

    hanger =
        glm::translate(
            hanger,
            glm::vec3(
                0.0f,
                0.91f,
                0.0f
            )
        );

    hanger =
        glm::scale(
            hanger,
            glm::vec3(
                0.11f,
                0.82f,
                0.11f
            )
        );

    drawCube(
        hanger,
        CABIN_HANGER_COLOR
    );

    // --------------------------------------------------------
    // CABLE GRIP
    // --------------------------------------------------------

    glm::mat4 grip =
        cabinParent;

    grip =
        glm::translate(
            grip,
            glm::vec3(
                0.0f,
                1.30f,
                0.0f
            )
        );

    grip =
        glm::scale(
            grip,
            glm::vec3(
                0.42f,
                0.04f,
                0.14f
            )
        );

    drawCube(
        grip,
        CABIN_HANGER_COLOR
    );
}


// ============================================================
// SHIP
// ============================================================

void drawShip()
{
    // Parent:
    // Translation * Local Yaw Rotation

    glm::mat4 shipParent(1.0f);

    shipParent =
        glm::translate(
            shipParent,
            shipPosition
        );

    shipParent =
        glm::rotate(
            shipParent,
            glm::radians(shipYaw),
            glm::vec3(
                0.0f,
                1.0f,
                0.0f
            )
        );

    // Hull - longer in local Z so facing direction looks natural.
    glm::mat4 hull =
        shipParent;

    hull =
        glm::scale(
            hull,
            glm::vec3(
                1.35f,
                0.42f,
                3.10f
            )
        );

    drawCube(
        hull,
        SHIP_HULL
    );

    // White upper rim.
    glm::mat4 rim =
        shipParent;

    rim =
        glm::translate(
            rim,
            glm::vec3(
                0.0f,
                0.27f,
                -0.05f
            )
        );

    rim =
        glm::scale(
            rim,
            glm::vec3(
                1.22f,
                0.12f,
                2.70f
            )
        );

    drawCube(
        rim,
        SHIP_WHITE
    );

    // Raised bow (+Z = forward).
    glm::mat4 bow =
        shipParent;

    bow =
        glm::translate(
            bow,
            glm::vec3(
                0.0f,
                0.43f,
                0.93f
            )
        );

    bow =
        glm::scale(
            bow,
            glm::vec3(
                1.05f,
                0.22f,
                0.82f
            )
        );

    drawCube(
        bow,
        SHIP_WHITE
    );

    // Cabin toward rear.
    glm::mat4 cabin =
        shipParent;

    cabin =
        glm::translate(
            cabin,
            glm::vec3(
                0.0f,
                0.65f,
                -0.45f
            )
        );

    cabin =
        glm::scale(
            cabin,
            glm::vec3(
                0.92f,
                0.72f,
                0.90f
            )
        );

    drawCube(
        cabin,
        SHIP_WHITE
    );

    // Front cabin window.
    glm::mat4 frontWindow =
        shipParent;

    frontWindow =
        glm::translate(
            frontWindow,
            glm::vec3(
                0.0f,
                0.70f,
                0.015f
            )
        );

    frontWindow =
        glm::scale(
            frontWindow,
            glm::vec3(
                0.60f,
                0.32f,
                0.035f
            )
        );

    drawCube(
        frontWindow,
        SHIP_WINDOW
    );

    // Side windows.
    for (
        float x :
    { -0.476f, 0.476f }
        )
    {
        glm::mat4 sideWindow =
            shipParent;

        sideWindow =
            glm::translate(
                sideWindow,
                glm::vec3(
                    x,
                    0.70f,
                    -0.45f
                )
            );

        sideWindow =
            glm::scale(
                sideWindow,
                glm::vec3(
                    0.035f,
                    0.32f,
                    0.47f
                )
            );

        drawCube(
            sideWindow,
            SHIP_WINDOW
        );
    }

    // Chimney.
    glm::mat4 chimney =
        shipParent;

    chimney =
        glm::translate(
            chimney,
            glm::vec3(
                -0.25f,
                1.25f,
                -0.63f
            )
        );

    chimney =
        glm::scale(
            chimney,
            glm::vec3(
                0.22f,
                0.72f,
                0.22f
            )
        );

    drawCylinder(
        chimney,
        SHIP_RED
    );

    // Mast.
    glm::mat4 mast =
        shipParent;

    mast =
        glm::translate(
            mast,
            glm::vec3(
                0.0f,
                1.17f,
                0.52f
            )
        );

    mast =
        glm::scale(
            mast,
            glm::vec3(
                0.08f,
                1.25f,
                0.08f
            )
        );

    drawCylinder(
        mast,
        SHIP_DARK
    );

    // Small red forward marker.
    glm::mat4 marker =
        shipParent;

    marker =
        glm::translate(
            marker,
            glm::vec3(
                0.0f,
                0.43f,
                1.48f
            )
        );

    marker =
        glm::scale(
            marker,
            glm::vec3(
                0.34f,
                0.22f,
                0.12f
            )
        );

    drawCube(
        marker,
        SHIP_RED
    );
}


// ============================================================
// COMPLETE SCENE
// ============================================================

void drawScene()
{
    // --------------------------------------------------------
    // WATER
    // --------------------------------------------------------
    drawWater();

    // --------------------------------------------------------
    // ISLANDS
    // --------------------------------------------------------
    drawIslands();

    // --------------------------------------------------------
    // LEFT MOUNTAIN GROUP
    // Bigger and farther from the center channel.
    // --------------------------------------------------------

    drawMountain(
        glm::vec3(-14.2f, 0.44f, -0.7f),
        glm::vec3(8.2f, 9.6f, 7.8f),
        18.0f,
        MOUNTAIN_GREEN
    );

    drawMountain(
        glm::vec3(-17.3f, 0.44f, -3.6f),
        glm::vec3(5.8f, 6.9f, 5.6f),
        -15.0f,
        MOUNTAIN_GREEN_2
    );

    drawMountain(
        glm::vec3(-16.4f, 0.44f, 3.8f),
        glm::vec3(5.2f, 6.1f, 5.0f),
        35.0f,
        MOUNTAIN_DARK
    );

    drawMountain(
        glm::vec3(-10.3f, 0.44f, -3.8f),
        glm::vec3(1.55f, 1.45f, 1.45f),
        5.0f,
        ROCK_COLOR
    );

    // --------------------------------------------------------
    // RIGHT MOUNTAIN GROUP
    // --------------------------------------------------------

    drawMountain(
        glm::vec3(14.2f, 0.44f, -0.5f),
        glm::vec3(8.2f, 9.7f, 7.8f),
        -20.0f,
        MOUNTAIN_GREEN
    );

    drawMountain(
        glm::vec3(17.3f, 0.44f, -3.6f),
        glm::vec3(5.8f, 7.0f, 5.6f),
        12.0f,
        MOUNTAIN_GREEN_2
    );

    drawMountain(
        glm::vec3(16.4f, 0.44f, 3.8f),
        glm::vec3(5.2f, 6.2f, 5.0f),
        -30.0f,
        MOUNTAIN_DARK
    );

    drawMountain(
        glm::vec3(10.3f, 0.44f, -3.8f),
        glm::vec3(1.55f, 1.45f, 1.45f),
        -5.0f,
        ROCK_COLOR
    );

    // --------------------------------------------------------
    // TREES
    // --------------------------------------------------------
    drawTrees();

    // --------------------------------------------------------
    // SIMPLE STATIONS
    // --------------------------------------------------------
    drawStation(
        stationAPosition,
        true
    );

    drawStation(
        stationBPosition,
        false
    );

    // --------------------------------------------------------
    // CABLE + SAME-DIRECTION PULLEYS
    // --------------------------------------------------------
    drawCable();

    const float renderedPulleyAngle =
        getRenderedPulleyAngle();

    drawPulley(
        cableStart,
        renderedPulleyAngle
    );

    drawPulley(
        cableEnd,
        renderedPulleyAngle
    );

    // --------------------------------------------------------
    // CABLE CAR
    // --------------------------------------------------------
    drawCableCar(
        getCableCarPosition(),
        cabinYaw
    );

    // --------------------------------------------------------
    // SHIP
    // --------------------------------------------------------
    drawShip();
}
