#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <map>
#include <functional>

class Texture {
public:
    // Texture types for different chess elements
    enum TextureType {
        BOARD_WOOD_LIGHT,
        BOARD_WOOD_DARK,
        PIECE_WHITE_MARBLE,
        PIECE_BLACK_MARBLE,
        PIECE_GOLD_METAL,
        PIECE_SILVER_METAL,
        PIECE_BRONZE_METAL,
        PIECE_IVORY,
        PIECE_EBONY,
        PIECE_EMERALD,
        PIECE_RUBY,
        PIECE_SAPPHIRE,
        PIECE_DIAMOND
    };

    // Initialize texture system
    static void initialize();
    
    // Load and manage textures
    static unsigned int loadTexture(const std::string& path);
    static unsigned int loadTextureFromMemory(const unsigned char* data, int width, int height, int channels);
    static unsigned int createProceduralTexture(int width, int height, const std::function<void(unsigned char*, int, int)>& generator);
    
    // Get texture by type
    static unsigned int getTexture(TextureType type);
    
    // Bind texture for rendering
    static void bindTexture(unsigned int textureID, unsigned int slot = 0);
    static void bindTexture(TextureType type, unsigned int slot = 0);
    
    // Cleanup
    static void cleanup();
    
    // Procedural texture generators
    static unsigned int generateWoodTexture(bool light = true);
    static unsigned int generateMarbleTexture(bool white = true);
    static unsigned int generateMetalTexture(bool gold = true);
    static unsigned int generateGemTexture(TextureType gemType);
    static unsigned int generateChessPatternTexture();

private:
    static std::map<TextureType, unsigned int> textureCache;
    static std::map<std::string, unsigned int> loadedTextures;
    static bool initialized;
    
    // Helper functions
    static void generateWoodPattern(unsigned char* data, int width, int height, bool light);
    static void generateMarblePattern(unsigned char* data, int width, int height, bool white);
    static void generateMetalPattern(unsigned char* data, int width, int height, bool gold);
    static void generateGemPattern(unsigned char* data, int width, int height, TextureType gemType);
    static void generateChessPattern(unsigned char* data, int width, int height);
    
    // Noise generation for procedural textures
    static float noise(float x, float y);
    static float smoothNoise(float x, float y);
    static float interpolate(float a, float b, float factor);
    static float perlinNoise(float x, float y);
};
