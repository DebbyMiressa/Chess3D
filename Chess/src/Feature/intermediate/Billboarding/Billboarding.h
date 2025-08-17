#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

class Billboarding {
public:
    // Message types for different game states
    enum class MessageType {
        GREETING,
        STALEMATE,
        CHECKMATE_WHITE,
        CHECKMATE_BLACK,
        CHECK_WHITE,
        CHECK_BLACK
    };
    
    // Initialize billboarding system
    static void initialize();
    
    // Get billboard vertex shader
    static std::string getBillboardVertexShader();
    
    // Get billboard fragment shader
    static std::string getBillboardFragmentShader();
    
    // Create a billboard message
    static void createMessage(const std::string& text, MessageType type, 
                            const glm::vec3& position, float scale = 1.0f);
    
    // Update message text
    static void updateMessage(int messageIndex, const std::string& newText);
    
    // Remove a message
    static void removeMessage(int messageIndex);
    
    // Clear all messages
    static void clearAllMessages();
    
    // Render all billboard messages
    static void renderMessages(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix);
    
    // Set message visibility
    static void setMessageVisible(int messageIndex, bool visible);
    
    // Set message position
    static void setMessagePosition(int messageIndex, const glm::vec3& position);
    
    // Set message scale
    static void setMessageScale(int messageIndex, float scale);
    
    // Get message count
    static int getMessageCount();
    
    // Cleanup
    static void cleanup();

private:
    struct BillboardMessage {
        std::string text;
        MessageType type;
        glm::vec3 position;
        float scale;
        bool visible;
        unsigned int textureID;
        float width;
        float height;
    };
    
    static bool initialized;
    static unsigned int billboardVAO;
    static unsigned int billboardVBO;
    static unsigned int billboardEBO;
    static std::vector<BillboardMessage> messages;
    
    // Helper functions
    static unsigned int createTextTexture(const std::string& text, MessageType type, int& outWidth, int& outHeight);
    static glm::vec3 getMessageColor(MessageType type);
    static void setupBillboardGeometry();
};
