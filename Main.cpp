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
// - One cable car automatically travels A <-> B
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

const glm::vec3 SKY_COLOR(0.56f, 0.79f, 0.95f);
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

const glm::vec3 CABLE_COLOR(0.07f, 0.08f, 0.09f);
const glm::vec3 PULLEY_COLOR(0.15f, 0.17f, 0.19f);
const glm::vec3 PULLEY_SPOKE(0.69f, 0.71f, 0.72f);

const glm::vec3 CABIN_RED(0.80f, 0.09f, 0.11f);
const glm::vec3 CABIN_DARK_RED(0.55f, 0.05f, 0.07f);
const glm::vec3 CABIN_WHITE(0.92f, 0.93f, 0.92f);
const glm::vec3 CABIN_WINDOW(0.16f, 0.49f, 0.68f);
const glm::vec3 CABIN_DARK(0.11f, 0.13f, 0.15f);

const glm::vec3 SHIP_HULL(0.48f, 0.12f, 0.08f);
const glm::vec3 SHIP_WHITE(0.91f, 0.92f, 0.89f);
const glm::vec3 SHIP_RED(0.78f, 0.12f, 0.10f);
const glm::vec3 SHIP_WINDOW(0.12f, 0.38f, 0.57f);
const glm::vec3 SHIP_DARK(0.10f, 0.12f, 0.14f);


// ============================================================
// GLOBAL CAMERA
// ============================================================

glm::vec3 cameraPosition(0.0f, 9.0f, 25.0f);

float cameraYaw = -90.0f;
float cameraPitch = -17.0f;

float cameraMoveSpeed = 8.0f;
float cameraLookSpeed = 70.0f;
float mouseSensitivity = 0.10f;

bool firstMouse = true;

double lastMouseX = INITIAL_WIDTH / 2.0;
double lastMouseY = INITIAL_HEIGHT / 2.0;


// ============================================================
// WATER / LAND LAYOUT
// ============================================================

// Water plane:
// x = [-16, 16]
// z = [-10, 10]
const float WATER_HALF_X = 16.0f;
const float WATER_HALF_Z = 10.0f;

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
    -14.5f, -5.5f,
    -4.75f, 4.75f
};

const RectObstacle RIGHT_ISLAND =
{
     5.5f, 14.5f,
    -4.75f, 4.75f
};

// Stations extend slightly toward the water.
// They are also collision obstacles for the ship.
const RectObstacle STATION_A_OBSTACLE =
{
    -9.20f, -3.60f,
    -2.25f,  2.25f
};

const RectObstacle STATION_B_OBSTACLE =
{
     3.60f,  9.20f,
    -2.25f,  2.25f
};

// Some mountains extend beyond the rectangular island edge.
// Circular footprints stop the ship from clipping through those slopes.
const std::vector<CircleObstacle> MOUNTAIN_OBSTACLES =
{
    { glm::vec2(-10.5f, -0.5f), 3.25f },
    { glm::vec2(-12.2f, -3.4f), 2.35f },
    { glm::vec2(-11.5f,  3.5f), 2.15f },

    { glm::vec2(10.5f,  0.0f), 3.35f },
    { glm::vec2(12.0f, -3.5f), 2.40f },
    { glm::vec2(11.5f,  3.5f), 2.15f }
};


// ============================================================
// CABLE CAR / STATIONS
// ============================================================

// Station floor centers.
// They intentionally sit near the inner edges of the two mountain areas.
const glm::vec3 stationAPosition(-6.40f, 4.25f, 0.0f);
const glm::vec3 stationBPosition(6.40f, 4.76f, 0.0f);

// Pulley centers / cable endpoints.
// Keep both wheels in the original clear station-centered positions.
const glm::vec3 cableStart(-6.40f, 6.45f, 0.0f);
const glm::vec3 cableEnd(6.40f, 7.25f, 0.0f);

// The cabin does NOT go all the way to either pulley center.
// It docks slightly toward the INNER side of each station.
// This keeps the original wheel appearance but prevents overlap.
//
// Physical cable:
//   t ~= 0.145 -> Station A cabin docking point
//   t ~= 0.855 -> Station B cabin docking point
const float CABLE_DOCK_T_A = 0.145f;
const float CABLE_DOCK_T_B = 0.855f;

// Cabin center hangs below the cable.
const float CABIN_DROP_FROM_CABLE = 1.54f;

// 0 = docked at Station A, 1 = docked at Station B.
float cableT = 0.0f;

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
const float PULLEY_RADIUS_WORLD = 0.625f;


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

        glm::vec3 cameraFront =
            getCameraFront();

        glm::mat4 view =
            glm::lookAt(
                cameraPosition,
                cameraPosition + cameraFront,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

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
        glfwSetWindowShouldClose(window, true);
    }
    else if (key == GLFW_KEY_G)
    {
        // G starts exactly one station-to-station trip.
        // If the cabin is already moving, ignore the key.
        if (!cableMoving)
        {
            cableMoving = true;

            if (cableDirection > 0.0f)
                std::cout << "Cable car departing Station A -> Station B\n";
            else
                std::cout << "Cable car departing Station B -> Station A\n";
        }
    }
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD)
    {
        cableSpeed = std::min(cableSpeed + 0.02f, 0.35f);
        std::cout << "Cable speed: " << cableSpeed << "\n";
    }
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
    {
        cableSpeed = std::max(cableSpeed - 0.02f, 0.03f);
        std::cout << "Cable speed: " << cableSpeed << "\n";
    }
    else if (key == GLFW_KEY_U)
    {
        shipMoveSpeed = std::min(shipMoveSpeed + 0.4f, 7.0f);
        std::cout << "Ship speed: " << shipMoveSpeed << "\n";
    }
    else if (key == GLFW_KEY_O)
    {
        shipMoveSpeed = std::max(shipMoveSpeed - 0.4f, 0.8f);
        std::cout << "Ship speed: " << shipMoveSpeed << "\n";
    }
}

void processContinuousInput(
    GLFWwindow* window)
{
    // --------------------------------------------------------
    // GLOBAL CAMERA MOVEMENT
    // W/S/A/D/R/F
    // --------------------------------------------------------

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

    if (
        glfwGetKey(window, GLFW_KEY_W)
        == GLFW_PRESS
        )
    {
        cameraPosition +=
            front *
            cameraStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_S)
        == GLFW_PRESS
        )
    {
        cameraPosition -=
            front *
            cameraStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_A)
        == GLFW_PRESS
        )
    {
        cameraPosition -=
            right *
            cameraStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_D)
        == GLFW_PRESS
        )
    {
        cameraPosition +=
            right *
            cameraStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_R)
        == GLFW_PRESS
        )
    {
        cameraPosition +=
            worldUp *
            cameraStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_F)
        == GLFW_PRESS
        )
    {
        cameraPosition -=
            worldUp *
            cameraStep;
    }

    // --------------------------------------------------------
    // CAMERA ROTATION - Arrow keys
    // --------------------------------------------------------

    float lookStep =
        cameraLookSpeed *
        deltaTime;

    if (
        glfwGetKey(window, GLFW_KEY_LEFT)
        == GLFW_PRESS
        )
    {
        cameraYaw -=
            lookStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_RIGHT)
        == GLFW_PRESS
        )
    {
        cameraYaw +=
            lookStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_UP)
        == GLFW_PRESS
        )
    {
        cameraPitch +=
            lookStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_DOWN)
        == GLFW_PRESS
        )
    {
        cameraPitch -=
            lookStep;
    }

    cameraPitch =
        std::clamp(
            cameraPitch,
            -89.0f,
            89.0f
        );

    // --------------------------------------------------------
    // CABLE CAR LOCAL Y ROTATION
    // Q / E
    // --------------------------------------------------------

    float cabinStep =
        cabinRotateSpeed *
        deltaTime;

    if (
        glfwGetKey(window, GLFW_KEY_Q)
        == GLFW_PRESS
        )
    {
        cabinYaw +=
            cabinStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_E)
        == GLFW_PRESS
        )
    {
        cabinYaw -=
            cabinStep;
    }

    if (cabinYaw > 360.0f)
        cabinYaw -= 360.0f;

    if (cabinYaw < -360.0f)
        cabinYaw += 360.0f;

    // --------------------------------------------------------
    // SHIP LOCAL ROTATION
    // J / L
    // --------------------------------------------------------

    float shipTurnStep =
        shipRotateSpeed *
        deltaTime;

    if (
        glfwGetKey(window, GLFW_KEY_J)
        == GLFW_PRESS
        )
    {
        shipYaw +=
            shipTurnStep;
    }

    if (
        glfwGetKey(window, GLFW_KEY_L)
        == GLFW_PRESS
        )
    {
        shipYaw -=
            shipTurnStep;
    }

    if (shipYaw > 360.0f)
        shipYaw -= 360.0f;

    if (shipYaw < -360.0f)
        shipYaw += 360.0f;

    // --------------------------------------------------------
    // SHIP FORWARD/BACKWARD
    //
    // We DO NOT directly change shipPosition.
    // tryMoveShip() first checks:
    // 1. water boundary
    // 2. island collision
    // 3. mountain collision
    // 4. station collision
    // --------------------------------------------------------

    float shipStep =
        shipMoveSpeed *
        deltaTime;

    if (
        glfwGetKey(window, GLFW_KEY_I)
        == GLFW_PRESS
        )
    {
        tryMoveShip(
            shipStep
        );
    }

    if (
        glfwGetKey(window, GLFW_KEY_K)
        == GLFW_PRESS
        )
    {
        tryMoveShip(
            -shipStep
        );
    }

    // Ship remains at water height.
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
    // Cursor stays visible and usable.
    // Camera mouse-look is active only while RIGHT mouse button is held.
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_PRESS)
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

    float xOffset = static_cast<float>(xpos - lastMouseX);
    float yOffset = static_cast<float>(lastMouseY - ypos);

    lastMouseX = xpos;
    lastMouseY = ypos;

    xOffset *= mouseSensitivity;
    yOffset *= mouseSensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    cameraPitch = std::clamp(cameraPitch, -89.0f, 89.0f);
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
    std::cout << "  + / -       : Increase / decrease cable speed\n\n";

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
    std::cout << "  Right Mouse : Hold + move mouse to look around\n";
    std::cout << "  Cursor      : Visible and usable\n\n";

    std::cout << "  ESC         : Exit\n";
    std::cout << "=============================================\n\n";
}

// ============================================================
// CABLE CAR ANIMATION
// ============================================================

void updateAnimations()
{
    // If cabin is docked/stopped, both pulley wheels stop too.
    if (!cableMoving)
        return;

    const float movementDirection =
        cableDirection;

    const float previousT =
        cableT;


    // --------------------------------------------------------
    // 1. MOVE CABLE CAR
    //
    // cableSpeed controls how fast t changes.
    // --------------------------------------------------------

    cableT +=
        cableDirection *
        cableSpeed *
        deltaTime;


    // --------------------------------------------------------
    // 2. STOP AT STATION B
    // --------------------------------------------------------

    if (cableT >= 1.0f)
    {
        cableT = 1.0f;

        cableMoving = false;

        // Next G press returns B -> A.
        cableDirection = -1.0f;

        std::cout
            << "Cable car arrived at Station B and stopped. "
            << "Press G to return to Station A.\n";
    }


    // --------------------------------------------------------
    // 3. STOP AT STATION A
    // --------------------------------------------------------

    else if (cableT <= 0.0f)
    {
        cableT = 0.0f;

        cableMoving = false;

        // Next G press travels A -> B.
        cableDirection = 1.0f;

        std::cout
            << "Cable car arrived at Station A and stopped. "
            << "Press G to travel to Station B.\n";
    }


    // --------------------------------------------------------
    // 4. PULLEY ROTATION LINKED TO CABLE-CAR SPEED
    //
    // This is no longer an independent fixed wheel speed.
    //
    // Cabin path length in world units:
    //   |cableEnd - cableStart| * docking fraction
    //
    // Cable-car world speed:
    //   pathLength * cableSpeed
    //
    // Wheel angular speed:
    //   omega = linearSpeed / wheelRadius
    //
    // Therefore if + increases cableSpeed by a certain ratio,
    // pulley speed increases by the SAME ratio.
    // If - decreases cableSpeed, wheel speed decreases too.
    // --------------------------------------------------------

    if (
        std::abs(cableT - previousT)
    >
        0.000001f
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

        // radians / second
        const float pulleyAngularSpeedRadians =
            cableLinearSpeed /
            PULLEY_RADIUS_WORLD;

        // degrees / second
        const float pulleyAngularSpeedDegrees =
            glm::degrees(
                pulleyAngularSpeedRadians
            );

        pulleyAngle +=
            movementDirection *
            pulleyAngularSpeedDegrees *
            deltaTime;

        if (pulleyAngle > 360.0f)
            pulleyAngle -= 360.0f;

        if (pulleyAngle < -360.0f)
            pulleyAngle += 360.0f;
    }
}

glm::vec3 getCableCarPosition()
{
    // The physical cable runs pulley-to-pulley.
    // The cabin uses only the inner docking section of that cable,
    // so the two original large pulley wheels remain visible and
    // the cabin stops before reaching either wheel.
    float lineT =
        CABLE_DOCK_T_A +
        cableT * (CABLE_DOCK_T_B - CABLE_DOCK_T_A);

    // Explicit linear interpolation:
    // P = A + t(B - A)
    glm::vec3 cablePoint =
        cableStart +
        lineT * (cableEnd - cableStart);

    return cablePoint + glm::vec3(0.0f, -CABIN_DROP_FROM_CABLE, 0.0f);
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
    // Brown soil layer.
    glm::mat4 leftSoil(1.0f);

    leftSoil =
        glm::translate(
            leftSoil,
            glm::vec3(
                -10.0f,
                0.18f,
                0.0f
            )
        );

    leftSoil =
        glm::scale(
            leftSoil,
            glm::vec3(
                9.0f,
                0.36f,
                9.5f
            )
        );

    drawCube(
        leftSoil,
        ISLAND_EDGE
    );

    glm::mat4 rightSoil(1.0f);

    rightSoil =
        glm::translate(
            rightSoil,
            glm::vec3(
                10.0f,
                0.18f,
                0.0f
            )
        );

    rightSoil =
        glm::scale(
            rightSoil,
            glm::vec3(
                9.0f,
                0.36f,
                9.5f
            )
        );

    drawCube(
        rightSoil,
        ISLAND_EDGE
    );

    // Green top layer.
    glm::mat4 leftGrass(1.0f);

    leftGrass =
        glm::translate(
            leftGrass,
            glm::vec3(
                -10.0f,
                0.39f,
                0.0f
            )
        );

    leftGrass =
        glm::scale(
            leftGrass,
            glm::vec3(
                8.85f,
                0.10f,
                9.35f
            )
        );

    drawCube(
        leftGrass,
        ISLAND_COLOR
    );

    glm::mat4 rightGrass(1.0f);

    rightGrass =
        glm::translate(
            rightGrass,
            glm::vec3(
                10.0f,
                0.39f,
                0.0f
            )
        );

    rightGrass =
        glm::scale(
            rightGrass,
            glm::vec3(
                8.85f,
                0.10f,
                9.35f
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
    // TREE PLACEMENT
    //
    // All trees call the SAME drawTree() function.
    // We only change:
    // - position   -> Translation
    // - yaw        -> Rotation around Y
    // - size       -> Scaling
    //
    // Trees are kept away from the cable-car path and station
    // entrances so the final scene remains clean.
    // ========================================================


    // --------------------------------------------------------
    // Main mountain definitions.
    // These are used only to calculate an approximate Y height
    // so each tree sits on the mountain slope instead of floating.
    // --------------------------------------------------------

    const glm::vec3 leftBase(
        -10.5f,
        0.44f,
        -0.5f
    );

    const glm::vec3 leftScale(
        6.3f,
        7.2f,
        6.2f
    );

    const glm::vec3 rightBase(
        10.5f,
        0.44f,
        0.0f
    );

    const glm::vec3 rightScale(
        6.5f,
        8.0f,
        6.4f
    );


    struct TreePlacement
    {
        float x;
        float z;
        float scale;
        float yaw;
        bool leftMountain;
    };


    // --------------------------------------------------------
    // TREES ON MOUNTAIN SLOPES
    //
    // 7 trees on the left main mountain
    // 7 trees on the right main mountain
    //
    // Different sizes and Y rotations make the repeated
    // geometric tree model look less repetitive.
    // --------------------------------------------------------

    const TreePlacement mountainTrees[] =
    {
        // LEFT MOUNTAIN
        {-12.50f,  0.90f, 0.70f,  18.0f, true },
        {-11.80f, -2.40f, 0.62f, -22.0f, true },
        { -9.20f,  1.15f, 0.68f,  30.0f, true },
        { -8.80f, -1.65f, 0.58f, -15.0f, true },

        {-11.65f,  1.95f, 0.57f,  42.0f, true },
        {-10.10f, -2.15f, 0.60f, -35.0f, true },
        { -9.75f,  2.05f, 0.55f,  12.0f, true },


        // RIGHT MOUNTAIN
        { 12.45f,  0.95f, 0.72f, -18.0f, false },
        { 11.85f, -2.35f, 0.63f,  24.0f, false },
        {  9.20f,  1.25f, 0.69f, -30.0f, false },
        {  8.80f, -1.70f, 0.58f,  15.0f, false },

        { 11.60f,  2.00f, 0.58f, -42.0f, false },
        { 10.10f, -2.20f, 0.61f,  34.0f, false },
        {  9.80f,  2.10f, 0.56f, -10.0f, false }
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

        // Approximate mountain surface height at this X/Z point.
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
    // LOWER TREES ON LEFT ISLAND
    //
    // Kept near the outer shore and away from Station A.
    // --------------------------------------------------------

    drawTree(
        glm::vec3(
            -13.60f,
            0.49f,
            3.65f
        ),
        0.72f,
        12.0f
    );

    drawTree(
        glm::vec3(
            -13.80f,
            0.49f,
            -3.70f
        ),
        0.66f,
        -20.0f
    );

    drawTree(
        glm::vec3(
            -11.65f,
            0.49f,
            4.05f
        ),
        0.58f,
        32.0f
    );

    drawTree(
        glm::vec3(
            -10.20f,
            0.49f,
            -4.05f
        ),
        0.61f,
        -38.0f
    );

    drawTree(
        glm::vec3(
            -8.35f,
            0.49f,
            4.00f
        ),
        0.54f,
        20.0f
    );


    // --------------------------------------------------------
    // LOWER TREES ON RIGHT ISLAND
    //
    // Mirrored loosely rather than perfectly, so the scene
    // looks natural without becoming crowded.
    // --------------------------------------------------------

    drawTree(
        glm::vec3(
            13.60f,
            0.49f,
            3.65f
        ),
        0.72f,
        -12.0f
    );

    drawTree(
        glm::vec3(
            13.80f,
            0.49f,
            -3.70f
        ),
        0.66f,
        20.0f
    );

    drawTree(
        glm::vec3(
            11.60f,
            0.49f,
            4.05f
        ),
        0.59f,
        -32.0f
    );

    drawTree(
        glm::vec3(
            10.15f,
            0.49f,
            -4.05f
        ),
        0.61f,
        38.0f
    );

    drawTree(
        glm::vec3(
            8.35f,
            0.49f,
            4.00f
        ),
        0.54f,
        -20.0f
    );
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

    // --------------------------------------------------------
    // SUPPORT STRUCTURE
    // --------------------------------------------------------

    float groundY =
        0.48f;

    float supportHeight =
        std::max(
            position.y -
            groundY,
            0.5f
        );

    float supportCenterY =
        groundY +
        supportHeight * 0.5f;

    const float postX = 2.25f;
    const float postZ = 1.55f;

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
    // PLATFORM
    // --------------------------------------------------------

    glm::mat4 floor(1.0f);

    floor =
        glm::translate(
            floor,
            position
            +
            glm::vec3(
                0.0f,
                -0.16f,
                0.0f
            )
        );

    floor =
        glm::scale(
            floor,
            glm::vec3(
                5.30f,
                0.32f,
                4.00f
            )
        );

    drawCube(
        floor,
        STATION_FLOOR
    );

    // Slight raised center guide floor.
    // Cabin passes above it without clipping.
    glm::mat4 centerFloor(1.0f);

    centerFloor =
        glm::translate(
            centerFloor,
            position
            +
            glm::vec3(
                0.0f,
                0.05f,
                0.0f
            )
        );

    centerFloor =
        glm::scale(
            centerFloor,
            glm::vec3(
                2.90f,
                0.10f,
                1.55f
            )
        );

    drawCube(
        centerFloor,
        STATION_WALL_2
    );

    // --------------------------------------------------------
    // UPPER PILLARS
    //
    // IMPORTANT:
    // Station is OPEN on both X sides.
    // Cable direction is along X, therefore cabin clearly
    // enters/exits instead of passing through a wall.
    // --------------------------------------------------------

    const float upperCenterY =
        position.y +
        1.60f;

    const float upperHeight =
        3.18f;

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
                    0.24f,
                    upperHeight,
                    0.24f
                )
            );

        drawCube(
            pillar,
            STATION_WALL
        );
    }

    // --------------------------------------------------------
    // SIDE RAILS - placed along Z edges
    // Do NOT block X-axis cabin route.
    // --------------------------------------------------------

    for (
        float sideZ :
    { -1.84f, 1.84f }
        )
    {
        glm::mat4 lowerRail(1.0f);

        lowerRail =
            glm::translate(
                lowerRail,
                position
                +
                glm::vec3(
                    0.0f,
                    0.48f,
                    sideZ
                )
            );

        lowerRail =
            glm::scale(
                lowerRail,
                glm::vec3(
                    4.45f,
                    0.58f,
                    0.12f
                )
            );

        drawCube(
            lowerRail,
            STATION_WALL_2
        );

        glm::mat4 upperBeam(1.0f);

        upperBeam =
            glm::translate(
                upperBeam,
                position
                +
                glm::vec3(
                    0.0f,
                    2.25f,
                    sideZ
                )
            );

        upperBeam =
            glm::scale(
                upperBeam,
                glm::vec3(
                    4.75f,
                    0.18f,
                    0.18f
                )
            );

        drawCube(
            upperBeam,
            STATION_DARK
        );
    }

    // --------------------------------------------------------
    // ROOF
    // High enough so pulley/cable do not clip into it.
    // --------------------------------------------------------

    glm::mat4 roof(1.0f);

    roof =
        glm::translate(
            roof,
            position
            +
            glm::vec3(
                0.0f,
                3.28f,
                0.0f
            )
        );

    roof =
        glm::scale(
            roof,
            glm::vec3(
                5.45f,
                0.24f,
                4.15f
            )
        );

    drawCube(
        roof,
        STATION_DARK
    );

    // Roof accent strip.
    glm::mat4 accentStrip(1.0f);

    accentStrip =
        glm::translate(
            accentStrip,
            position
            +
            glm::vec3(
                0.0f,
                2.90f,
                -2.02f
            )
        );

    accentStrip =
        glm::scale(
            accentStrip,
            glm::vec3(
                4.65f,
                0.18f,
                0.10f
            )
        );

    drawCube(
        accentStrip,
        accent
    );

    // --------------------------------------------------------
    // SMALL SIDE CONTROL ROOM
    //
    // Kept away from center cable corridor.
    // --------------------------------------------------------

    glm::mat4 controlRoom(1.0f);

    controlRoom =
        glm::translate(
            controlRoom,
            position
            +
            glm::vec3(
                0.0f,
                0.88f,
                1.42f
            )
        );

    controlRoom =
        glm::scale(
            controlRoom,
            glm::vec3(
                1.40f,
                1.35f,
                0.62f
            )
        );

    drawCube(
        controlRoom,
        STATION_WALL
    );

    glm::mat4 controlWindow(1.0f);

    controlWindow =
        glm::translate(
            controlWindow,
            position
            +
            glm::vec3(
                0.0f,
                0.98f,
                1.745f
            )
        );

    controlWindow =
        glm::scale(
            controlWindow,
            glm::vec3(
                0.80f,
                0.45f,
                0.03f
            )
        );

    drawCube(
        controlWindow,
        CABIN_WINDOW
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
    // CEILING-MOUNTED PULLEY ASSEMBLY
    //
    // Everything is made from simple geometric primitives:
    // - ceiling mounting plate  -> cube
    // - two hanging brackets    -> cubes
    // - axle                    -> cylinder
    // - pulley wheel            -> cylinder
    // - cross spokes            -> cubes
    //
    // IMPORTANT:
    // The mounting structure is FIXED.
    // Only wheel + spokes rotate.
    // ========================================================


    // --------------------------------------------------------
    // 1. CEILING MOUNTING PLATE
    //
    // The station roof underside is about 0.76 world-units
    // above the wheel center. This plate overlaps the underside
    // slightly, so it looks tightly bolted to the ceiling.
    // --------------------------------------------------------

    glm::mat4 ceilingPlate(1.0f);

    ceilingPlate =
        glm::translate(
            ceilingPlate,
            position +
            glm::vec3(
                0.0f,
                0.79f,
                0.0f
            )
        );

    ceilingPlate =
        glm::scale(
            ceilingPlate,
            glm::vec3(
                2.10f,
                0.18f,
                0.42f
            )
        );

    drawCube(
        ceilingPlate,
        STATION_DARK
    );


    // --------------------------------------------------------
    // 2. TWO VERTICAL HANGERS
    //
    // They connect directly from the ceiling plate down to the
    // wheel axle, making the mechanical attachment obvious.
    // Kept slightly behind the wheel in Z so the wheel/spokes
    // stay easy to see from the front.
    // --------------------------------------------------------

    const float hangerX = 0.68f;

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
                    0.38f,
                    -0.20f
                )
            );

        hanger =
            glm::scale(
                hanger,
                glm::vec3(
                    0.16f,
                    0.78f,
                    0.20f
                )
            );

        drawCube(
            hanger,
            STATION_DARK
        );
    }


    // --------------------------------------------------------
    // 3. SMALL AXLE-BEARING BLOCKS
    //
    // These make the connection between hangers and axle look
    // tighter instead of the wheel appearing to float.
    // --------------------------------------------------------

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
                    0.0f,
                    -0.08f
                )
            );

        bearing =
            glm::scale(
                bearing,
                glm::vec3(
                    0.30f,
                    0.30f,
                    0.30f
                )
            );

        drawCube(
            bearing,
            PULLEY_SPOKE
        );
    }


    // --------------------------------------------------------
    // 4. FIXED CENTER AXLE
    //
    // Cylinder is created with local Y-axis.
    // Rotate it 90 degrees about X so the axle runs along Z.
    // --------------------------------------------------------

    glm::mat4 axle(1.0f);

    axle =
        glm::translate(
            axle,
            position +
            glm::vec3(
                0.0f,
                0.0f,
                -0.04f
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
                0.24f,
                1.05f,
                0.24f
            )
        );

    drawCylinder(
        axle,
        PULLEY_SPOKE
    );


    // --------------------------------------------------------
    // 5. ROTATING WHEEL PARENT
    //
    // Both wheel and spokes inherit this rotation.
    // --------------------------------------------------------

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


    // --------------------------------------------------------
    // 6. LARGE DARK WHEEL
    //
    // Keep the same simple/clear appearance as before.
    // --------------------------------------------------------

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
                1.25f,
                0.28f,
                1.25f
            )
        );

    drawCylinder(
        wheel,
        PULLEY_COLOR
    );


    // --------------------------------------------------------
    // 7. CROSS SPOKES
    //
    // The spokes clearly show wheel direction/speed.
    // --------------------------------------------------------

    glm::mat4 horizontalSpoke =
        wheelParent;

    horizontalSpoke =
        glm::scale(
            horizontalSpoke,
            glm::vec3(
                1.05f,
                0.09f,
                0.34f
            )
        );

    drawCube(
        horizontalSpoke,
        PULLEY_SPOKE
    );


    glm::mat4 verticalSpoke =
        wheelParent;

    verticalSpoke =
        glm::rotate(
            verticalSpoke,
            glm::radians(90.0f),
            glm::vec3(
                0.0f,
                0.0f,
                1.0f
            )
        );

    verticalSpoke =
        glm::scale(
            verticalSpoke,
            glm::vec3(
                1.05f,
                0.09f,
                0.34f
            )
        );

    drawCube(
        verticalSpoke,
        PULLEY_SPOKE
    );


    // --------------------------------------------------------
    // 8. CENTER CAP
    // --------------------------------------------------------

    glm::mat4 centerCap(1.0f);

    centerCap =
        glm::translate(
            centerCap,
            position +
            glm::vec3(
                0.0f,
                0.0f,
                0.18f
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
                0.23f,
                0.15f,
                0.23f
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
    // MAIN EVALUATION TRANSFORM
    //
    // cabinParent =
    // TranslationAlongCable * LocalRotationAroundCabinY
    //
    // Every cabin part is a child of cabinParent.
    // Therefore Q/E rotates the ENTIRE cabin around its
    // OWN local Y-axis, not around world origin.
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

    // Main lower body.
    glm::mat4 lowerBody =
        cabinParent;

    lowerBody =
        glm::translate(
            lowerBody,
            glm::vec3(
                0.0f,
                -0.34f,
                0.0f
            )
        );

    lowerBody =
        glm::scale(
            lowerBody,
            glm::vec3(
                2.10f,
                0.62f,
                1.38f
            )
        );

    drawCube(
        lowerBody,
        CABIN_RED
    );

    // Upper white frame.
    glm::mat4 upperBody =
        cabinParent;

    upperBody =
        glm::translate(
            upperBody,
            glm::vec3(
                0.0f,
                0.28f,
                0.0f
            )
        );

    upperBody =
        glm::scale(
            upperBody,
            glm::vec3(
                2.10f,
                0.72f,
                1.38f
            )
        );

    drawCube(
        upperBody,
        CABIN_WHITE
    );

    // Roof.
    glm::mat4 roof =
        cabinParent;

    roof =
        glm::translate(
            roof,
            glm::vec3(
                0.0f,
                0.73f,
                0.0f
            )
        );

    roof =
        glm::scale(
            roof,
            glm::vec3(
                2.26f,
                0.16f,
                1.52f
            )
        );

    drawCube(
        roof,
        CABIN_DARK_RED
    );

    // Front and rear windows.
    for (
        float z :
    { -0.701f, 0.701f }
        )
    {
        glm::mat4 window(1.0f);

        window =
            cabinParent;

        window =
            glm::translate(
                window,
                glm::vec3(
                    0.0f,
                    0.30f,
                    z
                )
            );

        window =
            glm::scale(
                window,
                glm::vec3(
                    1.28f,
                    0.43f,
                    0.035f
                )
            );

        drawCube(
            window,
            CABIN_WINDOW
        );
    }

    // Left side window.
    glm::mat4 leftWindow =
        cabinParent;

    leftWindow =
        glm::translate(
            leftWindow,
            glm::vec3(
                -1.066f,
                0.30f,
                0.0f
            )
        );

    leftWindow =
        glm::scale(
            leftWindow,
            glm::vec3(
                0.035f,
                0.43f,
                0.77f
            )
        );

    drawCube(
        leftWindow,
        CABIN_WINDOW
    );

    // Right side door/window.
    glm::mat4 rightWindow =
        cabinParent;

    rightWindow =
        glm::translate(
            rightWindow,
            glm::vec3(
                1.066f,
                0.30f,
                0.0f
            )
        );

    rightWindow =
        glm::scale(
            rightWindow,
            glm::vec3(
                0.035f,
                0.43f,
                0.77f
            )
        );

    drawCube(
        rightWindow,
        CABIN_WINDOW
    );

    // Asymmetric dark door bar makes Y rotation obvious.
    glm::mat4 doorBar =
        cabinParent;

    doorBar =
        glm::translate(
            doorBar,
            glm::vec3(
                1.09f,
                0.04f,
                0.0f
            )
        );

    doorBar =
        glm::scale(
            doorBar,
            glm::vec3(
                0.045f,
                1.05f,
                0.09f
            )
        );

    drawCube(
        doorBar,
        CABIN_DARK
    );

    // Hanger from roof toward cable.
    glm::mat4 hanger =
        cabinParent;

    hanger =
        glm::translate(
            hanger,
            glm::vec3(
                0.0f,
                1.175f,
                0.0f
            )
        );

    hanger =
        glm::scale(
            hanger,
            glm::vec3(
                0.14f,
                0.73f,
                0.14f
            )
        );

    drawCube(
        hanger,
        CABIN_DARK
    );

    // Grip directly beneath cable.
    glm::mat4 grip =
        cabinParent;

    grip =
        glm::translate(
            grip,
            glm::vec3(
                0.0f,
                1.54f,
                0.0f
            )
        );

    grip =
        glm::scale(
            grip,
            glm::vec3(
                0.52f,
                0.10f,
                0.16f
            )
        );

    drawCube(
        grip,
        CABIN_DARK
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
    // ISLAND / SHORE AREAS
    // --------------------------------------------------------

    drawIslands();

    // --------------------------------------------------------
    // LEFT MOUNTAIN GROUP
    // --------------------------------------------------------

    drawMountain(
        glm::vec3(-10.5f, 0.44f, -0.5f),
        glm::vec3(6.3f, 7.2f, 6.2f),
        18.0f,
        MOUNTAIN_GREEN
    );

    drawMountain(
        glm::vec3(-12.2f, 0.44f, -3.4f),
        glm::vec3(4.5f, 5.3f, 4.7f),
        -15.0f,
        MOUNTAIN_GREEN_2
    );

    drawMountain(
        glm::vec3(-11.5f, 0.44f, 3.5f),
        glm::vec3(4.0f, 4.7f, 4.2f),
        35.0f,
        MOUNTAIN_DARK
    );

    // Small exposed rock near left coast.
    drawMountain(
        glm::vec3(-7.0f, 0.44f, -3.55f),
        glm::vec3(1.35f, 1.30f, 1.30f),
        5.0f,
        ROCK_COLOR
    );

    // --------------------------------------------------------
    // RIGHT MOUNTAIN GROUP
    // --------------------------------------------------------

    drawMountain(
        glm::vec3(10.5f, 0.44f, 0.0f),
        glm::vec3(6.5f, 8.0f, 6.4f),
        -20.0f,
        MOUNTAIN_GREEN
    );

    drawMountain(
        glm::vec3(12.0f, 0.44f, -3.5f),
        glm::vec3(4.6f, 5.8f, 4.8f),
        12.0f,
        MOUNTAIN_GREEN_2
    );

    drawMountain(
        glm::vec3(11.5f, 0.44f, 3.5f),
        glm::vec3(4.0f, 5.0f, 4.0f),
        -30.0f,
        MOUNTAIN_DARK
    );

    // Small exposed rock near right coast.
    drawMountain(
        glm::vec3(7.0f, 0.44f, -3.55f),
        glm::vec3(1.35f, 1.30f, 1.30f),
        -5.0f,
        ROCK_COLOR
    );

    // --------------------------------------------------------
    // TREES
    // --------------------------------------------------------

    drawTrees();

    // --------------------------------------------------------
    // STATIONS
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
    // CABLE + PULLEYS
    // --------------------------------------------------------

    drawCable();

    // Both pulley wheels rotate in the SAME visible direction.
    //
    // A -> B:
    //   pulleyAngle increases -> both wheels rotate the same way.
    //
    // B -> A:
    //   pulleyAngle decreases -> both wheels reverse together.
    drawPulley(
        cableStart,
        pulleyAngle
    );

    drawPulley(
        cableEnd,
        pulleyAngle
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
