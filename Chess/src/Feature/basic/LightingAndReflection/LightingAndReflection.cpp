#include "LightingAndReflection.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

// Initialize static members
glm::vec3 LightingAndReflection::lightPosition = glm::vec3(1.5f, 2.0f, 1.5f); // Very close for dramatic shadows
float LightingAndReflection::lightBrightness = 1.0f; // Default brightness

std::string LightingAndReflection::getPieceVertexShader() {
    return R"(
        #version 330 core
        layout(location=0) in vec3 position; 
        layout(location=1) in vec2 texCoord;
        layout(location=2) in vec3 normal;
        uniform mat4 M; 
        uniform mat4 itM; 
        uniform mat4 V; 
        uniform mat4 P;
        out vec3 vFrag; 
        out vec3 vNorm;
        out vec2 vTexCoord;
        void main(){ 
            vec4 w=M*vec4(position,1.0); 
            vFrag=w.xyz; 
            vNorm=mat3(itM)*normal; 
            vTexCoord=texCoord;
            gl_Position=P*V*w; 
        }
    )";
}

std::string LightingAndReflection::getPieceFragmentShader() {
    return R"(
        #version 330 core
        in vec3 vFrag; 
        in vec3 vNorm; 
        in vec2 vTexCoord;
        out vec4 FragColor;
        uniform vec3 uViewPos; 
        uniform vec3 lightPos; 
        uniform vec3 baseCol;
        uniform sampler2D diffuseTexture;
        uniform bool useTexture;
        void main(){ 
            vec3 N=normalize(vNorm); 
            vec3 L=normalize(lightPos-vFrag); 
            vec3 V=normalize(uViewPos-vFrag); 
            vec3 R=reflect(-L,N);
            float diff=max(dot(N,L),0.0); 
            float spec=pow(max(dot(R,V),0.0),32.0); 
            float c=0.2+0.7*diff+0.5*spec; 
            
            vec3 finalColor;
            if(useTexture) {
                vec3 texColor = texture(diffuseTexture, vTexCoord).rgb;
                // Apply lighting to texture color with better balance
                finalColor = texColor * (0.4 + 0.6 * c);
            } else {
                finalColor = baseCol * c;
            }
            
            FragColor=vec4(finalColor,1.0); 
        }
    )";
}

glm::vec3 LightingAndReflection::getLightPosition() {
    return lightPosition;
}

void LightingAndReflection::setLightPosition(const glm::vec3& position) {
    lightPosition = position;
}

void LightingAndReflection::setLightBrightness(float brightness) {
    lightBrightness = brightness;
}

float LightingAndReflection::getLightBrightness() {
    return lightBrightness;
}

void LightingAndReflection::updatePieceShaderUniforms(unsigned int shaderProgram, const glm::vec3& cameraPosition) {
    // Set camera position for lighting calculations
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(cameraPosition));
    
    // Set light position
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPosition));
    
    // Set light brightness
    glUniform1f(glGetUniformLocation(shaderProgram, "lightBrightness"), lightBrightness);
}

void LightingAndReflection::setTextureUniforms(unsigned int shaderProgram, unsigned int textureID, bool useTexture) {
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), useTexture ? 1 : 0);
    if (useTexture && textureID != 0) {
        glUniform1i(glGetUniformLocation(shaderProgram, "diffuseTexture"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
}

std::string LightingAndReflection::getEnhancedPieceVertexShader() {
    return R"(
        #version 330 core
        layout(location=0) in vec3 position; 
        layout(location=1) in vec2 texCoord;
        layout(location=2) in vec3 normal;
        uniform mat4 M; 
        uniform mat4 itM; 
        uniform mat4 V; 
        uniform mat4 P;
        out vec3 vFrag; 
        out vec3 vNorm;
        out vec2 vTexCoord;
        out vec3 vWorldPos;
        void main(){ 
            vec4 w = M * vec4(position, 1.0); 
            vFrag = w.xyz; 
            vWorldPos = w.xyz;
            vNorm = mat3(itM) * normal; 
            vTexCoord = texCoord;
            gl_Position = P * V * w; 
        }
    )";
}

std::string LightingAndReflection::getEnhancedPieceFragmentShader() {
    return R"(
        #version 330 core
        in vec3 vFrag; 
        in vec3 vNorm; 
        in vec2 vTexCoord;
        in vec3 vWorldPos;
        out vec4 FragColor;
        uniform vec3 uViewPos; 
        uniform vec3 lightPos; 
        uniform vec3 baseCol;
        uniform sampler2D diffuseTexture;
        uniform samplerCube environmentMap;
        uniform bool useTexture;
        uniform float reflectionStrength;
        uniform float lightBrightness;
        void main(){ 
            vec3 N = normalize(vNorm); 
            vec3 L = normalize(lightPos - vFrag); 
            vec3 V = normalize(uViewPos - vFrag); 
            vec3 R = reflect(-L, N);
            
            // Calculate reflection vector for environment mapping
            vec3 I = normalize(vWorldPos - uViewPos);
            vec3 reflectionVector = reflect(I, N);
            
            float diff = max(dot(N, L), 0.0); 
            float spec = pow(max(dot(R, V), 0.0), 32.0); 
            float c = (0.2 + 0.7 * diff + 0.5 * spec) * lightBrightness; 
            
            vec3 finalColor;
            if(useTexture) {
                vec3 texColor = texture(diffuseTexture, vTexCoord).rgb;
                finalColor = texColor * (0.4 + 0.6 * c);
            } else {
                finalColor = baseCol * c;
            }
            
            // Add environment reflection
            vec3 reflectionColor = texture(environmentMap, reflectionVector).rgb;
            finalColor = mix(finalColor, reflectionColor, reflectionStrength);
            
            FragColor = vec4(finalColor, 1.0); 
        }
    )";
}

void LightingAndReflection::updateReflectionUniforms(unsigned int shaderProgram, 
                                      const glm::vec3& cameraPosition,
                                      unsigned int environmentMapID,
                                      float reflectionStrength) {
    // Set camera position
    glUniform3fv(glGetUniformLocation(shaderProgram, "uViewPos"), 1, glm::value_ptr(cameraPosition));
    
    // Set environment map
    glUniform1i(glGetUniformLocation(shaderProgram, "environmentMap"), 1); // Use texture unit 1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMapID);
    
    // Set reflection strength
    glUniform1f(glGetUniformLocation(shaderProgram, "reflectionStrength"), reflectionStrength);
}

float LightingAndReflection::getReflectionStrength(bool isWhite, bool isMetal) {
    if (isMetal) {
        return isWhite ? 0.4f : 0.5f; // White metal pieces have slightly less reflection
    } else {
        return isWhite ? 0.2f : 0.3f; // Non-metal pieces have subtle reflection
    }
}

std::string LightingAndReflection::getSkyboxVertexShader() {
    return R"(
        #version 330 core
        layout(location=0) in vec3 position;
        out vec3 TexCoords;
        uniform mat4 P;
        uniform mat4 V;
        void main() {
            TexCoords = position;
            vec4 pos = P * V * vec4(position, 1.0);
            gl_Position = pos.xyww;
        }
    )";
}

std::string LightingAndReflection::getSkyboxFragmentShader() {
    return R"(
        #version 330 core
        in vec3 TexCoords;
        out vec4 FragColor;
        uniform samplerCube skybox;
        void main() {
            FragColor = texture(skybox, TexCoords);
        }
    )";
}
