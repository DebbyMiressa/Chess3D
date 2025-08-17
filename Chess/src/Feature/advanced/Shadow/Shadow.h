#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

class Shadow {
public:
    // Shadow map resolution
    static const unsigned int SHADOW_WIDTH = 2048;
    static const unsigned int SHADOW_HEIGHT = 2048;
    
    // Initialize shadow system
    static void initialize();
    
    // Get shadow mapping vertex shader
    static std::string getShadowVertexShader();
    
    // Get shadow mapping fragment shader
    static std::string getShadowFragmentShader();
    
    // Get shadow-receiving vertex shader
    static std::string getShadowReceiverVertexShader();
    
    // Get shadow-receiving fragment shader
    static std::string getShadowReceiverFragmentShader();
    
    // Generate shadow map
    static void generateShadowMap(unsigned int shadowMapFBO, 
                                const glm::vec3& lightPos,
                                const glm::vec3& lightDir,
                                float nearPlane = 0.1f,
                                float farPlane = 100.0f);
    
    // Get light space matrix for shadow mapping
    static glm::mat4 getLightSpaceMatrix(const glm::vec3& lightPos,
                                        const glm::vec3& lightDir,
                                        float nearPlane = 0.1f,
                                        float farPlane = 100.0f);
    
    // Update shadow uniforms for rendering
    static void updateShadowUniforms(unsigned int shaderProgram,
                                   unsigned int shadowMap,
                                   const glm::mat4& lightSpaceMatrix);
    
    // Create shadow map framebuffer
    static unsigned int createShadowMapFBO();
    
    // Cleanup
    static void cleanup();

private:
    static bool initialized;
    static glm::mat4 lightProjection;
    static glm::mat4 lightView;
};
