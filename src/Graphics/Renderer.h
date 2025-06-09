#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Math/Vec.h"   // For Vec4 type

// Forward-declare OpenCL types
namespace cl { class Context; class CommandQueue; class Kernel; class Buffer; class Image2D; }
class IMetric;

// The C++ equivalent of the OpenCL Ray struct
struct Ray {
    Vec4 pos;
    Vec4 vel;
    int terminated;
    int sx, sy;
    // Padding to ensure alignment with OpenCL's float4
    int padding1, padding2;
};

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    // The main render function
    void render(IMetric* metric);
    
    // Returns the OpenGL handle for the output texture to be displayed
    unsigned int getOutputTexture() const;

private:
    void createResources(int width, int height);
    void compileKernel(IMetric* metric);
    std::string generateCompilerOptions(IMetric* metric) const;

    int m_Width, m_Height;

    // OpenCL objects (temporarily disabled)
    // std::unique_ptr<cl::Context> m_Context;
    // std::unique_ptr<cl::CommandQueue> m_Queue;
    // std::unique_ptr<cl::Kernel> m_Kernel;
    // std::unique_ptr<cl::Buffer> m_RayBuffer;
    // std::unique_ptr<cl::Image2D> m_OutputImage;
    
    // OpenGL texture for output
    unsigned int m_OutputTextureID;

    std::vector<Ray> m_InitialRays;
    std::string m_LastMetricName;
};