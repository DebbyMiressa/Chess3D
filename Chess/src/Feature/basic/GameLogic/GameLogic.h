#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Types.h"

// Chess game logic system extracted from main.cpp
class GameLogic {
public:
    // Board validation
    static bool isValidSquare(int file, int rank);
    
    // Square occupation checks
    static bool isSquareOccupied(int file, int rank, const std::vector<Piece>& pieces, int excludePiece = -1);
    static bool isSquareOccupiedByEnemy(int file, int rank, bool isWhite, const std::vector<Piece>& pieces, int excludePiece = -1);
    
    // Move generation
    static std::vector<ChessSquare> getAllowableMoves(int pieceIndex, const std::vector<Piece>& pieces);
    static std::vector<ChessSquare> getLegalMovesConsideringCheck(int pieceIndex, const std::vector<Piece>& pieces);
    
    // Move validation
    static bool isValidMove(int pieceIndex, int targetFile, int targetRank, const std::vector<Piece>& pieces);
    static bool canCommitMove(const std::vector<Piece>& pcs, int moverIdx, ChessSquare from, ChessSquare to, bool sideToMove);
    static bool wouldBeLegalMove(const std::vector<Piece>& piecesIn, int moverIdx, ChessSquare from, ChessSquare to, bool enPassAvail, ChessSquare enPassSq);
    
    // Check detection
    static bool isSideInCheck(const std::vector<Piece>& pcs, bool sideIsWhite);
    static bool isSquareAttacked(const std::vector<Piece>& pcs, int f, int r, bool byWhite);
    static int findKingIndex(const std::vector<Piece>& pcs, bool isWhite);
    
    // Piece management
    static void markCaptured(std::vector<Piece>& pieces, int idx);
    
    // Utility functions
    static ChessSquare notationToSquare(const std::string& notation);
    static int squareToPieceIndex(const ChessSquare& square);
    static ChessSquare pieceIndexToSquare(int pieceIndex);
    
    // En-passant state (externally managed)
    static bool enPassantAvailable;
    static ChessSquare enPassantSquare;
    static int enPassantVictimIndex;
};
