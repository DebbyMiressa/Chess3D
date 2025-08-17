#include "CameraControl.h"
#include "../../../camera.h"
#include <GLFW/glfw3.h>
#include <iostream>

// Initialize static members
glm::vec3 CameraControl::orbitTarget = glm::vec3(0.0f, -0.1f, 0.0f);
float CameraControl::orbitYawDeg = -90.0f;
float CameraControl::orbitPitchDeg = 20.0f;
float CameraControl::orbitRadius = 5.0f;
bool CameraControl::cameraLocked = false;
CameraMode CameraControl::currentCameraMode = CameraMode::TOP;
float CameraControl::lastX = 500.0f;
float CameraControl::lastY = 350.0f;
bool CameraControl::firstMouse = true;

void CameraControl::initialize(Camera& camera) {
    // Set initial camera position
    camera.Position = glm::vec3(0.0f, 1.5f, 5.0f);
    
    // Set initial camera mode
    setCameraMode(camera, CameraMode::DIAGONAL, true);
    
    std::cout << "Camera control system initialized" << std::endl;
}

void CameraControl::updateOrbitCamera(Camera& camera) {
    const float yaw = glm::radians(orbitYawDeg);
    const float pitch = glm::radians(orbitPitchDeg);
    const float x = orbitRadius * cosf(pitch) * cosf(yaw);
    const float y = orbitRadius * sinf(pitch);
    const float z = orbitRadius * cosf(pitch) * sinf(yaw);
    glm::vec3 eye = orbitTarget + glm::vec3(x, y, z);
    
    camera.Position = eye;
    camera.Front = glm::normalize(orbitTarget - eye);
    camera.Right = glm::normalize(glm::cross(camera.Front, glm::vec3(0,1,0)));
    camera.Up = glm::normalize(glm::cross(camera.Right, camera.Front));
}

void CameraControl::setCameraMode(Camera& camera, CameraMode mode, bool boardFlipped) {
    currentCameraMode = mode;
    float yawBase;
    
    switch (mode) {
        case CameraMode::TOP:
            yawBase = boardFlipped ? 90.0f : -90.0f;      // flip-aware top view
            orbitPitchDeg = 90.0f;
            orbitRadius = 6.0f;
            break;
        case CameraMode::DIAGONAL:
            yawBase = boardFlipped ? -45.0f : 135.0f;     // flip-aware diagonal (fixed)
            orbitPitchDeg = 30.0f;
            orbitRadius = 6.0f;
            break;
    }
    
    orbitYawDeg = yawBase;
    updateOrbitCamera(camera);
}

void CameraControl::handleMouseMovement(Camera& camera, double xpos, double ypos, bool cameraLocked) {
    if (cameraLocked) return; // Don't move camera if locked
    
    if (firstMouse) { 
        lastX = (float)xpos; 
        lastY = (float)ypos; 
        firstMouse = false; 
    }
    
    float dx = (float)xpos - lastX;
    float dy = (float)ypos - lastY;
    lastX = (float)xpos; 
    lastY = (float)ypos;
    
    const float sensitivity = 0.15f;
    orbitYawDeg += dx * sensitivity;
    orbitPitchDeg -= dy * sensitivity;
    
    if (orbitPitchDeg > 85.0f) orbitPitchDeg = 85.0f;
    if (orbitPitchDeg < -85.0f) orbitPitchDeg = -85.0f;
    
    updateOrbitCamera(camera);
}

void CameraControl::handleMouseScroll(Camera& camera, double yoffset, bool cameraLocked) {
    if (cameraLocked) return; // Don't zoom if camera is locked
    
    orbitRadius *= (yoffset > 0 ? 0.9f : 1.1f);
    if (orbitRadius < 2.0f) orbitRadius = 2.0f;
    if (orbitRadius > 12.0f) orbitRadius = 12.0f;
    
    updateOrbitCamera(camera);
}

CameraMode CameraControl::getCurrentCameraMode() {
    return currentCameraMode;
}

void CameraControl::setCurrentCameraMode(CameraMode mode) {
    currentCameraMode = mode;
}

bool CameraControl::isCameraLocked() {
    return cameraLocked;
}

void CameraControl::setCameraLocked(bool locked) {
    cameraLocked = locked;
    std::cout << "Camera " << (cameraLocked ? "locked" : "unlocked") << std::endl;
}

glm::vec3 CameraControl::getOrbitTarget() {
    return orbitTarget;
}

void CameraControl::setOrbitTarget(const glm::vec3& target) {
    orbitTarget = target;
}

float CameraControl::getOrbitYaw() {
    return orbitYawDeg;
}

float CameraControl::getOrbitPitch() {
    return orbitPitchDeg;
}

float CameraControl::getOrbitRadius() {
    return orbitRadius;
}

void CameraControl::setOrbitYaw(float yaw) {
    orbitYawDeg = yaw;
}

void CameraControl::setOrbitPitch(float pitch) {
    orbitPitchDeg = pitch;
}

void CameraControl::setOrbitRadius(float radius) {
    orbitRadius = radius;
}
