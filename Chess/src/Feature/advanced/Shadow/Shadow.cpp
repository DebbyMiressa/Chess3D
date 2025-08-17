#include "Shadow.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

bool Shadow::initialized = false;
glm::mat4 Shadow::lightProjection = glm::mat4(1.0f);
glm::mat4 Shadow::lightView = glm::mat4(1.0f);

void Shadow::initialize() {
    if (initialized) return;
    
    std::cout << "Shadow system initialized" << std::endl;
    initialized = true;
}

std::string Shadow::getShadowVertexShader() {
    return R"(
        #version 330 core
        layout(location=0) in vec3 position;
        layout(location=1) in vec2 texCoord;
        layout(location=2) in vec3 normal;
        uniform mat4 M;
        uniform mat4 lightSpaceMatrix;
        void main() {
            gl_Position = lightSpaceMatrix * M * vec4(position, 1.0);
        }
    )";
}

std::string Shadow::getShadowFragmentShader() {
    return R"(
        #version 330 core
        void main() {
            // Fragment shader for shadow mapping - just output depth
        }
    )";
}

std::string Shadow::getShadowReceiverVertexShader() {
    return R"(
        #version 330 core
        layout(location=0) in vec3 position; 
        layout(location=1) in vec2 texCoord;
        layout(location=2) in vec3 normal;
        uniform mat4 M; 
        uniform mat4 itM; 
        uniform mat4 V; 
        uniform mat4 P;
        uniform mat4 lightSpaceMatrix;
        out vec3 vFrag; 
        out vec3 vNorm;
        out vec2 vTexCoord;
        out vec4 vFragPosLightSpace;
        void main(){ 
            vec4 w = M * vec4(position, 1.0); 
            vFrag = w.xyz; 
            vNorm = mat3(itM) * normal; 
            vTexCoord = texCoord;
            vFragPosLightSpace = lightSpaceMatrix * w;
            gl_Position = P * V * w; 
        }
    )";
}

std::string Shadow::getShadowReceiverFragmentShader() {
    return R"(
        #version 330 core
        in vec3 vFrag; 
        in vec3 vNorm; 
        in vec2 vTexCoord;
        in vec4 vFragPosLightSpace;
        out vec4 FragColor;
        uniform vec3 uViewPos; 
        uniform vec3 lightPos; 
        uniform vec3 baseCol;
        uniform sampler2D diffuseTexture;
        uniform sampler2D shadowMap;
        uniform bool useTexture;
        uniform bool useShadows;
        
        float ShadowCalculation(vec4 fragPosLightSpace) {
            // Perspective divide
            vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
            
            // Transform to [0,1] range
            projCoords = projCoords * 0.5 + 0.5;
            
            // Get closest depth value from light's perspective
            float closestDepth = texture(shadowMap, projCoords.xy).r;
            
            // Get current depth
            float currentDepth = projCoords.z;
            
            // Check whether current frag pos is in shadow
            float bias = 0.005;
            float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
            
            return shadow;
        }
        
        void main(){ 
            vec3 N = normalize(vNorm); 
            vec3 L = normalize(lightPos - vFrag); 
            vec3 V = normalize(uViewPos - vFrag); 
            vec3 R = reflect(-L, N);
            float diff = max(dot(N, L), 0.0); 
            float spec = pow(max(dot(R, V), 0.0), 32.0); 
            float c = 0.2 + 0.7 * diff + 0.5 * spec; 
            
            vec3 finalColor;
            if(useTexture) {
                vec3 texColor = texture(diffuseTexture, vTexCoord).rgb;
                finalColor = texColor * (0.4 + 0.6 * c);
            } else {
                finalColor = baseCol * c;
            }
            
            // Apply shadows
            if(useShadows) {
                float shadow = ShadowCalculation(vFragPosLightSpace);
                finalColor = finalColor * (1.0 - shadow * 0.85); // Very strong shadows for dramatic effect
            }
            
            FragColor = vec4(finalColor, 1.0); 
        }
    )";
}

glm::mat4 Shadow::getLightSpaceMatrix(const glm::vec3& lightPos,
                                     const glm::vec3& lightDir,
                                     float nearPlane,
                                     float farPlane) {
    // Create light projection matrix (orthographic for directional light)
    // Smaller frustum for closer light to capture more detail
    float left = -6.0f;
    float right = 6.0f;
    float bottom = -6.0f;
    float top = 6.0f;
    
    lightProjection = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    
    // Create light view matrix
    lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
    
    return lightProjection * lightView;
}

void Shadow::generateShadowMap(unsigned int shadowMapFBO, 
                             const glm::vec3& lightPos,
                             const glm::vec3& lightDir,
                             float nearPlane,
                             float farPlane) {
    // Bind shadow map framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Configure depth test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void Shadow::updateShadowUniforms(unsigned int shaderProgram,
                                unsigned int shadowMap,
                                const glm::mat4& lightSpaceMatrix) {
    // Set shadow map texture
    glUniform1i(glGetUniformLocation(shaderProgram, "shadowMap"), 2); // Use texture unit 2
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    
    // Set light space matrix
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
    
    // Enable shadows
    glUniform1i(glGetUniformLocation(shaderProgram, "useShadows"), 1);
}

unsigned int Shadow::createShadowMapFBO() {
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    
    // Create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    // Attach depth texture to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    return depthMapFBO;
}

void Shadow::cleanup() {
    initialized = false;
}
