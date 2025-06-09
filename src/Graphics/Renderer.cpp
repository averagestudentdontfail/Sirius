#include "Renderer.h"
#include "Physics/IMetric.h"
#include <glad/glad.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cmath>

// Temporarily disable OpenCL for compilation test
/*
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
*/

// Helper function to load kernel source from file
std::string loadKernelSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height), m_OutputTextureID(0) {
    // Simplified constructor for compilation test
    std::cout << "Renderer created with size: " << width << "x" << height << std::endl;
    
    // Create a simple OpenGL texture
    glGenTextures(1, &m_OutputTextureID);
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // Create a simple test pattern
    std::vector<float> testData(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 4;
            testData[index + 0] = static_cast<float>(x) / width;   // R
            testData[index + 1] = static_cast<float>(y) / height;  // G
            testData[index + 2] = 0.5f;                            // B
            testData[index + 3] = 1.0f;                            // A
        }
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, testData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

Renderer::~Renderer() {
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }
}

void Renderer::createResources(int width, int height) {
    // Placeholder for now
    m_Width = width;
    m_Height = height;
}

std::string Renderer::generateCompilerOptions(IMetric* metric) const {
    if (!metric) return "";
    
    auto tensor = metric->getMetricTensor(Vec4(0.0));
    std::string options = " -cl-mad-enable -cl-fast-relaxed-math";
    options += " -DMETRIC_G00=" + std::to_string(tensor[0][0].real);
    options += " -DMETRIC_G11=" + std::to_string(tensor[1][1].real);
    options += " -DMETRIC_G22=" + std::to_string(tensor[2][2].real);
    options += " -DMETRIC_G33=" + std::to_string(tensor[3][3].real);
    return options;
}

void Renderer::compileKernel(IMetric* metric) {
    if (!metric) return;
    
    std::cout << "Compiling kernel for metric: " << metric->getName() << std::endl;
    // Placeholder - will implement OpenCL kernel compilation later
    m_LastMetricName = metric->getName();
}

void Renderer::render(IMetric* metric) {
    if (!metric) return;
    
    // Simple test render - just update the texture with a time-based pattern
    static float time = 0.0f;
    time += 0.016f; // ~60 FPS
    
    std::vector<float> pixelData(m_Width * m_Height * 4);
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = (y * m_Width + x) * 4;
            float ndc_x = (2.0f * x / m_Width) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / m_Height);
            
            // Simple animated pattern
            pixelData[index + 0] = 0.5f + 0.5f * sin(ndc_x * 10.0f + time);  // R
            pixelData[index + 1] = 0.5f + 0.5f * sin(ndc_y * 10.0f + time);  // G
            pixelData[index + 2] = 0.5f + 0.5f * sin((ndc_x + ndc_y) * 5.0f + time);  // B
            pixelData[index + 3] = 1.0f;  // A
        }
    }
    
    // Update the OpenGL texture
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Renderer::getOutputTexture() const {
    return m_OutputTextureID;
}