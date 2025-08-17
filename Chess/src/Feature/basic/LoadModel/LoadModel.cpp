#include "LoadModel.h"
#include "../../../Object/Mesh.h"
#include "../../../shader.h"
#include <iostream>

// Initialize static members
std::unique_ptr<Object> LoadModel::meshPawn;
std::unique_ptr<Object> LoadModel::meshRook;
std::unique_ptr<Object> LoadModel::meshKnight;
std::unique_ptr<Object> LoadModel::meshBishop;
std::unique_ptr<Object> LoadModel::meshQueen;
std::unique_ptr<Object> LoadModel::meshKing;
bool LoadModel::meshesInitialized = false;

bool LoadModel::initializeMeshes(Shader& shader) {
    if (meshesInitialized) {
        return true;
    }
    
    try {
        // Load chess piece meshes
        meshPawn   = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/pawn.obj");
        meshRook   = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/rook.obj");
        meshKnight = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/knight.obj");
        meshBishop = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/bishop.obj");
        meshQueen  = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/queen.obj");
        meshKing   = std::make_unique<Object>(PATH_TO_OBJECTS "/Piece/king.obj");
        
        // Initialize meshes with shader
        meshPawn->makeObject(shader, false);
        meshRook->makeObject(shader, false);
        meshKnight->makeObject(shader, false);
        meshBishop->makeObject(shader, false);
        meshQueen->makeObject(shader, false);
        meshKing->makeObject(shader, false);
        
        meshesInitialized = true;
        std::cout << "Meshes: pawn loaded, rook, knight, bishop, queen, king initialized." << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cout << "Error initializing meshes: " << e.what() << std::endl;
        return false;
    }
}

Object* LoadModel::getMeshFor(PieceType pieceType) {
    if (!meshesInitialized) {
        std::cout << "Warning: Meshes not initialized" << std::endl;
        return nullptr;
    }
    
    switch (pieceType) {
        case PieceType::PAWN:   return meshPawn.get();
        case PieceType::ROOK:   return meshRook.get();
        case PieceType::KNIGHT: return meshKnight.get();
        case PieceType::BISHOP: return meshBishop.get();
        case PieceType::QUEEN:  return meshQueen.get();
        case PieceType::KING:   return meshKing.get();
        default:                return meshPawn.get();
    }
}

void LoadModel::cleanupMeshes() {
    meshPawn.reset();
    meshRook.reset();
    meshKnight.reset();
    meshBishop.reset();
    meshQueen.reset();
    meshKing.reset();
    meshesInitialized = false;
    std::cout << "Meshes cleaned up" << std::endl;
}

bool LoadModel::areMeshesInitialized() {
    return meshesInitialized;
}
