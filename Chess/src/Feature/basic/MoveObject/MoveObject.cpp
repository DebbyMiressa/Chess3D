#include "MoveObject.h"
#include "../GameLogic/GameLogic.h"
#include "../CameraControl/CameraControl.h"
#include <iostream>
#include <glm/glm.hpp>

// Static member initializations
bool MoveObject::pressedLeft = false;
bool MoveObject::pressedRight = false;
bool MoveObject::pressedUp = false;
bool MoveObject::pressedDown = false;
bool MoveObject::prevShift = false;

bool MoveObject::getKeyState(GLFWwindow* window, int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}

bool MoveObject::isArrowKeyPressed(GLFWwindow* window, int& dx, int& dy, bool boardFlipped) {
    bool left = getKeyState(window, GLFW_KEY_LEFT);
    bool right = getKeyState(window, GLFW_KEY_RIGHT);
    bool up = getKeyState(window, GLFW_KEY_UP);
    bool down = getKeyState(window, GLFW_KEY_DOWN);
    
    dx = 0; dy = 0;
    
    if (left) dx = boardFlipped ? 1 : -1;
    if (right) dx = boardFlipped ? -1 : 1;
    if (up) dy = boardFlipped ? 1 : -1;
    if (down) dy = boardFlipped ? -1 : 1;
    
    return left || right || up || down;
}

bool MoveObject::checkArrowKeyEdge(GLFWwindow* window, int& dx, int& dy, bool boardFlipped) {
    bool left = getKeyState(window, GLFW_KEY_LEFT);
    bool right = getKeyState(window, GLFW_KEY_RIGHT);
    bool up = getKeyState(window, GLFW_KEY_UP);
    bool down = getKeyState(window, GLFW_KEY_DOWN);
    
    dx = 0; dy = 0;
    bool stepped = false;
    
    if (left && !pressedLeft) { dx = boardFlipped ? 1 : -1; stepped = true; }
    if (right && !pressedRight) { dx = boardFlipped ? -1 : 1; stepped = true; }
    if (up && !pressedUp) { dy = boardFlipped ? 1 : -1; stepped = true; }
    if (down && !pressedDown) { dy = boardFlipped ? -1 : 1; stepped = true; }
    
    return stepped;
}

void MoveObject::updateCursorPosition(ChessSquare& cursorPos, int dx, int dy, const std::vector<Piece>& pieces, int& selectedPiece) {
    cursorPos.file = glm::clamp(cursorPos.file + dx, 0, 7);
    cursorPos.rank = glm::clamp(cursorPos.rank + dy, 0, 7);
    
    // Find piece at new position
    selectedPiece = findPieceAtPosition(cursorPos, pieces);
}

int MoveObject::findPieceAtPosition(const ChessSquare& pos, const std::vector<Piece>& pieces) {
    for(size_t i = 0; i < pieces.size(); ++i) {
        if (pieces[i].file == pos.file && pieces[i].rank == pos.rank) {
            return (int)i;
        }
    }
    return -1;
}

void MoveObject::handleShiftMovement(GLFWwindow* window, bool boardFlipped, ChessSquare& pendingTarget, bool& hasPendingTarget, 
                                   ChessSquare& pendingStart, int& accumDx, int& accumDy, const std::vector<Piece>& pieces, int selectedPiece) {
    if (!prevShift) {
        // SHIFT just pressed: start accumulation from current square
        if (selectedPiece >= 0 && selectedPiece < (int)pieces.size()) {
            pendingStart = ChessSquare(pieces[selectedPiece].file, pieces[selectedPiece].rank);
            accumDx = 0; accumDy = 0;
            pendingTarget = pendingStart;
            hasPendingTarget = true;
        } else {
            hasPendingTarget = false;
        }
    }
    
    if (hasPendingTarget) {
        // Accumulate without validating per step; clamp to board
        auto stepAccum = [&](int dfx, int dfy){ accumDx += dfx; accumDy += dfy; };
        bool stepped = false;
        
        if (getKeyState(window, GLFW_KEY_LEFT) && !pressedLeft) { stepAccum(boardFlipped ? 1 : -1, 0); stepped = true; }
        if (getKeyState(window, GLFW_KEY_RIGHT) && !pressedRight) { stepAccum(boardFlipped ? -1 : 1, 0); stepped = true; }
        if (getKeyState(window, GLFW_KEY_UP) && !pressedUp) { stepAccum(0, boardFlipped ? 1 : -1); stepped = true; }
        if (getKeyState(window, GLFW_KEY_DOWN) && !pressedDown) { stepAccum(0, boardFlipped ? -1 : 1); stepped = true; }
        
        int nf = glm::clamp(pendingStart.file + accumDx, 0, 7);
        int nr = glm::clamp(pendingStart.rank + accumDy, 0, 7);
        pendingTarget = ChessSquare(nf, nr);
        
        if (stepped) {
            std::cout << "SHIFT step: accum=(" << accumDx << "," << accumDy << ") start=(" << pendingStart.file << "," << pendingStart.rank
                      << ") -> target=(" << pendingTarget.file << "," << pendingTarget.rank << ")" << std::endl;
        }
    }
}

bool MoveObject::validateMove(const std::vector<Piece>& pieces, int selectedPiece, const ChessSquare& from, const ChessSquare& to, bool whitesTurn) {
    if (selectedPiece < 0 || selectedPiece >= (int)pieces.size()) return false;
    if (pieces[selectedPiece].isWhite != whitesTurn) return false;
    
    return GameLogic::canCommitMove(pieces, selectedPiece, from, to, whitesTurn);
}

void MoveObject::executeMove(std::vector<Piece>& pieces, int selectedPiece, const ChessSquare& from, const ChessSquare& to, 
                           bool& whitesTurn, bool& boardFlipped, Camera& camera, int& savedSelWhite, int& savedSelBlack, 
                           ChessSquare& savedCursorWhite, ChessSquare& savedCursorBlack, ChessSquare& cursorPos) {
    if (selectedPiece < 0 || selectedPiece >= (int)pieces.size()) return;
    
    auto updatePieceModel = [&](Piece& p){
        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3((p.file - 3.5f) * 0.6f, -0.12f, (p.rank - 3.5f) * 0.6f));
        float scaleByType = (p.type == PieceType::PAWN) ? 0.15f : 0.2f;
        p.model = glm::scale(m, glm::vec3(scaleByType));
    };
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
    
    // Handle capture if enemy present
    captureIfEnemyAt(to.file, to.rank, selectedPiece);
    
    bool movingWasWhite = pieces[selectedPiece].isWhite;
    
    // Castling rook move if king moved two squares
    if (pieces[selectedPiece].type == PieceType::KING && abs(to.file - from.file) == 2) {
        int rr = from.rank;
        if (to.file > from.file) {
            // king side: move rook from 7 to 5
            for (int i = 0; i < (int)pieces.size(); ++i) {
                if (pieces[i].type == PieceType::ROOK && pieces[i].isWhite == movingWasWhite && pieces[i].rank == rr && pieces[i].file == 7) {
                    pieces[i].file = 5;
                    pieces[i].hasMoved = true;
                    updatePieceModel(pieces[i]);
                    break;
                }
            }
        } else {
            // queen side: move rook from 0 to 3
            for (int i = 0; i < (int)pieces.size(); ++i) {
                if (pieces[i].type == PieceType::ROOK && pieces[i].isWhite == movingWasWhite && pieces[i].rank == rr && pieces[i].file == 0) {
                    pieces[i].file = 3;
                    pieces[i].hasMoved = true;
                    updatePieceModel(pieces[i]);
                    break;
                }
            }
        }
    }
    
    // En-passant capture if pawn moved to enPassantSquare
    if (pieces[selectedPiece].type == PieceType::PAWN && GameLogic::enPassantAvailable && 
        to.file == GameLogic::enPassantSquare.file && to.rank == GameLogic::enPassantSquare.rank) {
        int dir = movingWasWhite ? 1 : -1;
        int victimRank = to.rank - dir;
        for (int i = 0; i < (int)pieces.size(); ++i) {
            if (pieces[i].file == to.file && pieces[i].rank == victimRank && pieces[i].type == PieceType::PAWN && pieces[i].isWhite != movingWasWhite) {
                GameLogic::markCaptured(pieces, i);
                break;
            }
        }
    }
    
    // Update piece
    pieces[selectedPiece].file = to.file;
    pieces[selectedPiece].rank = to.rank;
    pieces[selectedPiece].hasMoved = true;
    updatePieceModel(pieces[selectedPiece]);
    
    // Pawn promotion (auto-queen)
    if (pieces[selectedPiece].type == PieceType::PAWN && 
        ((movingWasWhite && pieces[selectedPiece].rank == 7) || (!movingWasWhite && pieces[selectedPiece].rank == 0))) {
        pieces[selectedPiece].type = PieceType::QUEEN;
        updatePieceModel(pieces[selectedPiece]);
    }
    
    // Update en-passant availability for next move
    GameLogic::enPassantAvailable = false;
    GameLogic::enPassantVictimIndex = -1;
    if (pieces[selectedPiece].type == PieceType::PAWN && abs(to.rank - from.rank) == 2) {
        GameLogic::enPassantAvailable = true;
        GameLogic::enPassantVictimIndex = selectedPiece;
        GameLogic::enPassantSquare = ChessSquare(from.file, (from.rank + to.rank)/2);
    }
    
    // Flip orientation after each move
    boardFlipped = !boardFlipped;
    CameraControl::setCameraMode(camera, CameraControl::getCurrentCameraMode(), boardFlipped);
    
    // Set side to move to the opposite of the piece that just moved
    whitesTurn = !movingWasWhite;
    
    // Switch highlight to side-to-move last piece or default
    if (whitesTurn) {
        if (savedSelWhite >= 0) {
            selectedPiece = savedSelWhite;
            cursorPos = savedCursorWhite;
        } else {
            ChessSquare defW(0,1);
            selectedPiece = GameLogic::squareToPieceIndex(defW);
            cursorPos = defW;
        }
    } else {
        if (savedSelBlack >= 0) {
            selectedPiece = savedSelBlack;
            cursorPos = savedCursorBlack;
        } else {
            ChessSquare defB(0,6);
            selectedPiece = GameLogic::squareToPieceIndex(defB);
            cursorPos = defB;
        }
    }
}

void MoveObject::updatePressedStates(bool left, bool right, bool up, bool down, bool shift) {
    pressedLeft = left;
    pressedRight = right;
    pressedUp = up;
    pressedDown = down;
    prevShift = shift;
}

bool MoveObject::getPressedState(int key) {
    switch(key) {
        case GLFW_KEY_LEFT: return pressedLeft;
        case GLFW_KEY_RIGHT: return pressedRight;
        case GLFW_KEY_UP: return pressedUp;
        case GLFW_KEY_DOWN: return pressedDown;
        default: return false;
    }
}

bool MoveObject::getPrevShift() {
    return prevShift;
}

int MoveObject::getDirectionMultiplier(bool boardFlipped, int direction) {
    return boardFlipped ? -direction : direction;
}
