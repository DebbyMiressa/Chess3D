#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>

class Cubemap {
public:
    // Cubemap face types
    enum CubemapFace {
        RIGHT = 0,   // +X
        LEFT = 1,    // -X
        TOP = 2,     // +Y
        BOTTOM = 3,  // -Y
        FRONT = 4,   // +Z
        BACK = 5     // -Z
    };

    // Constructor - loads cubemap from folder
    Cubemap(const std::string& folderPath);
    
    // Destructor
    ~Cubemap();
    
    // Bind cubemap for rendering
    void bind(unsigned int slot = 0) const;
    
    // Get the OpenGL texture ID
    unsigned int getTextureID() const { return textureID; }
    
    // Check if cubemap is valid
    bool isValid() const { return textureID != 0; }
    
    // Cleanup
    void cleanup();

private:
    unsigned int textureID;
    bool loaded;
    
    // Load individual face
    bool loadFace(CubemapFace face, const std::string& filePath);
    
    // Get face name for file loading
    std::string getFaceFileName(CubemapFace face) const;
};
