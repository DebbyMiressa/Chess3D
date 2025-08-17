#pragma once

#include <memory>
#include <string>
#include "../GameLogic/Types.h"

// Forward declaration
class Object;
class Shader;

// Model loading system extracted from main.cpp
class LoadModel {
public:
    // Initialize all chess piece meshes
    static bool initializeMeshes(Shader& shader);
    
    // Get mesh for a specific piece type
    static Object* getMeshFor(PieceType pieceType);
    
    // Cleanup meshes
    static void cleanupMeshes();
    
    // Check if meshes are initialized
    static bool areMeshesInitialized();
    
private:
    // Static mesh pointers
    static std::unique_ptr<Object> meshPawn;
    static std::unique_ptr<Object> meshRook;
    static std::unique_ptr<Object> meshKnight;
    static std::unique_ptr<Object> meshBishop;
    static std::unique_ptr<Object> meshQueen;
    static std::unique_ptr<Object> meshKing;
    
    static bool meshesInitialized;
};
