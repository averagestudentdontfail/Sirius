#include "Renderer.h"
#include "Physics/IMetric.h"
#include <glad/glad.h>  // Must come before OpenCL includes for proper GL interop

// We need to include the OpenCL C++ bindings.
// You might need to point your include path to where the SDK is installed.
// For now, let's assume it's findable.
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>

#include <iostream>
#include <fstream>

// Helper function to load kernel source from file
std::string loadKernelSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height) {
    // 1. Initialize OpenCL Platform and Device
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    if (platforms.empty()) {
        throw std::runtime_error("No OpenCL platforms found.");
    }
    cl::Platform platform = platforms.front();
    
    std::vector<cl::Device> devices;
    platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    if (devices.empty()) {
        // Fallback to CPU if no GPU available
        platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        if (devices.empty()) {
            throw std::runtime_error("No OpenCL devices found.");
        }
        std::cout << "No GPU found, using CPU device." << std::endl;
    }
    cl::Device device = devices.front();
    std::cout << "Using OpenCL device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

    // 2. Create OpenCL Context (simplified without GL interop for now)
    m_Context = std::make_unique<cl::Context>(device);

    // 3. Create Command Queue
    m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, device);

    // 4. Create GPU resources (buffers, images)
    createResources(width, height);
}

Renderer::~Renderer() {
    // Clean up OpenGL resources
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }
}

void Renderer::createResources(int width, int height) {
    m_Width = width;
    m_Height = height;

    // Clean up old texture if it exists
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }

    // Create an OpenGL texture for output
    glGenTextures(1, &m_OutputTextureID);
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create an OpenCL image for output (separate from GL texture for now)
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    m_OutputImage = std::make_unique<cl::Image2D>(*m_Context, CL_MEM_WRITE_ONLY, format, width, height);

    // Create a buffer for the rays
    m_InitialRays.resize(width * height);
    m_RayBuffer = std::make_unique<cl::Buffer>(*m_Context, CL_MEM_READ_ONLY, sizeof(Ray) * m_InitialRays.size());
}

std::string Renderer::generateCompilerOptions(IMetric* metric) const {
    auto tensor = metric->getMetricTensor(Vec4(0.0)); // Get tensor at origin for now
    
    std::string options = " -cl-mad-enable -cl-fast-relaxed-math";
    options += " -DMETRIC_G00=" + std::to_string(tensor[0][0].real);
    options += " -DMETRIC_G11=" + std::to_string(tensor[1][1].real);
    options += " -DMETRIC_G22=" + std::to_string(tensor[2][2].real);
    options += " -DMETRIC_G33=" + std::to_string(tensor[3][3].real);
    
    return options;
}

void Renderer::compileKernel(IMetric* metric) {
    std::cout << "Compiling kernel for metric: " << metric->getName() << std::endl;
    std::string kernelSource = loadKernelSource("kernels/raytracer.cl");
    cl::Program program(*m_Context, kernelSource);

    std::string options = generateCompilerOptions(metric);
    
    try {
        program.build(options.c_str());
    } catch (const cl::BuildError& err) {
        std::cerr << "OpenCL Kernel Build Error: " << std::endl;
        for (const auto& log : err.getBuildLog()) {
            std::cerr << log.second << std::endl;
        }
        throw;
    }

    m_Kernel = std::make_unique<cl::Kernel>(program, "trace_rays");
    m_LastMetricName = metric->getName();
}

void Renderer::render(IMetric* metric) {
    if (!metric) return;
    
    // Re-compile kernel if the metric has changed
    if (!m_Kernel || m_LastMetricName != metric->getName()) {
        compileKernel(metric);
    }
    
    // 1. Setup initial rays on the CPU (simple camera for now)
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = y * m_Width + x;
            m_InitialRays[index].sx = x;
            m_InitialRays[index].sy = y;
            m_InitialRays[index].pos = Vec4(0.0, 0.0, 0.0, -5.0); // Camera at z = -5
            m_InitialRays[index].terminated = 0;
            m_InitialRays[index].padding1 = 0;
            m_InitialRays[index].padding2 = 0;
            
            // Convert pixel coords to normalized device coords [-1, 1]
            float ndc_x = (2.0f * x / static_cast<float>(m_Width)) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / static_cast<float>(m_Height));
            
            m_InitialRays[index].vel = glm::normalize(Vec4(0.0, ndc_x, ndc_y, 1.0)); // Ray direction (t,x,y,z)
        }
    }
    
    // 2. Transfer ray data from CPU to GPU
    m_Queue->enqueueWriteBuffer(*m_RayBuffer, CL_TRUE, 0, sizeof(Ray) * m_InitialRays.size(), m_InitialRays.data());
    
    // 3. Set kernel arguments
    m_Kernel->setArg(0, *m_RayBuffer);
    m_Kernel->setArg(1, *m_OutputImage);
    
    // 4. Execute the kernel
    m_Queue->enqueueNDRangeKernel(*m_Kernel, cl::NullRange, cl::NDRange(m_Width * m_Height), cl::NullRange);

    // 5. Read the result back from OpenCL and update the OpenGL texture
    std::vector<float> pixelData(m_Width * m_Height * 4); // RGBA
    cl::array<size_t, 3> origin = {0, 0, 0};
    cl::array<size_t, 3> region = {static_cast<size_t>(m_Width), static_cast<size_t>(m_Height), 1};
    
    m_Queue->enqueueReadImage(*m_OutputImage, CL_TRUE, origin, region, 0, 0, pixelData.data());
    
    // Update the OpenGL texture with the new data
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    m_Queue->finish();
}

unsigned int Renderer::getOutputTexture() const {
    return m_OutputTextureID;
}