#pragma once

#include <glm/glm.hpp>

// Forward declaration
class Camera;

// Camera modes
enum class CameraMode { 
    TOP, 
    DIAGONAL 
};

// Camera control system extracted from main.cpp
class CameraControl {
public:
    // Initialize camera control system
    static void initialize(Camera& camera);
    
    // Update orbit camera position based on current angles and radius
    static void updateOrbitCamera(Camera& camera);
    
    // Set camera mode and update camera position accordingly
    static void setCameraMode(Camera& camera, CameraMode mode, bool boardFlipped);
    
    // Handle mouse movement for orbit camera
    static void handleMouseMovement(Camera& camera, double xpos, double ypos, bool cameraLocked);
    
    // Handle mouse scroll for zoom
    static void handleMouseScroll(Camera& camera, double yoffset, bool cameraLocked);
    
    // Get current camera mode
    static CameraMode getCurrentCameraMode();
    
    // Set current camera mode
    static void setCurrentCameraMode(CameraMode mode);
    
    // Get camera lock state
    static bool isCameraLocked();
    
    // Set camera lock state
    static void setCameraLocked(bool locked);
    
    // Get orbit target
    static glm::vec3 getOrbitTarget();
    
    // Set orbit target
    static void setOrbitTarget(const glm::vec3& target);
    
    // Get orbit angles
    static float getOrbitYaw();
    static float getOrbitPitch();
    static float getOrbitRadius();
    
    // Set orbit angles
    static void setOrbitYaw(float yaw);
    static void setOrbitPitch(float pitch);
    static void setOrbitRadius(float radius);
    
private:
    // Camera state
    static glm::vec3 orbitTarget;
    static float orbitYawDeg;
    static float orbitPitchDeg;
    static float orbitRadius;
    static bool cameraLocked;
    static CameraMode currentCameraMode;
    
    // Mouse state
    static float lastX;
    static float lastY;
    static bool firstMouse;
};
