#include "Cubemap.h"
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Cubemap::Cubemap(const std::string& folderPath) : textureID(0), loaded(false) {
    // Generate cubemap texture
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    // Load all faces
    bool allLoaded = true;
    allLoaded &= loadFace(RIGHT, folderPath + "/px.png");
    allLoaded &= loadFace(LEFT, folderPath + "/nx.png");
    allLoaded &= loadFace(TOP, folderPath + "/py.png");
    allLoaded &= loadFace(BOTTOM, folderPath + "/ny.png");
    allLoaded &= loadFace(FRONT, folderPath + "/pz.png");
    allLoaded &= loadFace(BACK, folderPath + "/nz.png");
    
    if (allLoaded) {
        // Set texture parameters
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        
        loaded = true;
        std::cout << "Cubemap loaded successfully from: " << folderPath << std::endl;
    } else {
        std::cerr << "Failed to load cubemap from: " << folderPath << std::endl;
        cleanup();
    }
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

Cubemap::~Cubemap() {
    cleanup();
}

void Cubemap::bind(unsigned int slot) const {
    if (loaded && textureID != 0) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    }
}

void Cubemap::cleanup() {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
        textureID = 0;
    }
    loaded = false;
}

bool Cubemap::loadFace(CubemapFace face, const std::string& filePath) {
    int width, height, channels;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        std::cerr << "Failed to load cubemap face: " << filePath << std::endl;
        return false;
    }
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    GLenum target = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
    
    glTexImage2D(target, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    stbi_image_free(data);
    return true;
}

std::string Cubemap::getFaceFileName(CubemapFace face) const {
    switch (face) {
        case RIGHT: return "px.png";
        case LEFT: return "nx.png";
        case TOP: return "py.png";
        case BOTTOM: return "ny.png";
        case FRONT: return "pz.png";
        case BACK: return "nz.png";
        default: return "";
    }
}
