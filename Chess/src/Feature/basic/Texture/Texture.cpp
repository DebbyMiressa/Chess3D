#include "Texture.h"
#include <iostream>
#include <random>
#include <cmath>

// Static member initializations
std::map<Texture::TextureType, unsigned int> Texture::textureCache;
std::map<std::string, unsigned int> Texture::loadedTextures;
bool Texture::initialized = false;

void Texture::initialize() {
    if (initialized) {
        // Clear existing textures to regenerate with new parameters
        cleanup();
    }
    
    // Generate all procedural textures
    generateWoodTexture(true);   // Light wood for board
    generateWoodTexture(false);  // Dark wood for board
    generateMarbleTexture(true); // White marble
    generateMarbleTexture(false); // Black marble
    generateMetalTexture(true);  // Gold metal
    generateMetalTexture(false); // Silver metal
    generateGemTexture(PIECE_EMERALD);
    generateGemTexture(PIECE_RUBY);
    generateGemTexture(PIECE_SAPPHIRE);
    generateGemTexture(PIECE_DIAMOND);
    
    initialized = true;
    std::cout << "Texture system initialized with " << textureCache.size() << " textures" << std::endl;
}

unsigned int Texture::loadTexture(const std::string& path) {
    if (loadedTextures.find(path) != loadedTextures.end()) {
        return loadedTextures[path];
    }
    
    // For now, return a default texture since we're using procedural textures
    // In a real implementation, you would load from file using stb_image
    return 0;
}

unsigned int Texture::loadTextureFromMemory(const unsigned char* data, int width, int height, int channels) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return textureID;
}

unsigned int Texture::createProceduralTexture(int width, int height, const std::function<void(unsigned char*, int, int)>& generator) {
    std::vector<unsigned char> data(width * height * 3);
    generator(data.data(), width, height);
    return loadTextureFromMemory(data.data(), width, height, 3);
}

unsigned int Texture::getTexture(TextureType type) {
    auto it = textureCache.find(type);
    if (it != textureCache.end()) {
        return it->second;
    }
    return 0;
}

void Texture::bindTexture(unsigned int textureID, unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, textureID);
}

void Texture::bindTexture(TextureType type, unsigned int slot) {
    bindTexture(getTexture(type), slot);
}

void Texture::cleanup() {
    for (auto& pair : textureCache) {
        glDeleteTextures(1, &pair.second);
    }
    textureCache.clear();
    
    for (auto& pair : loadedTextures) {
        glDeleteTextures(1, &pair.second);
    }
    loadedTextures.clear();
    
    initialized = false;
}

unsigned int Texture::generateWoodTexture(bool light) {
    TextureType type = light ? BOARD_WOOD_LIGHT : BOARD_WOOD_DARK;
    if (textureCache.find(type) != textureCache.end()) {
        return textureCache[type];
    }
    
    unsigned int textureID = createProceduralTexture(512, 512, 
        [light](unsigned char* data, int width, int height) {
            generateWoodPattern(data, width, height, light);
        });
    
    textureCache[type] = textureID;
    return textureID;
}

unsigned int Texture::generateMarbleTexture(bool white) {
    TextureType type = white ? PIECE_WHITE_MARBLE : PIECE_BLACK_MARBLE;
    if (textureCache.find(type) != textureCache.end()) {
        return textureCache[type];
    }
    
    unsigned int textureID = createProceduralTexture(512, 512, 
        [white](unsigned char* data, int width, int height) {
            generateMarblePattern(data, width, height, white);
        });
    
    textureCache[type] = textureID;
    return textureID;
}

unsigned int Texture::generateMetalTexture(bool gold) {
    TextureType type = gold ? PIECE_GOLD_METAL : PIECE_SILVER_METAL;
    if (textureCache.find(type) != textureCache.end()) {
        return textureCache[type];
    }
    
    unsigned int textureID = createProceduralTexture(512, 512, 
        [gold](unsigned char* data, int width, int height) {
            generateMetalPattern(data, width, height, gold);
        });
    
    textureCache[type] = textureID;
    return textureID;
}

unsigned int Texture::generateGemTexture(TextureType gemType) {
    if (textureCache.find(gemType) != textureCache.end()) {
        return textureCache[gemType];
    }
    
    unsigned int textureID = createProceduralTexture(512, 512, 
        [gemType](unsigned char* data, int width, int height) {
            generateGemPattern(data, width, height, gemType);
        });
    
    textureCache[gemType] = textureID;
    return textureID;
}

unsigned int Texture::generateChessPatternTexture() {
    return createProceduralTexture(512, 512, 
        [](unsigned char* data, int width, int height) {
            generateChessPattern(data, width, height);
        });
}

// Procedural texture generation functions
void Texture::generateWoodPattern(unsigned char* data, int width, int height, bool light) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> noiseDist(0.0f, 1.0f);
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            
            // Base wood color
            float baseR, baseG, baseB;
            if (light) {
                baseR = 0.95f; baseG = 0.85f; baseB = 0.65f; // Lighter oak
            } else {
                baseR = 0.6f; baseG = 0.4f; baseB = 0.2f; // Lighter walnut
            }
            
            // Wood grain pattern
            float grain = perlinNoise(x * 0.01f, y * 0.01f) * 0.2f + 0.8f;
            float ring = perlinNoise(x * 0.02f, y * 0.02f) * 0.15f + 0.85f;
            
            // Combine patterns
            float pattern = grain * ring;
            
            data[index] = static_cast<unsigned char>((baseR * pattern) * 255);
            data[index + 1] = static_cast<unsigned char>((baseG * pattern) * 255);
            data[index + 2] = static_cast<unsigned char>((baseB * pattern) * 255);
        }
    }
}

void Texture::generateMarblePattern(unsigned char* data, int width, int height, bool white) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            
            // Base marble color
            float baseR, baseG, baseB;
            if (white) {
                baseR = 0.95f; baseG = 0.95f; baseB = 0.9f; // White marble
            } else {
                baseR = 0.1f; baseG = 0.1f; baseB = 0.15f; // Black marble
            }
            
            // Marble veining
            float vein1 = perlinNoise(x * 0.005f, y * 0.005f);
            float vein2 = perlinNoise(x * 0.01f, y * 0.01f);
            float vein3 = perlinNoise(x * 0.02f, y * 0.02f);
            
            float veins = (vein1 * 0.5f + vein2 * 0.3f + vein3 * 0.2f) * 0.4f;
            
            // Add some variation
            float variation = perlinNoise(x * 0.03f, y * 0.03f) * 0.1f + 0.9f;
            
            float finalR = baseR * variation + veins * (white ? 0.3f : 0.1f);
            float finalG = baseG * variation + veins * (white ? 0.3f : 0.1f);
            float finalB = baseB * variation + veins * (white ? 0.3f : 0.1f);
            
            // Clamp values to [0, 1] range
            finalR = (finalR < 0.0f) ? 0.0f : ((finalR > 1.0f) ? 1.0f : finalR);
            finalG = (finalG < 0.0f) ? 0.0f : ((finalG > 1.0f) ? 1.0f : finalG);
            finalB = (finalB < 0.0f) ? 0.0f : ((finalB > 1.0f) ? 1.0f : finalB);
            
            data[index] = static_cast<unsigned char>(finalR * 255);
            data[index + 1] = static_cast<unsigned char>(finalG * 255);
            data[index + 2] = static_cast<unsigned char>(finalB * 255);
        }
    }
}

void Texture::generateMetalPattern(unsigned char* data, int width, int height, bool gold) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            
            // Base metal color
            float baseR, baseG, baseB;
            if (gold) {
                baseR = 0.8f; baseG = 0.6f; baseB = 0.2f; // Gold
            } else {
                baseR = 0.8f; baseG = 0.8f; baseB = 0.9f; // Silver
            }
            
            // Metal surface variation
            float surface = perlinNoise(x * 0.05f, y * 0.05f) * 0.2f + 0.8f;
            float reflection = perlinNoise(x * 0.1f, y * 0.1f) * 0.3f + 0.7f;
            
            float finalR = baseR * surface * reflection;
            float finalG = baseG * surface * reflection;
            float finalB = baseB * surface * reflection;
            
            // Clamp values to [0, 1] range
            finalR = (finalR < 0.0f) ? 0.0f : ((finalR > 1.0f) ? 1.0f : finalR);
            finalG = (finalG < 0.0f) ? 0.0f : ((finalG > 1.0f) ? 1.0f : finalG);
            finalB = (finalB < 0.0f) ? 0.0f : ((finalB > 1.0f) ? 1.0f : finalB);
            
            data[index] = static_cast<unsigned char>(finalR * 255);
            data[index + 1] = static_cast<unsigned char>(finalG * 255);
            data[index + 2] = static_cast<unsigned char>(finalB * 255);
        }
    }
}

void Texture::generateGemPattern(unsigned char* data, int width, int height, TextureType gemType) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            
            // Base gem color
            float baseR, baseG, baseB;
            switch (gemType) {
                case PIECE_EMERALD:
                    baseR = 0.0f; baseG = 0.8f; baseB = 0.4f; break;
                case PIECE_RUBY:
                    baseR = 0.8f; baseG = 0.1f; baseB = 0.1f; break;
                case PIECE_SAPPHIRE:
                    baseR = 0.1f; baseG = 0.1f; baseB = 0.8f; break;
                case PIECE_DIAMOND:
                    baseR = 0.9f; baseG = 0.9f; baseB = 1.0f; break;
                default:
                    baseR = 0.5f; baseG = 0.5f; baseB = 0.5f; break;
            }
            
            // Gem facets and reflections
            float facet1 = perlinNoise(x * 0.02f, y * 0.02f);
            float facet2 = perlinNoise(x * 0.04f, y * 0.04f);
            float reflection = perlinNoise(x * 0.08f, y * 0.08f) * 0.5f + 0.5f;
            
            float facets = (facet1 * 0.6f + facet2 * 0.4f) * 0.3f + 0.7f;
            float finalR = baseR * facets * reflection;
            float finalG = baseG * facets * reflection;
            float finalB = baseB * facets * reflection;
            
            // Clamp values to [0, 1] range
            finalR = (finalR < 0.0f) ? 0.0f : ((finalR > 1.0f) ? 1.0f : finalR);
            finalG = (finalG < 0.0f) ? 0.0f : ((finalG > 1.0f) ? 1.0f : finalG);
            finalB = (finalB < 0.0f) ? 0.0f : ((finalB > 1.0f) ? 1.0f : finalB);
            
            data[index] = static_cast<unsigned char>(finalR * 255);
            data[index + 1] = static_cast<unsigned char>(finalG * 255);
            data[index + 2] = static_cast<unsigned char>(finalB * 255);
        }
    }
}

void Texture::generateChessPattern(unsigned char* data, int width, int height) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 3;
            
            // Create a chess pattern
            bool isLight = ((x / 64) + (y / 64)) % 2 == 0;
            
            if (isLight) {
                data[index] = 245; data[index + 1] = 235; data[index + 2] = 200; // Light square
            } else {
                data[index] = 139; data[index + 1] = 69; data[index + 2] = 19; // Dark square
            }
        }
    }
}

// Noise generation functions
float Texture::noise(float x, float y) {
    int n = static_cast<int>(x + y * 57);
    n = (n << 13) ^ n;
    return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
}

float Texture::smoothNoise(float x, float y) {
    float corners = (noise(x-1, y-1) + noise(x+1, y-1) + noise(x-1, y+1) + noise(x+1, y+1)) / 16;
    float sides = (noise(x-1, y) + noise(x+1, y) + noise(x, y-1) + noise(x, y+1)) / 8;
    float center = noise(x, y) / 4;
    return corners + sides + center;
}

float Texture::interpolate(float a, float b, float factor) {
    float ft = factor * 3.1415927f;
    float f = (1.0f - cos(ft)) * 0.5f;
    return a * (1.0f - f) + b * f;
}

float Texture::perlinNoise(float x, float y) {
    int intX = static_cast<int>(x);
    int intY = static_cast<int>(y);
    float fracX = x - intX;
    float fracY = y - intY;
    
    float v1 = smoothNoise(intX, intY);
    float v2 = smoothNoise(intX + 1, intY);
    float v3 = smoothNoise(intX, intY + 1);
    float v4 = smoothNoise(intX + 1, intY + 1);
    
    float i1 = interpolate(v1, v2, fracX);
    float i2 = interpolate(v3, v4, fracX);
    
    return interpolate(i1, i2, fracY);
}
