#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.h"
#include "shader.h"
#include "Object/Mesh.h"
#include "Feature/basic/LightingAndReflection/LightingAndReflection.h"
#include "Feature/basic/LoadModel/LoadModel.h"
#include "Feature/basic/CameraControl/CameraControl.h"
#include "Feature/basic/GameLogic/GameLogic.h"
#include "Feature/basic/MoveObject/MoveObject.h"
#include "Feature/basic/Texture/Texture.h"
#include "Feature/basic/Cubemap/Cubemap.h"
#include "Feature/advanced/Shadow/Shadow.h"
#include "Feature/intermediate/Billboarding/Billboarding.h"

#include <iostream>

// Global variables for light control
bool lightControlMode = false;
glm::vec3 lightPosition = glm::vec3(1.5f, 2.0f, 1.5f);
float lightMoveSpeed = 0.1f;
#include <string>
#include <vector>
#include <cmath>

// Forward declare ChessSquare before globals that use it

#ifndef PATH_TO_OBJECTS
#define PATH_TO_OBJECTS ""
#endif

static int WINDOW_WIDTH = 1000;
static int WINDOW_HEIGHT = 700;

Camera camera(glm::vec3(0.0f, 1.5f, 5.0f));
bool boardFlipped = true;  // Start flipped so initial/top/diagonal face the intended side
bool whitesTurn = true;    // Start with white to move
bool colorsFlipped = false; // keep fixed (no toggling)
bool invertPieceColorsOnce = true; // flip mapping once at startup only
// Diagnostics/toggles
bool computeAllowDuringShift = false; // if false, allowable moves are computed only on SHIFT release

// Track last moved piece per side (index) and its cursor square
int savedSelWhite = -1; ChessSquare savedCursorWhite; 
int savedSelBlack = -1; ChessSquare savedCursorBlack;

// Pending click state for mouse selection on tiles
static bool clickPending = false; static double clickX = 0.0, clickY = 0.0;
// Pending keyboard (SHIFT) target
static ChessSquare pendingTarget; static bool hasPendingTarget = false; static int accumDx = 0, accumDy = 0; static ChessSquare pendingStart;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Scene vertical layout constants
static const float TILE_Y = -0.10f;        // height of the board tiles
static const float PIECE_Y = -0.12f;       // slightly lower so pieces rest on tiles
static const float BOARD_THICKNESS = 0.08f;
static const float BOARD_EPSILON = 0.003f; // gap to avoid z-fighting

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_WIDTH = width; WINDOW_HEIGHT = height;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
}



// Orbit camera with mouse drag
void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    CameraControl::handleMouseMovement(camera, xpos, ypos, CameraControl::isCameraLocked());
}

void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    CameraControl::handleMouseScroll(camera, yoffset, CameraControl::isCameraLocked());
}

        // Mouse button captures for click-to-move
        void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
            if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
                glfwGetCursorPos(window, &clickX, &clickY);
                clickPending = true;
            }
        }

// interaction state
bool dragging = false;
double prevMouseX = 0.0, prevMouseY = 0.0;

std::string inputBuffer;
bool waitingForSecondInput = false;
char firstInput = '\0';

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a real fullscreen window on the primary monitor
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    if (primary && mode) {
        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        WINDOW_WIDTH = mode->width;
        WINDOW_HEIGHT = mode->height;
    }
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Chess", primary, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl; return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Initialize camera control system
    CameraControl::initialize(camera);

    // Initialize texture system
    Texture::initialize();

    // Initialize cubemap for skybox
    std::string cubemapPath = std::string(PATH_TO_SRC) + "/Feature/basic/Cubemap";
    Cubemap skybox(cubemapPath);
    
    // Initialize shadow system
    Shadow::initialize();
    
    // Initialize billboarding system
    Billboarding::initialize();
    
    // Create skybox shader
    const std::string skyboxVS = LightingAndReflection::getSkyboxVertexShader();
    const std::string skyboxFS = LightingAndReflection::getSkyboxFragmentShader();
    Shader skyboxSh(skyboxVS, skyboxFS);

    // Enhanced shaders with reflection and shadow support
    const std::string pieceVS = LightingAndReflection::getEnhancedPieceVertexShader();
    const std::string pieceFS = LightingAndReflection::getEnhancedPieceFragmentShader();
    Shader pieceSh(pieceVS, pieceFS);
    
    // Shadow shaders
    const std::string shadowVS = Shadow::getShadowVertexShader();
    const std::string shadowFS = Shadow::getShadowFragmentShader();
    Shader shadowSh(shadowVS, shadowFS);
    
    // Shadow receiver shaders
    const std::string shadowReceiverVS = Shadow::getShadowReceiverVertexShader();
    const std::string shadowReceiverFS = Shadow::getShadowReceiverFragmentShader();
    Shader shadowReceiverSh(shadowReceiverVS, shadowReceiverFS);
    
    // Billboarding shader
    const std::string billboardVS = Billboarding::getBillboardVertexShader();
    const std::string billboardFS = Billboarding::getBillboardFragmentShader();
    Shader billboardSh(billboardVS, billboardFS);

    // Initialize piece meshes after shader is available
    LoadModel::initializeMeshes(pieceSh);
    
    // Create shadow map framebuffer
    unsigned int shadowMapFBO = Shadow::createShadowMapFBO();

    // Build flat chessboard (8x8 squares) using a dedicated quad VAO (positions + normals)
    GLuint tileVAO = 0, tileVBO = 0;
    {
        // Unit square on XZ plane centered at origin, Y=0, with up normals and texture coordinates
        const float quad[] = {
            // pos                     // texCoord  // normal
            -0.5f, 0.0f, -0.5f,         0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
             0.5f, 0.0f, -0.5f,         1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
             0.5f, 0.0f,  0.5f,         1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
            -0.5f, 0.0f, -0.5f,         0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
             0.5f, 0.0f,  0.5f,         1.0f, 1.0f,  0.0f, 1.0f, 0.0f,
            -0.5f, 0.0f,  0.5f,         0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &tileVAO);
        glGenBuffers(1, &tileVBO);
        glBindVertexArray(tileVAO);
        glBindBuffer(GL_ARRAY_BUFFER, tileVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        // Match piece shader: location 0 = position, location 1 = texCoord, location 2 = normal
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
        glBindVertexArray(0);
    }
    // Board base slab (single cuboid) VAO
    GLuint boardVAO = 0, boardVBO = 0;
    {
        // Unit cube (positions + normals), 36 vertices
        const float cube[] = {
            // +Y (top)
            -0.5f, 0.5f, -0.5f,  0,1,0,   0.5f, 0.5f, -0.5f,  0,1,0,   0.5f, 0.5f,  0.5f,  0,1,0,
            -0.5f, 0.5f, -0.5f,  0,1,0,   0.5f, 0.5f,  0.5f,  0,1,0,  -0.5f, 0.5f,  0.5f,  0,1,0,
            // -Y (bottom)
            -0.5f,-0.5f,  0.5f,  0,-1,0,  0.5f,-0.5f,  0.5f,  0,-1,0,  0.5f,-0.5f, -0.5f, 0,-1,0,
            -0.5f,-0.5f,  0.5f,  0,-1,0,  0.5f,-0.5f, -0.5f, 0,-1,0, -0.5f,-0.5f, -0.5f, 0,-1,0,
            // +X
             0.5f,-0.5f, -0.5f,  1,0,0,   0.5f,-0.5f,  0.5f,  1,0,0,   0.5f, 0.5f,  0.5f,  1,0,0,
             0.5f,-0.5f, -0.5f,  1,0,0,   0.5f, 0.5f,  0.5f,  1,0,0,   0.5f, 0.5f, -0.5f,  1,0,0,
            // -X
            -0.5f,-0.5f,  0.5f, -1,0,0,  -0.5f,-0.5f, -0.5f, -1,0,0,  -0.5f, 0.5f, -0.5f, -1,0,0,
            -0.5f,-0.5f,  0.5f, -1,0,0,  -0.5f, 0.5f, -0.5f, -1,0,0,  -0.5f, 0.5f,  0.5f, -1,0,0,
            // +Z
            -0.5f,-0.5f,  0.5f,  0,0,1,   0.5f,-0.5f,  0.5f,  0,0,1,   0.5f, 0.5f,  0.5f,  0,0,1,
            -0.5f,-0.5f,  0.5f,  0,0,1,   0.5f, 0.5f,  0.5f,  0,0,1,  -0.5f, 0.5f,  0.5f,  0,0,1,
            // -Z
             0.5f,-0.5f, -0.5f,  0,0,-1, -0.5f,-0.5f, -0.5f,  0,0,-1, -0.5f, 0.5f, -0.5f,  0,0,-1,
             0.5f,-0.5f, -0.5f,  0,0,-1, -0.5f, 0.5f, -0.5f,  0,0,-1,  0.5f, 0.5f, -0.5f,  0,0,-1,
        };
        glGenVertexArrays(1, &boardVAO);
        glGenBuffers(1, &boardVBO);
        glBindVertexArray(boardVAO);
        glBindBuffer(GL_ARRAY_BUFFER, boardVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    
    // Skybox cube VAO
    GLuint skyboxVAO = 0, skyboxVBO = 0;
    {
        // Large cube for skybox (positions only)
        const float skyboxVertices[] = {
            // positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
    
    std::vector<glm::mat4> tileModels;
    tileModels.reserve(64);
    for(int z=0; z<8; ++z){
        for(int x=0; x<8; ++x){
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3((x-3.5f)*0.6f, TILE_Y, (z-3.5f)*0.6f));
            m = glm::scale(m, glm::vec3(0.6f, 1.0f, 0.6f)); // square tile spanning the cell
            tileModels.push_back(m);
        }
    }

    // Place pieces with proper chess piece types
    std::vector<Piece> pieces;
    auto placeBackRank = [&](int rowZ, bool isWhite){
        PieceType backRankTypes[] = {PieceType::ROOK, PieceType::KNIGHT, PieceType::BISHOP, PieceType::QUEEN, 
                                    PieceType::KING, PieceType::BISHOP, PieceType::KNIGHT, PieceType::ROOK};
        
        for(int file=0; file<8; ++file){
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3((file-3.5f)*0.6f, PIECE_Y, (rowZ-3.5f)*0.6f));
            m = glm::scale(m, glm::vec3(0.2f));
            PieceType t = backRankTypes[file];
            pieces.push_back(Piece{LoadModel::getMeshFor(t), m, isWhite, file, rowZ, t});
        }
    };
    auto placePawns = [&](int rowZ, bool isWhite){
        for(int file=0; file<8; ++file){
            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3((file-3.5f)*0.6f, PIECE_Y, (rowZ-3.5f)*0.6f));
            m = glm::scale(m, glm::vec3(0.15f));
            pieces.push_back(Piece{LoadModel::getMeshFor(PieceType::PAWN), m, isWhite, file, rowZ, PieceType::PAWN});
        }
    };
    // White (near -z) rows 1 and 2 → z index 1 and 0? Using origin-centered board: use z indices 1 and 0 for near side
    placeBackRank(0, true); placePawns(1, true);
    // Black rows on far side
    placePawns(6, false); placeBackRank(7, false);
    // Start selection/highlight at a2 (white pawn)
    ChessSquare startA2(0,1);
    int selectedPiece = GameLogic::squareToPieceIndex(startA2);
    // Debug: Print initial piece positions
    std::cout << "Initial piece positions:" << std::endl;
    for(size_t i = 0; i < pieces.size(); ++i) {
        std::cout << "Piece " << i << ": (" << pieces[i].file << "," << pieces[i].rank << ") "
                  << (pieces[i].isWhite ? "White" : "Black") << " "
                  << (pieces[i].type == PieceType::PAWN ? "Pawn" : "Other") << std::endl;
    }
    // Current cursor position for arrow key navigation - start at a2
    ChessSquare cursorPos = startA2;
    
    // Initialize light control state
    lightPosition = LightingAndReflection::getLightPosition();
    
    // Create initial greeting message (top-left). Track its index for safe removal.
    static int greetingIndex = -1;
    Billboarding::createMessage("WELCOME TO CHESS!", Billboarding::MessageType::GREETING,
                                glm::vec3(-3.0f, 1.5f, -2.0f), 1.2f);
    greetingIndex = Billboarding::getMessageCount() - 1;
    
    // Timer to remove greeting message after ~10 seconds
    static float greetingTimer = 0.0f;
    static bool greetingRemoved = false;


    // Set up key callback for chess notation and camera modes
    // Note: Using polling instead of callback to avoid lambda capture issues

    // Helper: update a piece's model transform from its board position and type
    auto updatePieceModel = [&](Piece& p){
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3((p.file - 3.5f) * 0.6f, PIECE_Y, (p.rank - 3.5f) * 0.6f));
        float scale = (p.type == PieceType::PAWN) ? 0.15f : 0.2f;
        p.model = glm::scale(m, glm::vec3(scale));
    };

    // Helper: capture enemy piece at target square if present (excluding self index). Returns captured index or -1
    auto captureIfEnemyAt = [&](int targetFile, int targetRank, int selfIndex)->int {
        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (i == selfIndex) continue;
            if (pieces[i].file == targetFile && pieces[i].rank == targetRank && pieces[i].isWhite != pieces[selfIndex].isWhite) {
                GameLogic::markCaptured(pieces, i);
                return i;
            }
        }
        return -1;
    };

    // Helper: after move, flip board orientation, optionally colors, set side to move, and restore selection
    auto finishTurnAndRestoreSelection = [&](bool movedWasWhite, bool flipColors){
        boardFlipped = !boardFlipped;
        if (flipColors) colorsFlipped = !colorsFlipped;
        CameraControl::setCameraMode(camera, CameraControl::getCurrentCameraMode(), boardFlipped);
        whitesTurn = !movedWasWhite;
        if (whitesTurn) {
            if (savedSelWhite >= 0) { selectedPiece = savedSelWhite; cursorPos = savedCursorWhite; }
            else { selectedPiece = GameLogic::squareToPieceIndex(ChessSquare(0,1)); cursorPos = ChessSquare(0,1); }
        } else {
            if (savedSelBlack >= 0) { selectedPiece = savedSelBlack; cursorPos = savedCursorBlack; }
            else { selectedPiece = GameLogic::squareToPieceIndex(ChessSquare(0,6)); cursorPos = ChessSquare(0,6); }
        }
    };

    // Axis geometry (lines)
    GLuint axisVAO=0, axisVBO=0;
    {
        float axis[18] = {
            0,0,0,  1,0,0,
            0,0,0,  0,1,0,
            0,0,0,  0,0,1
        };
        glGenVertexArrays(1,&axisVAO);
        glGenBuffers(1,&axisVBO);
        glBindVertexArray(axisVAO);
        glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(axis), axis, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,(void*)0);
        glBindVertexArray(0);
    }


    // Mouse buttons for dragging selected sphere
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods){
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                dragging = true;
                glfwGetCursorPos(w, &prevMouseX, &prevMouseY);
            } else if (action == GLFW_RELEASE) {
                dragging = false;
            }
        }
    });



    while (!glfwWindowShouldClose(window)) {
        float current = (float)glfwGetTime();
        deltaTime = current - lastFrame; lastFrame = current;
        processInput(window);

        glClearColor(0.65f, 0.80f, 0.95f, 1.0f); // light blue
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // Selection keys (edge triggered)
        
        // Arrow key navigation and movement with optional inversion
        bool shift = MoveObject::getKeyState(window, GLFW_KEY_LEFT_SHIFT) || MoveObject::getKeyState(window, GLFW_KEY_RIGHT_SHIFT);
        
        // Light control mode toggle
        static bool lKeyPressed = false;
        bool lKeyCurrent = MoveObject::getKeyState(window, GLFW_KEY_L);
        if (lKeyCurrent && !lKeyPressed) {
            lightControlMode = !lightControlMode;
            if (lightControlMode) {
                std::cout << "Light control mode: ON - Use WASD to move light, QE for up/down, RF for speed, TG for brightness" << std::endl;
            } else {
                std::cout << "Light control mode: OFF" << std::endl;
            }
        }
        lKeyPressed = lKeyCurrent;
        
        // Light movement controls
        if (lightControlMode) {
            // WASD for horizontal movement
            if (MoveObject::getKeyState(window, GLFW_KEY_W)) lightPosition.z -= lightMoveSpeed;
            if (MoveObject::getKeyState(window, GLFW_KEY_S)) lightPosition.z += lightMoveSpeed;
            if (MoveObject::getKeyState(window, GLFW_KEY_A)) lightPosition.x -= lightMoveSpeed;
            if (MoveObject::getKeyState(window, GLFW_KEY_D)) lightPosition.x += lightMoveSpeed;
            
            // QE for vertical movement
            if (MoveObject::getKeyState(window, GLFW_KEY_Q)) lightPosition.y += lightMoveSpeed;
            if (MoveObject::getKeyState(window, GLFW_KEY_E)) lightPosition.y -= lightMoveSpeed;
            
            // RF for speed control
            if (MoveObject::getKeyState(window, GLFW_KEY_R)) lightMoveSpeed = std::min(lightMoveSpeed + 0.01f, 1.0f);
            if (MoveObject::getKeyState(window, GLFW_KEY_F)) lightMoveSpeed = std::max(lightMoveSpeed - 0.01f, 0.01f);
            
            // TG for brightness control
            static float lightBrightness = 1.0f;
            if (MoveObject::getKeyState(window, GLFW_KEY_T)) lightBrightness = std::min(lightBrightness + 0.05f, 3.0f);
            if (MoveObject::getKeyState(window, GLFW_KEY_G)) lightBrightness = std::max(lightBrightness - 0.05f, 0.1f);
            
            // Update light position and brightness
            LightingAndReflection::setLightPosition(lightPosition);
            LightingAndReflection::setLightBrightness(lightBrightness);
            
            // Print light position every 30 frames (about once per second)
            static int frameCount = 0;
            frameCount++;
            if (frameCount % 30 == 0) {
                std::cout << "Light position: (" << lightPosition.x << ", " << lightPosition.y << ", " << lightPosition.z 
                          << ") Speed: " << lightMoveSpeed << " Brightness: " << lightBrightness << std::endl;
            }
        }

        // Compute moves; if side to move is in check, restrict to only legal moves that resolve check
        std::vector<ChessSquare> allowableMoves;
        if (selectedPiece >= 0 && selectedPiece < (int)pieces.size()) {
            bool inCheck = GameLogic::isSideInCheck(pieces, whitesTurn);
            if (inCheck && pieces[selectedPiece].isWhite == whitesTurn) {
                allowableMoves = GameLogic::getLegalMovesConsideringCheck(selectedPiece, pieces);
            } else {
                allowableMoves = GameLogic::getAllowableMoves(selectedPiece, pieces);
                // Even when not in check, we should not allow moves that expose king; filter
                ChessSquare from(pieces[selectedPiece].file, pieces[selectedPiece].rank);
                allowableMoves.erase(
                    std::remove_if(allowableMoves.begin(), allowableMoves.end(),
                        [&](const ChessSquare& m) {
                            return !GameLogic::wouldBeLegalMove(pieces, selectedPiece, from, m, GameLogic::enPassantAvailable, GameLogic::enPassantSquare);
                        }),
                    allowableMoves.end()
                );
            }
        }

        // Pending target while SHIFT is held
        if (shift) {
            MoveObject::handleShiftMovement(window, boardFlipped, pendingTarget, hasPendingTarget, pendingStart, accumDx, accumDy, pieces, selectedPiece);
        } else {
            // SHIFT released: validate only final pendingTarget
            if (MoveObject::getPrevShift() && hasPendingTarget && selectedPiece >= 0 && selectedPiece < (int)pieces.size()) {
                if (!(pendingTarget.file == pendingStart.file && pendingTarget.rank == pendingStart.rank)) {
                    if (GameLogic::canCommitMove(pieces, selectedPiece, pendingStart, pendingTarget, whitesTurn)) {
                        std::cout << "SHIFT release: start=(" << pendingStart.file << "," << pendingStart.rank << ") final=(" 
                                  << pendingTarget.file << "," << pendingTarget.rank << ") allowed=yes" << std::endl;
                        // Handle capture if enemy present
                        captureIfEnemyAt(pendingTarget.file, pendingTarget.rank, selectedPiece);
                        bool movingWasWhite = pieces[selectedPiece].isWhite;
                        // Commit move
                        // Castling rook move if king moved two squares
                        if (pieces[selectedPiece].type == PieceType::KING && abs(pendingTarget.file - pendingStart.file) == 2) {
                            int rr = pendingStart.rank;
                            if (pendingTarget.file > pendingStart.file) {
                                // king side: move rook from 7 to 5
                                for (int i=0;i<(int)pieces.size();++i) if (pieces[i].type==PieceType::ROOK && pieces[i].isWhite==movingWasWhite && pieces[i].rank==rr && pieces[i].file==7) {
                                    pieces[i].file = 5; pieces[i].hasMoved = true;
                                    updatePieceModel(pieces[i]);
                                    break;
                                }
                            } else {
                                // queen side: move rook from 0 to 3
                                for (int i=0;i<(int)pieces.size();++i) if (pieces[i].type==PieceType::ROOK && pieces[i].isWhite==movingWasWhite && pieces[i].rank==rr && pieces[i].file==0) {
                                    pieces[i].file = 3; pieces[i].hasMoved = true;
                                    updatePieceModel(pieces[i]);
                                    break;
                                }
                            }
                        }
                        // En-passant capture if pawn moved to enPassantSquare
                        if (pieces[selectedPiece].type == PieceType::PAWN && GameLogic::enPassantAvailable && pendingTarget.file == GameLogic::enPassantSquare.file && pendingTarget.rank == GameLogic::enPassantSquare.rank) {
                            int dir = movingWasWhite ? 1 : -1;
                            int victimRank = pendingTarget.rank - dir;
                            for (int i=0;i<(int)pieces.size();++i) if (pieces[i].file==pendingTarget.file && pieces[i].rank==victimRank && pieces[i].type==PieceType::PAWN && pieces[i].isWhite!=movingWasWhite) { GameLogic::markCaptured(pieces, i); break; }
                        }
                        // Update piece
                        pieces[selectedPiece].file = pendingTarget.file;
                        pieces[selectedPiece].rank = pendingTarget.rank;
                        pieces[selectedPiece].hasMoved = true;
                        updatePieceModel(pieces[selectedPiece]);
                        // Pawn promotion (auto-queen)
                        if (pieces[selectedPiece].type == PieceType::PAWN && ((movingWasWhite && pieces[selectedPiece].rank == 7) || (!movingWasWhite && pieces[selectedPiece].rank == 0))) {
                            pieces[selectedPiece].type = PieceType::QUEEN;
                            // Update the mesh to reflect the new piece type
                            pieces[selectedPiece].mesh = LoadModel::getMeshFor(PieceType::QUEEN);
                            // Update model scale for new type
                            updatePieceModel(pieces[selectedPiece]);
                            std::cout << "Pawn promoted to Queen at (" << pieces[selectedPiece].file << ", " << pieces[selectedPiece].rank << ")" << std::endl;
                        }
                        // Update en-passant availability for next move
                        GameLogic::enPassantAvailable = false; GameLogic::enPassantVictimIndex = -1;
                        if (pieces[selectedPiece].type == PieceType::PAWN && abs(pendingTarget.rank - pendingStart.rank) == 2) {
                            GameLogic::enPassantAvailable = true; GameLogic::enPassantVictimIndex = selectedPiece; GameLogic::enPassantSquare = ChessSquare(pendingStart.file, (pendingStart.rank + pendingTarget.rank)/2);
                        }
                        // Finish turn (no color flip here)
                        finishTurnAndRestoreSelection(movingWasWhite, false);
                    }
                }
            }
            hasPendingTarget = false;

            // Normal cursor navigation when SHIFT is not held (selection can browse any side)
            int dx = 0, dy = 0;
            if (MoveObject::checkArrowKeyEdge(window, dx, dy, boardFlipped)) {
                MoveObject::updateCursorPosition(cursorPos, dx, dy, pieces, selectedPiece);
            }
        }
        
        MoveObject::updatePressedStates(
            MoveObject::getKeyState(window, GLFW_KEY_LEFT),
            MoveObject::getKeyState(window, GLFW_KEY_RIGHT),
            MoveObject::getKeyState(window, GLFW_KEY_UP),
            MoveObject::getKeyState(window, GLFW_KEY_DOWN),
            shift
        );

        // Handle Caps Lock for camera lock
        static bool pressedCapsLock = false;
        bool capsLock = glfwGetKey(window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS;
        if (capsLock && !pressedCapsLock) {
            CameraControl::setCameraLocked(!CameraControl::isCameraLocked());
        }
        pressedCapsLock = capsLock;

        // Toggle: allowable moves computation during SHIFT (H)
        static bool pressedH = false;
        bool hk = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS;
        if (hk && !pressedH) {
            computeAllowDuringShift = !computeAllowDuringShift;
            std::cout << "Toggle computeAllowDuringShift=" << (computeAllowDuringShift ? "ON" : "OFF") << std::endl;
        }
        pressedH = hk;

        // Handle chess notation input and camera modes
        static bool pressedTab = false;
        bool tab = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
        if (tab && !pressedTab) {
            // Cycle through camera modes (only 2 now)
            int nextMode = (static_cast<int>(CameraControl::getCurrentCameraMode()) + 1) % 2;
            CameraControl::setCameraMode(camera, static_cast<CameraMode>(nextMode), boardFlipped);
        }
        pressedTab = tab;

        // Compute matrices before drawing
        CameraControl::updateOrbitCamera(camera);
        glm::mat4 V = camera.GetViewMatrix();
        float ratio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;
        glm::mat4 P = camera.GetProjectionMatrix(45.0f, ratio, 0.01f, 100.0f);
        
        // Generate shadow map
        glm::vec3 lightPos = LightingAndReflection::getLightPosition();
        glm::vec3 lightDir = glm::normalize(lightPos - glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = Shadow::getLightSpaceMatrix(lightPos, lightDir);
        
        Shadow::generateShadowMap(shadowMapFBO, lightPos, lightDir);
        shadowSh.use();
        shadowSh.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
        
        // Render scene to shadow map
        for(size_t i = 0; i < pieces.size(); ++i) {
            if (pieces[i].file < 0 || pieces[i].rank < 0) continue;
            shadowSh.setMatrix4("M", pieces[i].model);
            pieces[i].mesh->draw();
        }
        
        // Render board to shadow map
        glm::mat4 shadowBoardModel(1.0f);
        float shadowBoardThickness = BOARD_THICKNESS;
        float shadowBoardSize = 8.0f * 0.6f;
        shadowBoardModel = glm::translate(shadowBoardModel, glm::vec3(0.0f, TILE_Y - BOARD_EPSILON - 0.5f * shadowBoardThickness, 0.0f));
        shadowBoardModel = glm::scale(shadowBoardModel, glm::vec3(shadowBoardSize, shadowBoardThickness, shadowBoardSize));
        shadowSh.setMatrix4("M", shadowBoardModel);
        glBindVertexArray(boardVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        // Render tiles to shadow map
        for(size_t i = 0; i < tileModels.size(); ++i) {
            shadowSh.setMatrix4("M", tileModels[i]);
            glBindVertexArray(tileVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        
        // Switch back to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw skybox first
        if (skybox.isValid()) {
            glDepthFunc(GL_LEQUAL);
            skyboxSh.use();
            skyboxSh.setMatrix4("V", glm::mat4(glm::mat3(V))); // Remove translation from view matrix
            skyboxSh.setMatrix4("P", P);
            skybox.bind(0);
            skyboxSh.setInt("skybox", 0);
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glDepthFunc(GL_LESS);
        }

        // Process pending click for move (all pieces)
        if (clickPending) {
            clickPending = false;
            if (selectedPiece >= 0 && selectedPiece < (int)pieces.size()) {
                const Piece& sel = pieces[selectedPiece];
                // Only allow if respect turn
                if (sel.isWhite == whitesTurn) {
                    float xNdc = (float)((2.0 * clickX) / WINDOW_WIDTH - 1.0);
                    float yNdc = (float)(1.0 - (2.0 * clickY) / WINDOW_HEIGHT);
                    glm::vec4 rayClip(xNdc, yNdc, -1.0f, 1.0f);
                    glm::mat4 invP = glm::inverse(P);
                    glm::vec4 rayEye = invP * rayClip; rayEye.z = -1.0f; rayEye.w = 0.0f;
                    glm::mat4 invV = glm::inverse(V);
                    glm::vec3 rayDir = glm::normalize(glm::vec3(invV * rayEye));
                    glm::vec3 rayOrig = camera.Position;
                    float planeY = -0.3f;
                    if (fabs(rayDir.y) > 1e-5f) {
                        float t = (planeY - rayOrig.y) / rayDir.y;
                        if (t > 0.0f) {
                            glm::vec3 hit = rayOrig + t * rayDir;
                            int file = (int)roundf(hit.x / 0.6f + 3.5f);
                            int rank = (int)roundf(hit.z / 0.6f + 3.5f);
                            file = glm::clamp(file, 0, 7); rank = glm::clamp(rank, 0, 7);
                            // Always require the target to be within the legal set considering checks and king safety
                            bool allowed = false;
                            {
                                std::vector<ChessSquare> moves = GameLogic::getLegalMovesConsideringCheck(selectedPiece, pieces);
                                for (const auto& m : moves) if (m.file == file && m.rank == rank) { allowed = true; break; }
                            }
                            if (allowed) {
                                // Capture if enemy on target (robust)
                                captureIfEnemyAt(file, rank, selectedPiece);
                                pieces[selectedPiece].file = file; pieces[selectedPiece].rank = rank;
                                updatePieceModel(pieces[selectedPiece]);
                                cursorPos = ChessSquare(file, rank);
                                if (sel.isWhite) { savedSelWhite = selectedPiece; savedCursorWhite = cursorPos; }
                                else { savedSelBlack = selectedPiece; savedCursorBlack = cursorPos; }
                                // Finish turn and also flip colors for click-to-move path
                                finishTurnAndRestoreSelection(sel.isWhite, true);
                            }
                        }
                    }
                }
            }
        }

        // Draw board tiles with shadows
        shadowReceiverSh.use();
        shadowReceiverSh.setMatrix4("V", V); 
        shadowReceiverSh.setMatrix4("P", P);
        LightingAndReflection::updatePieceShaderUniforms(shadowReceiverSh.ID, camera.Position);
        Shadow::updateShadowUniforms(shadowReceiverSh.ID, shadowMapFBO, lightSpaceMatrix);
        // Draw board base slab first (dark wood texture)
        glm::mat4 boardModel(1.0f);
        float boardThickness = BOARD_THICKNESS;
        float boardSize = 8.0f * 0.6f; // 4.8
        boardModel = glm::translate(boardModel, glm::vec3(0.0f, TILE_Y - BOARD_EPSILON - 0.5f * boardThickness, 0.0f));
        boardModel = glm::scale(boardModel, glm::vec3(boardSize, boardThickness, boardSize));
        
        // Bind dark wood texture for board base
        Texture::bindTexture(Texture::BOARD_WOOD_DARK, 0);
        shadowReceiverSh.setInt("diffuseTexture", 0);
        shadowReceiverSh.setBool("useTexture", true);
        shadowReceiverSh.setVector3f("baseCol", glm::vec3(0.45f, 0.23f, 0.09f));
        shadowReceiverSh.setMatrix4("M", boardModel);
        shadowReceiverSh.setMatrix4("itM", glm::transpose(glm::inverse(boardModel)));
        shadowReceiverSh.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
        glBindVertexArray(boardVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Conditionally compute allowable moves for visual hints
        if (computeAllowDuringShift) {
            if (selectedPiece >= 0 && selectedPiece < pieces.size()) {
                allowableMoves = GameLogic::getLegalMovesConsideringCheck(selectedPiece, pieces);
            }
        } else {
            allowableMoves.clear();
        }
        for(size_t i=0;i<tileModels.size();++i){
            int tileFile = i % 8; int tileRank = i / 8;
            bool isSelectedSquare = false, isAllowableMove = false, isPending = false, isCapture = false;
            if (selectedPiece < 0 || selectedPiece >= pieces.size()) {
                if (tileFile == cursorPos.file && tileRank == cursorPos.rank) isSelectedSquare = true;
            } else {
                bool squareHasPiece = false; int occupant = -1;
                for(size_t j=0;j<pieces.size();++j) {
                    if (pieces[j].file == tileFile && pieces[j].rank == tileRank) { squareHasPiece = true; occupant = (int)j; break; }
                }
                if (!squareHasPiece && tileFile == cursorPos.file && tileRank == cursorPos.rank) isSelectedSquare = true;
                for (const auto& move : allowableMoves) if (move.file == tileFile && move.rank == tileRank) { isAllowableMove = true; break; }
                // If this allowable square has an enemy on it, mark as capture candidate when pressing SHIFT
                if (isAllowableMove && shift && occupant >= 0 && pieces[occupant].isWhite != pieces[selectedPiece].isWhite) {
                    isCapture = true;
                }
            }
            if (hasPendingTarget && tileFile == pendingTarget.file && tileRank == pendingTarget.rank) isPending = true;
            if (isPending) {
                shadowReceiverSh.setBool("useTexture", false);
                shadowReceiverSh.setVector3f("baseCol", glm::vec3(0.0f, 0.8f, 0.2f)); // green pending
            } else if (isCapture) {
                shadowReceiverSh.setBool("useTexture", false);
                shadowReceiverSh.setVector3f("baseCol", glm::vec3(0.1f, 0.45f, 1.0f)); // blue capture
            } else if (isAllowableMove) {
                shadowReceiverSh.setBool("useTexture", false);
                shadowReceiverSh.setVector3f("baseCol", glm::vec3(1.0f, 0.5f, 0.0f)); // orange
            } else if (isSelectedSquare) {
                shadowReceiverSh.setBool("useTexture", false);
                shadowReceiverSh.setVector3f("baseCol", glm::vec3(1.0f, 0.0f, 0.0f)); // red
            } else {
                // Use wood textures for chess tiles
                bool light = ((i/8 + i%8)%2==0);
                if (light) {
                    Texture::bindTexture(Texture::BOARD_WOOD_LIGHT, 0);
                    shadowReceiverSh.setInt("diffuseTexture", 0);
                    shadowReceiverSh.setBool("useTexture", true);
                    shadowReceiverSh.setVector3f("baseCol", glm::vec3(1.0f, 1.0f, 1.0f)); // White for texture
                } else {
                    Texture::bindTexture(Texture::BOARD_WOOD_DARK, 0);
                    shadowReceiverSh.setInt("diffuseTexture", 0);
                    shadowReceiverSh.setBool("useTexture", true);
                    shadowReceiverSh.setVector3f("baseCol", glm::vec3(1.0f, 1.0f, 1.0f)); // White for texture
                }
            }
            shadowReceiverSh.setMatrix4("M", tileModels[i]);
            shadowReceiverSh.setMatrix4("itM", glm::transpose(glm::inverse(tileModels[i])));
            shadowReceiverSh.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
            glBindVertexArray(tileVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // King-in-check halo overlays: draw a red tile under the king(s) in check
        auto drawCheckHalo = [&](bool whiteKing){
            int k = GameLogic::findKingIndex(pieces, whiteKing);
            if (k < 0) return;
            if (!GameLogic::isSquareAttacked(pieces, pieces[k].file, pieces[k].rank, !whiteKing)) return;
            glm::mat4 halo = glm::translate(glm::mat4(1.0f), glm::vec3((pieces[k].file-3.5f)*0.6f, TILE_Y + 0.001f, (pieces[k].rank-3.5f)*0.6f));
            halo = glm::scale(halo, glm::vec3(0.6f, 1.0f, 0.6f));
            shadowReceiverSh.setBool("useTexture", false);
            shadowReceiverSh.setVector3f("baseCol", glm::vec3(1.0f, 0.1f, 0.1f));
            shadowReceiverSh.setMatrix4("M", halo);
            shadowReceiverSh.setMatrix4("itM", glm::transpose(glm::inverse(halo)));
            shadowReceiverSh.setMatrix4("lightSpaceMatrix", lightSpaceMatrix);
            glBindVertexArray(tileVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        };
        drawCheckHalo(true);
        drawCheckHalo(false);
        
        // Remove greeting message after 10.0 seconds for visibility
        if (!greetingRemoved) {
            greetingTimer += deltaTime;
            if (greetingTimer > 10.0f && greetingIndex >= 0) {
                Billboarding::removeMessage(greetingIndex);
                greetingRemoved = true;
                greetingIndex = -1;
            }
        }
        
        // Check for checkmate/stalemate and display messages
        bool whiteInCheck = GameLogic::isSideInCheck(pieces, true);
        bool blackInCheck = GameLogic::isSideInCheck(pieces, false);
        
        // Debug: Print check status
        static bool lastWhiteInCheck = false, lastBlackInCheck = false;
        if (whiteInCheck != lastWhiteInCheck) {
            if (whiteInCheck) std::cout << "White king is now in check!" << std::endl;
            else std::cout << "White king is no longer in check." << std::endl;
            lastWhiteInCheck = whiteInCheck;
        }
        if (blackInCheck != lastBlackInCheck) {
            if (blackInCheck) std::cout << "Black king is now in check!" << std::endl;
            else std::cout << "Black king is no longer in check." << std::endl;
            lastBlackInCheck = blackInCheck;
        }
        
        // Debug: Check for checkmate and stalemate
        bool whiteCheckmate = false, blackCheckmate = false, stalemate = false;
        
        // Check for checkmate
        if (whiteInCheck) {
            // Check if white has any legal moves
            bool whiteHasLegalMoves = false;
            for (size_t i = 0; i < pieces.size(); ++i) {
                if (pieces[i].isWhite && pieces[i].file >= 0 && pieces[i].rank >= 0) {
                    auto moves = GameLogic::getLegalMovesConsideringCheck(i, pieces);
                    if (!moves.empty()) {
                        whiteHasLegalMoves = true;
                        break;
                    }
                }
            }
            if (!whiteHasLegalMoves) {
                whiteCheckmate = true;
                std::cout << "CHECKMATE: Black wins! White king is checkmated." << std::endl;
            }
        }
        
        if (blackInCheck) {
            // Check if black has any legal moves
            bool blackHasLegalMoves = false;
            for (size_t i = 0; i < pieces.size(); ++i) {
                if (!pieces[i].isWhite && pieces[i].file >= 0 && pieces[i].rank >= 0) {
                    auto moves = GameLogic::getLegalMovesConsideringCheck(i, pieces);
                    if (!moves.empty()) {
                        blackHasLegalMoves = true;
                        break;
                    }
                }
            }
            if (!blackHasLegalMoves) {
                blackCheckmate = true;
                std::cout << "CHECKMATE: White wins! Black king is checkmated." << std::endl;
            }
        }
        
        // Check for stalemate (side to move has no legal moves and is NOT in check)
        if (!whiteCheckmate && !blackCheckmate) {
            bool sideInCheck = whitesTurn ? whiteInCheck : blackInCheck;
            if (!sideInCheck) {
                bool sideHasMoves = false;
                for (size_t i = 0; i < pieces.size(); ++i) {
                    if (pieces[i].isWhite == whitesTurn && pieces[i].file >= 0 && pieces[i].rank >= 0) {
                        auto moves = GameLogic::getLegalMovesConsideringCheck(i, pieces);
                        if (!moves.empty()) { sideHasMoves = true; break; }
                    }
                }
                if (!sideHasMoves) {
                    stalemate = true;
                    std::cout << "STALEMATE: Side to move has no legal moves." << std::endl;
                }
            }
        }
        
        // Remove old check messages
        static int checkMessageIndex = -1;
        if (checkMessageIndex >= 0) {
            Billboarding::removeMessage(checkMessageIndex);
            checkMessageIndex = -1;
        }
        
        // Display check, checkmate, and stalemate messages
        if (whiteCheckmate) {
            checkMessageIndex = Billboarding::getMessageCount();
            Billboarding::createMessage("CHECKMATE! Black Wins!", Billboarding::MessageType::CHECKMATE_BLACK, 
                                      glm::vec3(-3.0f, 1.0f, -2.0f), 1.5f);
        } else if (blackCheckmate) {
            checkMessageIndex = Billboarding::getMessageCount();
            Billboarding::createMessage("CHECKMATE! White Wins!", Billboarding::MessageType::CHECKMATE_WHITE, 
                                      glm::vec3(-3.0f, 1.0f, -2.0f), 1.5f);
        } else if (stalemate) {
            checkMessageIndex = Billboarding::getMessageCount();
            Billboarding::createMessage("STALEMATE! Game Draw!", Billboarding::MessageType::STALEMATE, 
                                      glm::vec3(-3.0f, 1.0f, -2.0f), 1.5f);
        } else if (whiteInCheck) {
            checkMessageIndex = Billboarding::getMessageCount();
            Billboarding::createMessage("White King in Check!", Billboarding::MessageType::CHECK_WHITE, 
                                      glm::vec3(-3.0f, 1.0f, -2.0f), 1.5f);
        } else if (blackInCheck) {
            checkMessageIndex = Billboarding::getMessageCount();
            Billboarding::createMessage("Black King in Check!", Billboarding::MessageType::CHECK_BLACK, 
                                      glm::vec3(-3.0f, 1.0f, -2.0f), 1.5f);
        }

        // Draw pieces with enhanced lighting, reflection, and shadows
        pieceSh.use();
        pieceSh.setMatrix4("V", V); 
        pieceSh.setMatrix4("P", P);
        LightingAndReflection::updatePieceShaderUniforms(pieceSh.ID, camera.Position);
        for(size_t i=0;i<pieces.size();++i){
            if (pieces[i].file < 0 || pieces[i].rank < 0) continue; // captured/off-board
            
            if (i == (size_t)selectedPiece) {
                pieceSh.setVector3f("baseCol", glm::vec3(1.0f, 0.0f, 0.0f));
                pieceSh.setBool("useTexture", false);
                // No reflection for selected piece
                LightingAndReflection::updateReflectionUniforms(pieceSh.ID, camera.Position, skybox.getTextureID(), 0.0f);
            } else if (pieces[i].isWhite) {
                // Use white marble texture for white pieces
                Texture::bindTexture(Texture::PIECE_WHITE_MARBLE, 0);
                pieceSh.setInt("diffuseTexture", 0);
                pieceSh.setBool("useTexture", true);
                pieceSh.setVector3f("baseCol", glm::vec3(1.0f, 1.0f, 1.0f));
                // Add reflection for white pieces
                float reflectionStrength = LightingAndReflection::getReflectionStrength(true, false);
                LightingAndReflection::updateReflectionUniforms(pieceSh.ID, camera.Position, skybox.getTextureID(), reflectionStrength);
            } else {
                // Use black marble texture for black pieces
                Texture::bindTexture(Texture::PIECE_BLACK_MARBLE, 0);
                pieceSh.setInt("diffuseTexture", 0);
                pieceSh.setBool("useTexture", true);
                pieceSh.setVector3f("baseCol", glm::vec3(1.0f, 1.0f, 1.0f));
                // Add reflection for black pieces
                float reflectionStrength = LightingAndReflection::getReflectionStrength(false, false);
                LightingAndReflection::updateReflectionUniforms(pieceSh.ID, camera.Position, skybox.getTextureID(), reflectionStrength);
            }
            
            pieceSh.setMatrix4("M", pieces[i].model);
            pieceSh.setMatrix4("itM", glm::transpose(glm::inverse(pieces[i].model)));
            pieces[i].mesh->draw();
        }

        // Draw axes as plain lines (no lighting needed)
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_LINES, 0, 6);

        // Render billboard messages (after everything else)
        billboardSh.use();
        Billboarding::renderMessages(V, P);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup texture system
    Texture::cleanup();
    
    // Cleanup billboarding system
    Billboarding::cleanup();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
