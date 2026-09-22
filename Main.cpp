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
// - One global/free camera + one fixed top overview camera
// - Two stations
// - One cable car stays fixed at the midpoint of the cable
// - C switches between free camera and top overview camera
// - Two station pulleys remain fixed
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

// Dark evening-style sky so the scene feels deeper and less flat.
const glm::vec3 SKY_COLOR(0.015f, 0.018f, 0.028f);

// Richer water blues with a darker outer border.
const glm::vec3 WATER_COLOR(0.10f, 0.50f, 0.73f);
const glm::vec3 WATER_EDGE_COLOR(0.05f, 0.31f, 0.46f);

// More natural island colors.
const glm::vec3 ISLAND_COLOR(0.28f, 0.44f, 0.20f);
const glm::vec3 ISLAND_EDGE(0.43f, 0.31f, 0.17f);

// Mountain greens with clearer depth variation.
const glm::vec3 MOUNTAIN_GREEN(0.36f, 0.49f, 0.25f);
const glm::vec3 MOUNTAIN_GREEN_2(0.46f, 0.57f, 0.31f);
const glm::vec3 MOUNTAIN_DARK(0.25f, 0.35f, 0.18f);
const glm::vec3 ROCK_COLOR(0.56f, 0.52f, 0.44f);

// Tree palette: darker trunk and richer leaves.
const glm::vec3 TREE_TRUNK(0.40f, 0.23f, 0.10f);
const glm::vec3 TREE_GREEN(0.05f, 0.48f, 0.17f);
const glm::vec3 TREE_GREEN_2(0.03f, 0.62f, 0.22f);

// Station palette: clean concrete/metal look.
const glm::vec3 STATION_WALL(0.86f, 0.87f, 0.85f);
const glm::vec3 STATION_WALL_2(0.70f, 0.72f, 0.74f);
const glm::vec3 STATION_DARK(0.22f, 0.25f, 0.30f);
const glm::vec3 STATION_FLOOR(0.53f, 0.40f, 0.27f);
const glm::vec3 STATION_ACCENT_A(0.78f, 0.14f, 0.11f);
const glm::vec3 STATION_ACCENT_B(0.15f, 0.38f, 0.70f);

// Cable and pulley colors that stay visible on a dark background.
const glm::vec3 CABLE_COLOR(0.86f, 0.88f, 0.90f);
const glm::vec3 PULLEY_COLOR(0.16f, 0.18f, 0.21f);
const glm::vec3 PULLEY_SPOKE(0.74f, 0.76f, 0.78f);

// Cable-car body with stronger but still believable contrast.
const glm::vec3 CABIN_RED(0.84f, 0.10f, 0.08f);
const glm::vec3 CABIN_DARK_RED(0.58f, 0.06f, 0.05f);
const glm::vec3 CABIN_WHITE(0.94f, 0.94f, 0.92f);
const glm::vec3 CABIN_WINDOW(0.22f, 0.60f, 0.78f);
const glm::vec3 CABIN_DARK(0.10f, 0.11f, 0.13f);

// Neutral bright metallic color for the hanger and cable grip.
const glm::vec3 CABIN_HANGER_COLOR(0.74f, 0.76f, 0.80f);


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
// true  -> fixed high overview camera
bool overviewCameraActive = false;

// Fixed overview camera similar to a high isometric/top view.
// It is NOT attached to the cable car.
const glm::vec3 OVERVIEW_CAMERA_POSITION(
    0.0f,
    34.0f,
    31.0f
);

const glm::vec3 OVERVIEW_CAMERA_TARGET(
    0.0f,
    2.2f,
    0.0f
);
// ============================================================
// WATER / LAND LAYOUT
// ============================================================

// Water plane:
// x = [-16, 16]
// z = [-10, 10]
const float WATER_HALF_X = 22.0f;
const float WATER_HALF_Z = 11.5f;

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

// Cable car is fixed at the exact midpoint of the cable.
// Its center hangs below the cable by this amount.
const float CABIN_DROP_FROM_CABLE = 1.35f;

// Pulleys are static because the cable car no longer moves.
float pulleyAngle = 0.0f;


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
    const glm::vec3& position
);

void drawScene();

glm::vec3 getCableCarPosition();
glm::vec3 getCameraFront();


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
    std::cout << "Cable car is fixed at the midpoint of the cable.\n";

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

        if (overviewCameraActive)
        {
            // Fixed high overview camera.
            // It shows the whole scene from above at an angle.
            view =
                glm::lookAt(
                    OVERVIEW_CAMERA_POSITION,
                    OVERVIEW_CAMERA_TARGET,
                    glm::vec3(
                        0.0f,
                        1.0f,
                        0.0f
                    )
                );
        }
        else
        {
            glm::vec3 cameraFront =
                getCameraFront();

            view =
                glm::lookAt(
                    cameraPosition,
                    cameraPosition + cameraFront,
                    glm::vec3(
                        0.0f,
                        1.0f,
                        0.0f
                    )
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
    else if (key == GLFW_KEY_C)
    {
        overviewCameraActive =
            !overviewCameraActive;

        firstMouse = true;

        std::cout
            << "Camera: "
            << (
                overviewCameraActive
                ? "TOP OVERVIEW"
                : "GLOBAL / FREE"
                )
            << "\n";
    }
}

void processContinuousInput(
    GLFWwindow* window)
{
    // Global/free camera controls work only when overview mode is OFF.
    if (!overviewCameraActive)
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

}


// ============================================================
// MOUSE / WINDOW / CAMERA
// ============================================================

void mouseCallback(
    GLFWwindow* window,
    double xpos,
    double ypos)
{
    // Overview camera is fixed.
    // Mouse look works only in global/free camera mode.
    if (overviewCameraActive)
    {
        firstMouse = true;
        return;
    }

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
    std::cout << "  Fixed       : Stays at the midpoint of the cable\n\n";

    std::cout << "CAMERA\n";
    std::cout << "  C           : Global/free camera <-> top overview camera\n";
    std::cout << "  Overview    : Fixed high-angle whole-scene view\n\n";

    std::cout << "GLOBAL / FREE CAMERA\n";
    std::cout << "  W / S       : Forward / backward\n";
    std::cout << "  A / D       : Left / right\n";
    std::cout << "  R / F       : Up / down\n";
    std::cout << "  Arrow Keys  : Look around\n";
    std::cout << "  Right Mouse : Hold + move to look around\n\n";

    std::cout << "  Cursor      : Visible and usable\n";
    std::cout << "  ESC         : Exit\n";
    std::cout << "=============================================\n\n";
}

// ============================================================
// CABLE CAR POSITION
// ============================================================

glm::vec3 getCableCarPosition()
{
    // Exact midpoint of the physical cable.
    const glm::vec3 cableMidpoint =
        (
            cableStart +
            cableEnd
            )
        *
        0.5f;

    // The cabin hangs below the cable.
    return
        cableMidpoint
        +
        glm::vec3(
            0.0f,
            -CABIN_DROP_FROM_CABLE,
            0.0f
        );
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
    /*
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

    */

    /*
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
    */

    /*
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
    */

    // --------------------------------------------------------
    // LOWER ISLAND TREES
    // Kept away from station entrances and cable path.
    // --------------------------------------------------------

    /*
    drawTree(glm::vec3(-18.6f, 0.49f, 4.20f), 0.70f, 15.0f);
    drawTree(glm::vec3(-18.7f, 0.49f, -4.10f), 0.64f, -18.0f);
    drawTree(glm::vec3(-15.8f, 0.49f, 4.45f), 0.58f, 30.0f);
    drawTree(glm::vec3(-12.1f, 0.49f, 4.25f), 0.54f, -26.0f);

    drawTree(glm::vec3(18.6f, 0.49f, 4.20f), 0.70f, -15.0f);
    drawTree(glm::vec3(18.7f, 0.49f, -4.10f), 0.64f, 18.0f);
    drawTree(glm::vec3(15.8f, 0.49f, 4.45f), 0.58f, -30.0f);
    drawTree(glm::vec3(12.1f, 0.49f, 4.25f), 0.54f, 26.0f);
    */

    // ========================================================
    // ACTIVE TREES: ONLY 4 TREES TOTAL
    // No mountain trees.
    // Two larger trees on each island, side by side.
    // Right side uses mirrored positions.
    // ========================================================

    // LEFT ISLAND: two side-by-side trees near the front-inner corner.


    drawTree(
        glm::vec3(
            -12.65f,
            0.49f,
            4.10f
        ),
        0.86f,
        12.0f
    );

    drawTree(
        glm::vec3(
            -11.45f,
            0.49f,
            4.05f
        ),
        0.82f,
        -8.0f
    );

    // RIGHT ISLAND: mirrored trees in the same relative position.
    drawTree(
        glm::vec3(
            12.65f,
            0.49f,
            4.10f
        ),
        0.86f,
        -12.0f
    );

    drawTree(
        glm::vec3(
            11.45f,
            0.49f,
            4.05f
        ),
        0.82f,
        8.0f
    );
}

// ============================================================
// STATIONS
// ============================================================

void drawStation(
    const glm::vec3& position)
{
    // ========================================================
    // SIMPLE STATION
    //
    // Basic geometric parts only:
    // - 4 lower support columns
    // - 1 platform
    // - 4 upper roof columns
    // - 2 side safety rails
    // - 1 flat roof
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

    const float upperHeight = 2.94f;
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
}

// ============================================================
// CABLE CAR
// ============================================================

void drawCableCar(
    const glm::vec3& position)
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
    // TranslationAlongCable only.
    // ========================================================

    glm::mat4 cabinParent(1.0f);

    cabinParent =
        glm::translate(
            cabinParent,
            position
        );

    // Make only the visible cabin body slightly larger.
    // Hanger and cable grip keep their original size/position
    // so the cable connection stays unchanged.
    glm::mat4 bodyParent =
        cabinParent;

    bodyParent =
        glm::scale(
            bodyParent,
            glm::vec3(
                1.25f,
                1.25f,
                1.25f
            )
        );

    // --------------------------------------------------------
    // LOWER BODY
    // --------------------------------------------------------

    glm::mat4 lowerBody =
        bodyParent;

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
        bodyParent;

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
        bodyParent;

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
            bodyParent;

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
            bodyParent;

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
        stationAPosition);

    drawStation(
        stationBPosition
    );

    // --------------------------------------------------------
    // CABLE + STATIC PULLEYS
    // --------------------------------------------------------
    drawCable();

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
        getCableCarPosition()
    );
}
