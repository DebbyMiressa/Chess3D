#pragma once

#include <glm/glm.hpp>
#include <string>

// Lighting system extracted from main.cpp
class LightingAndReflection {
public:
    // Get the vertex shader source for pieces
    static std::string getPieceVertexShader();
    
    // Get the fragment shader source for pieces
    static std::string getPieceFragmentShader();
    
    // Get the light position used in the chess application
    static glm::vec3 getLightPosition();
    
    // Set the light position
    static void setLightPosition(const glm::vec3& position);
    
    // Update lighting uniforms for the piece shader
    static void updatePieceShaderUniforms(unsigned int shaderProgram, const glm::vec3& cameraPosition);
    static void setTextureUniforms(unsigned int shaderProgram, unsigned int textureID, bool useTexture = true);
    
    // Enhanced piece shader with reflection support
    static std::string getEnhancedPieceVertexShader();
    static std::string getEnhancedPieceFragmentShader();
    
    // Update reflection uniforms
    static void updateReflectionUniforms(unsigned int shaderProgram, 
                                       const glm::vec3& cameraPosition,
                                       unsigned int environmentMapID,
                                       float reflectionStrength = 0.3f);
    
    // Get reflection strength for different piece types
    static float getReflectionStrength(bool isWhite, bool isMetal);
    
    // Light brightness control
    static void setLightBrightness(float brightness);
    static float getLightBrightness();
    
    // Skybox shader sources
    static std::string getSkyboxVertexShader();
    static std::string getSkyboxFragmentShader();
    
private:
    static glm::vec3 lightPosition;
    static float lightBrightness;
};
