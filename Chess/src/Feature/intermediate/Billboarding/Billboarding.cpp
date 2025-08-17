#include "Billboarding.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <stb_image.h>

// Ensure declarations are visible even if the header isn't found by the compiler
extern "C" {
    unsigned char* stbi_load(char const* filename, int* x, int* y, int* comp, int req_comp);
    void stbi_image_free(void* retval_from_stbi_load);
}

// Initialize static members
bool Billboarding::initialized = false;
unsigned int Billboarding::billboardVAO = 0;
unsigned int Billboarding::billboardVBO = 0;
unsigned int Billboarding::billboardEBO = 0;
std::vector<Billboarding::BillboardMessage> Billboarding::messages;

// Font texture cache - stores loaded character textures
static std::map<char, unsigned int> fontTextures;
static std::map<char, std::pair<int,int>> fontTextureSizes; // char -> {width,height}
static bool fontLoaded = false;

// Load a single character texture from the font directory structure
unsigned int loadCharacterTexture(char character) {
    std::string filename;
    
    // Determine filename based on character and directory structure
    if (character >= 'A' && character <= 'Z') {
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/letter/" + std::string(1, character) + ".png";
    } else if (character >= '0' && character <= '9') {
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/number/" + std::string(1, character) + ".png";
    } else if (character == '!') {
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/symbol/exclamation.png";
    } else if (character == '?') {
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/symbol/question.png";
    } else if (character == ' ') {
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/symbol/space.png";
    } else {
        // Default to space for unknown characters
        filename = "../Chess/src/Feature/intermediate/Billboarding/font/symbol/space.png";
    }
    
    // Load the image using stb_image
    int width, height, channels;
    unsigned char* data = stbi_load(filename.c_str(), &width, &height, &channels, 4);
    
    if (!data) {
        std::cout << "Failed to load font texture: " << filename << std::endl;
        // Create a simple fallback texture (transparent)
        unsigned int fallbackTexture;
        glGenTextures(1, &fallbackTexture);
        glBindTexture(GL_TEXTURE_2D, fallbackTexture);
        
        unsigned char fallbackData[4] = {0, 0, 0, 0}; // Transparent
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, fallbackData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        return fallbackTexture;
    }
    
    // Create OpenGL texture
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Free the image data
    stbi_image_free(data);
    
    // Remember this character's texture size so we can pack without cropping
    fontTextureSizes[character] = {width, height};
    std::cout << "Loaded font texture: " << filename << " (" << width << "x" << height << ")" << std::endl;
    
    return textureID;
}

// Load all font textures
void loadFontTextures() {
    if (fontLoaded) return;
    
    // Load letters A-Z
    for (char c = 'A'; c <= 'Z'; c++) {
        fontTextures[c] = loadCharacterTexture(c);
    }
    
    // Load numbers 0-9
    for (char c = '0'; c <= '9'; c++) {
        fontTextures[c] = loadCharacterTexture(c);
    }
    
    // Load punctuation
    fontTextures['!'] = loadCharacterTexture('!');
    fontTextures['?'] = loadCharacterTexture('?');
    fontTextures[' '] = loadCharacterTexture(' ');
    
    fontLoaded = true;
    std::cout << "Font textures loaded successfully!" << std::endl;
}

// Clean up font textures
void cleanupFontTextures() {
    for (auto& pair : fontTextures) {
        glDeleteTextures(1, &pair.second);
    }
    fontTextures.clear();
    fontLoaded = false;
}

void Billboarding::initialize() {
    if (initialized) return;
    
    setupBillboardGeometry();
    loadFontTextures(); // Load font textures
    initialized = true;
    std::cout << "Billboarding system initialized with PNG font textures" << std::endl;
}

std::string Billboarding::getBillboardVertexShader() {
    return R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        layout(location = 1) in vec2 texCoord;
        
        out vec2 TexCoord;
        
        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;
        uniform vec3 cameraPos;
        uniform vec3 cameraRight;
        uniform vec3 cameraUp;
        uniform float scale;
        
        void main() {
            // Billboard calculation - always face camera
            vec3 worldPos = position * scale;
            vec3 billboardPos = worldPos.x * cameraRight + worldPos.y * cameraUp;
            vec3 finalPos = (model * vec4(0.0, 0.0, 0.0, 1.0)).xyz + billboardPos;
            
            gl_Position = projection * view * vec4(finalPos, 1.0);
            TexCoord = texCoord;
        }
    )";
}

std::string Billboarding::getBillboardFragmentShader() {
    return R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;
        
        uniform sampler2D textTexture;
        uniform vec3 textColor;
        uniform float alpha;
        
        void main() {
            vec4 texColor = texture(textTexture, TexCoord);
            if (texColor.a < 0.1) discard; // Discard transparent pixels
            
            // Use the texture's alpha directly for text visibility
            FragColor = vec4(0.0, 0.0, 0.0, texColor.a); // Pure black text
        }
    )";
}

void Billboarding::createMessage(const std::string& text, MessageType type, 
                               const glm::vec3& position, float scale) {
    BillboardMessage message;
    message.text = text;
    message.type = type;
    message.position = position;
    message.scale = scale;
    message.visible = true;
    int texW = 0, texH = 0;
    message.textureID = createTextTexture(text, type, texW, texH);
    message.width = static_cast<float>(texW) / 64.0f;  // normalize to glyph size to keep squareish
    message.height = static_cast<float>(texH) / 64.0f;
    
    messages.push_back(message);
    std::cout << "Created billboard message: " << text << std::endl;
}

void Billboarding::updateMessage(int messageIndex, const std::string& newText) {
    if (messageIndex >= 0 && messageIndex < (int)messages.size()) {
        messages[messageIndex].text = newText;
        // Recreate texture with new text
        glDeleteTextures(1, &messages[messageIndex].textureID);
        int texW = 0, texH = 0;
        messages[messageIndex].textureID = createTextTexture(newText, messages[messageIndex].type, texW, texH);
        messages[messageIndex].width = static_cast<float>(texW) / 64.0f;
        messages[messageIndex].height = static_cast<float>(texH) / 64.0f;
    }
}

void Billboarding::removeMessage(int messageIndex) {
    if (messageIndex >= 0 && messageIndex < (int)messages.size()) {
        glDeleteTextures(1, &messages[messageIndex].textureID);
        messages.erase(messages.begin() + messageIndex);
    }
}

void Billboarding::clearAllMessages() {
    for (auto& message : messages) {
        glDeleteTextures(1, &message.textureID);
    }
    messages.clear();
}

void Billboarding::renderMessages(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) {
    if (!initialized || messages.empty()) return;
    
    // Extract camera position and orientation from view matrix
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::vec3 cameraPos = glm::vec3(invView[3]);
    glm::vec3 cameraRight = glm::vec3(invView[0]);
    glm::vec3 cameraUp = glm::vec3(invView[1]);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    
    glBindVertexArray(billboardVAO);
    
    for (const auto& message : messages) {
        if (!message.visible) continue;
        
        // Set uniforms for this message
        glm::mat4 model = glm::translate(glm::mat4(1.0f), message.position);
        
        // Bind the text texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, message.textureID);
        
        // Get current shader program ID
        GLint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        
        glUniformMatrix4fv(glGetUniformLocation(currentProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(currentProgram, "view"), 1, GL_FALSE, glm::value_ptr(viewMatrix));
        glUniformMatrix4fv(glGetUniformLocation(currentProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniform3fv(glGetUniformLocation(currentProgram, "cameraPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(currentProgram, "cameraRight"), 1, glm::value_ptr(cameraRight));
        glUniform3fv(glGetUniformLocation(currentProgram, "cameraUp"), 1, glm::value_ptr(cameraUp));
        glUniform1f(glGetUniformLocation(currentProgram, "scale"), message.scale);
        
        glm::vec3 color = getMessageColor(message.type);
        glUniform1i(glGetUniformLocation(currentProgram, "textTexture"), 0);
        glUniform1f(glGetUniformLocation(currentProgram, "alpha"), 1.0f);
        
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }
    
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void Billboarding::setMessageVisible(int messageIndex, bool visible) {
    if (messageIndex >= 0 && messageIndex < (int)messages.size()) {
        messages[messageIndex].visible = visible;
    }
}

void Billboarding::setMessagePosition(int messageIndex, const glm::vec3& position) {
    if (messageIndex >= 0 && messageIndex < (int)messages.size()) {
        messages[messageIndex].position = position;
    }
}

void Billboarding::setMessageScale(int messageIndex, float scale) {
    if (messageIndex >= 0 && messageIndex < (int)messages.size()) {
        messages[messageIndex].scale = scale;
    }
}

int Billboarding::getMessageCount() {
    return (int)messages.size();
}

void Billboarding::cleanup() {
    if (!initialized) return;
    
    clearAllMessages();
    cleanupFontTextures(); // Clean up font textures
    
    if (billboardVAO != 0) {
        glDeleteVertexArrays(1, &billboardVAO);
        billboardVAO = 0;
    }
    if (billboardVBO != 0) {
        glDeleteBuffers(1, &billboardVBO);
        billboardVBO = 0;
    }
    if (billboardEBO != 0) {
        glDeleteBuffers(1, &billboardEBO);
        billboardEBO = 0;
    }
    
    initialized = false;
    std::cout << "Billboarding system cleaned up" << std::endl;
}

static inline char toUpperAscii(char c) {
    if (c >= 'a' && c <= 'z') return static_cast<char>(c - 'a' + 'A');
    return c;
}

unsigned int Billboarding::createTextTexture(const std::string& text, MessageType type, int& outWidth, int& outHeight) {
    loadFontTextures();

    // Build an uppercase version to be case-insensitive
    std::string upper;
    upper.reserve(text.size());
    for (char c : text) upper.push_back(toUpperAscii(c));

    // Determine total canvas size based on glyph sizes (use actual 64x64, with small spacing)
    const int spacingX = 6;
    const int spacingY = 0;
    int totalWidth = 0;
    int maxHeight = 0;
    for (size_t i = 0; i < upper.size(); ++i) {
        char c = upper[i];
        auto szIt = fontTextureSizes.find(c);
        int w = 64, h = 64;
        if (szIt != fontTextureSizes.end()) { w = szIt->second.first; h = szIt->second.second; }
        totalWidth += w + spacingX;
        if (h > maxHeight) maxHeight = h;
    }
    if (totalWidth <= 0) totalWidth = 64; if (maxHeight <= 0) maxHeight = 64;

    // Add margins
    const int marginX = 16;
    const int marginY = 8;
    int width = totalWidth + marginX * 2;
    int height = maxHeight + marginY * 2;
    outWidth = width; outHeight = height;

    std::vector<unsigned char> textureData(width * height * 4, 0);

    // Pack glyphs left-to-right using full glyph sizes; flip vertically while copying to fix inversion
    int cursorX = marginX;
    int baselineY = marginY; // top-aligned

    for (size_t i = 0; i < upper.size(); ++i) {
        char c = upper[i];
        auto texIt = fontTextures.find(c);
        if (texIt == fontTextures.end()) { cursorX += 64 + spacingX; continue; }

        // Determine this glyph's size
        int glyphW = 64, glyphH = 64;
        auto szIt = fontTextureSizes.find(c);
        if (szIt != fontTextureSizes.end()) { glyphW = szIt->second.first; glyphH = szIt->second.second; }

        // Read glyph pixels from its GL texture
        glBindTexture(GL_TEXTURE_2D, texIt->second);
        std::vector<unsigned char> glyphData(glyphW * glyphH * 4);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, glyphData.data());

        // Copy glyph while vertically scaling height by 0.5 (squash): sample every other source row
        for (int y = 0; y < glyphH / 2; ++y) {
            for (int x = 0; x < glyphW; ++x) {
                int outX = cursorX + x;
                int srcY = y * 2; // scale factor 0.5 on height
                int outY = baselineY + ((glyphH / 2) - 1 - y); // still flipped vertically
                if (outX < 0 || outX >= width || outY < 0 || outY >= height) continue;
                int outIndex = (outY * width + outX) * 4;
                int inIndex = (srcY * glyphW + x) * 4;
                textureData[outIndex + 0] = glyphData[inIndex + 0];
                textureData[outIndex + 1] = glyphData[inIndex + 1];
                textureData[outIndex + 2] = glyphData[inIndex + 2];
                textureData[outIndex + 3] = glyphData[inIndex + 3];
            }
        }

        cursorX += glyphW + spacingX;
    }

    // Upload final atlas texture
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Created text texture (" << width << "x" << height << ") for '" << text << "' using full glyph sizes" << std::endl;
    return textureID;
}

glm::vec3 Billboarding::getMessageColor(MessageType type) {
    switch (type) {
        case MessageType::GREETING:
            return glm::vec3(0.2f, 0.8f, 0.2f); // Green
        case MessageType::STALEMATE:
            return glm::vec3(0.8f, 0.8f, 0.2f); // Yellow
        case MessageType::CHECKMATE_WHITE:
        case MessageType::CHECKMATE_BLACK:
            return glm::vec3(0.8f, 0.2f, 0.2f); // Red
        case MessageType::CHECK_WHITE:
        case MessageType::CHECK_BLACK:
            return glm::vec3(0.8f, 0.4f, 0.2f); // Orange
        default:
            return glm::vec3(1.0f, 1.0f, 1.0f); // White
    }
}

void Billboarding::setupBillboardGeometry() {
    // Create a wide but taller quad for billboarding (2:1 aspect ratio for better text display)
    float vertices[] = {
        // positions        // texture coords
        -1.5f, -0.5f, 0.0f,  0.0f, 0.0f,
         1.5f, -0.5f, 0.0f,  1.0f, 0.0f,
         1.5f,  0.5f, 0.0f,  1.0f, 1.0f,
        -1.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glGenVertexArrays(1, &billboardVAO);
    glGenBuffers(1, &billboardVBO);
    glGenBuffers(1, &billboardEBO);
    
    glBindVertexArray(billboardVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, billboardEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Texture coordinate attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
}
