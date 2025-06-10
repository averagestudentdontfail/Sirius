#pragma once

#include <vector>
#include <string>
#include <memory>
#include "Math/Vec.h"

// Forward-declare OpenCL types
namespace cl { class Context; class CommandQueue; class Kernel; class Buffer; class Image2D; class Device; }
class IMetric;

// Ray struct matching the OpenCL kernel
struct Ray {
    Vec4 pos;
    Vec4 vel;
    int terminated;
    int sx, sy;
    int padding1, padding2;
};

class Renderer {
public:
    Renderer(int width, int height);
    ~Renderer();

    void render(IMetric* metric);
    unsigned int getOutputTexture() const;

private:
    void createResources(int width, int height);
    void compileKernel(IMetric* metric);
    void setupRays();
    void renderFallback();
    std::string generateCompilerOptions(IMetric* metric) const;

    int m_Width, m_Height;

    // OpenCL objects
    std::unique_ptr<cl::Context> m_Context;
    std::unique_ptr<cl::CommandQueue> m_Queue;
    std::unique_ptr<cl::Kernel> m_Kernel;
    std::unique_ptr<cl::Buffer> m_RayBuffer;
    std::unique_ptr<cl::Image2D> m_OutputImage;
    std::unique_ptr<cl::Device> m_Device;
    
    // OpenGL texture
    unsigned int m_OutputTextureID;

    // Platform detection
    bool m_IsPOCL = false;
    bool m_IsNVIDIA = false;
    bool m_HasRealOpenCL30 = false;

    std::vector<Ray> m_InitialRays;
    std::string m_LastMetricName;
};