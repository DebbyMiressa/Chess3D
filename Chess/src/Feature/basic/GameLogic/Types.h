#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Forward declaration
class Object;

// Chess square representation
struct ChessSquare {
    int file; // 0-7 (a-h)
    int rank; // 0-7 (1-8)
    ChessSquare(int f, int r) : file(f), rank(r) {}
    ChessSquare() : file(0), rank(0) {}
};

// Chess piece types
enum class PieceType {
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
};

// Chess piece representation
struct Piece { 
    Object* mesh; 
    glm::mat4 model; 
    bool isWhite;  // true for white pieces (ranks 1-2), false for black (ranks 7-8)
    int file, rank; // current position on board
    PieceType type; // piece identity
    bool hasMoved = false; // for castling and two-step pawn tracking
};

