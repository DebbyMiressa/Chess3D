#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include "../GameLogic/Types.h"

class Camera; // Forward declaration

class MoveObject {
public:
    // Keyboard input handling
    static bool getKeyState(GLFWwindow* window, int key);
    static bool isArrowKeyPressed(GLFWwindow* window, int& dx, int& dy, bool boardFlipped);
    static bool checkArrowKeyEdge(GLFWwindow* window, int& dx, int& dy, bool boardFlipped);
    
    // Cursor navigation
    static void updateCursorPosition(ChessSquare& cursorPos, int dx, int dy, const std::vector<Piece>& pieces, int& selectedPiece);
    static int findPieceAtPosition(const ChessSquare& pos, const std::vector<Piece>& pieces);
    
    // SHIFT-based movement
    static void handleShiftMovement(GLFWwindow* window, bool boardFlipped, ChessSquare& pendingTarget, bool& hasPendingTarget, 
                                   ChessSquare& pendingStart, int& accumDx, int& accumDy, const std::vector<Piece>& pieces, int selectedPiece);
    
    // Move validation and execution
    static bool validateMove(const std::vector<Piece>& pieces, int selectedPiece, const ChessSquare& from, const ChessSquare& to, bool whitesTurn);
    static void executeMove(std::vector<Piece>& pieces, int selectedPiece, const ChessSquare& from, const ChessSquare& to, 
                           bool& whitesTurn, bool& boardFlipped, Camera& camera, int& savedSelWhite, int& savedSelBlack, 
                           ChessSquare& savedCursorWhite, ChessSquare& savedCursorBlack, ChessSquare& cursorPos);
    
    // State management
    static void updatePressedStates(bool left, bool right, bool up, bool down, bool shift);
    static bool getPressedState(int key);
    static bool getPrevShift();
    
    // Direction calculation
    static int getDirectionMultiplier(bool boardFlipped, int direction);

private:
    static bool pressedLeft, pressedRight, pressedUp, pressedDown, prevShift;
};
