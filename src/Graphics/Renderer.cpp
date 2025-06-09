#include "Renderer.h"
#include "Physics/IMetric.h"
#include <glad/glad.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cmath>

// OpenCL includes
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>

// Helper function to load kernel source from file
std::string loadKernelSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height), m_OutputTextureID(0) {
    std::cout << "Initializing OpenCL Renderer (" << width << "x" << height << ")..." << std::endl;
    
    try {
        // 1. Initialize OpenCL Platform and Device
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found.");
        }
        
        cl::Platform platform = platforms.front();
        std::cout << "Using OpenCL Platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
        
        // Try to get GPU device first, fallback to CPU
        std::vector<cl::Device> devices;
        try {
            platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
        } catch (const cl::Error&) {
            std::cout << "No GPU devices found, trying CPU..." << std::endl;
            platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        }
        
        if (devices.empty()) {
            throw std::runtime_error("No OpenCL devices found.");
        }
        
        m_Device = devices.front();
        std::cout << "Using OpenCL device: " << m_Device.getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Device type: " << (m_Device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU ? "GPU" : "CPU") << std::endl;
        
        // 2. Create OpenCL Context
        m_Context = std::make_unique<cl::Context>(m_Device);
        
        // 3. Create Command Queue
        m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, m_Device);
        
        // 4. Create GPU resources
        createResources(width, height);
        
        std::cout << "OpenCL Renderer initialized successfully!" << std::endl;
        
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        throw std::runtime_error("Failed to initialize OpenCL");
    } catch (const std::exception& err) {
        std::cerr << "Error initializing OpenCL Renderer: " << err.what() << std::endl;
        throw;
    }
}

Renderer::~Renderer() {
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }
    std::cout << "OpenCL Renderer destroyed." << std::endl;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create OpenCL image for output (separate from GL for compatibility)
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    m_OutputImage = std::make_unique<cl::Image2D>(*m_Context, CL_MEM_WRITE_ONLY, format, width, height);

    // Create buffer for rays
    m_InitialRays.resize(width * height);
    m_RayBuffer = std::make_unique<cl::Buffer>(*m_Context, CL_MEM_READ_ONLY, sizeof(Ray) * m_InitialRays.size());
    
    std::cout << "OpenCL resources created for " << width << "x" << height << " (" << m_InitialRays.size() << " rays)" << std::endl;
}

std::string Renderer::generateCompilerOptions(IMetric* metric) const {
    if (!metric) return "";
    
    auto tensor = metric->getMetricTensor(Vec4(0.0, 0.0, 0.0, 0.0)); // Get tensor at origin
    
    std::string options = " -cl-mad-enable -cl-fast-relaxed-math";
    options += " -DMETRIC_G00=" + std::to_string(tensor[0][0].real);
    options += " -DMETRIC_G11=" + std::to_string(tensor[1][1].real);
    options += " -DMETRIC_G22=" + std::to_string(tensor[2][2].real);
    options += " -DMETRIC_G33=" + std::to_string(tensor[3][3].real);
    
    std::cout << "Compiler options: " << options << std::endl;
    return options;
}

void Renderer::compileKernel(IMetric* metric) {
    if (!metric) return;
    
    try {
        std::cout << "Compiling OpenCL kernel for metric: " << metric->getName() << std::endl;
        
        std::string kernelSource = loadKernelSource("kernels/raytracer.cl");
        std::cout << "Loaded kernel source (" << kernelSource.length() << " characters)" << std::endl;
        
        cl::Program program(*m_Context, kernelSource);
        std::string options = generateCompilerOptions(metric);
        
        try {
            program.build({m_Device}, options.c_str());
            std::cout << "Kernel compiled successfully!" << std::endl;
        } catch (const cl::BuildError& err) {
            std::cerr << "OpenCL Kernel Build Error:" << std::endl;
            for (const auto& log : err.getBuildLog()) {
                std::cerr << "Device: " << log.first.getInfo<CL_DEVICE_NAME>() << std::endl;
                std::cerr << "Build Log:\n" << log.second << std::endl;
            }
            throw;
        }

        m_Kernel = std::make_unique<cl::Kernel>(program, "trace_rays");
        m_LastMetricName = metric->getName();
        
        std::cout << "Kernel created successfully!" << std::endl;
        
    } catch (const std::exception& err) {
        std::cerr << "Error compiling kernel: " << err.what() << std::endl;
        throw;
    }
}

void Renderer::render(IMetric* metric) {
    if (!metric) return;
    
    try {
        // Re-compile kernel if the metric has changed
        if (!m_Kernel || m_LastMetricName != metric->getName()) {
            compileKernel(metric);
        }
        
        // 1. Setup initial rays on the CPU (simple perspective camera)
        setupRays();
        
        // 2. Transfer ray data from CPU to GPU
        m_Queue->enqueueWriteBuffer(*m_RayBuffer, CL_FALSE, 0, sizeof(Ray) * m_InitialRays.size(), m_InitialRays.data());
        
        // 3. Set kernel arguments
        m_Kernel->setArg(0, *m_RayBuffer);
        m_Kernel->setArg(1, *m_OutputImage);
        
        // 4. Execute the kernel
        cl::NDRange globalSize(m_Width * m_Height);
        m_Queue->enqueueNDRangeKernel(*m_Kernel, cl::NullRange, globalSize, cl::NullRange);

        // 5. Read the result back from OpenCL and update the OpenGL texture
        std::vector<float> pixelData(m_Width * m_Height * 4); // RGBA
        cl::array<size_t, 3> origin = {0, 0, 0};
        cl::array<size_t, 3> region = {static_cast<size_t>(m_Width), static_cast<size_t>(m_Height), 1};
        
        m_Queue->enqueueReadImage(*m_OutputImage, CL_TRUE, origin, region, 0, 0, pixelData.data());
        
        // 6. Update the OpenGL texture with the new data
        glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        
        // Ensure all operations complete
        m_Queue->finish();
        
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Runtime Error in render(): " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        // Fallback to a simple pattern if OpenCL fails
        renderFallback();
    } catch (const std::exception& err) {
        std::cerr << "Error in render(): " << err.what() << std::endl;
        renderFallback();
    }
}

void Renderer::setupRays() {
    // Setup camera parameters
    static float time = 0.0f;
    time += 0.016f; // ~60 FPS
    
    const float fov = 60.0f * M_PI / 180.0f; // 60 degrees in radians
    const float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = y * m_Width + x;
            
            // Set ray pixel coordinates
            m_InitialRays[index].sx = x;
            m_InitialRays[index].sy = y;
            m_InitialRays[index].terminated = 0;
            m_InitialRays[index].padding1 = 0;
            m_InitialRays[index].padding2 = 0;
            
            // Camera position in spacetime (t, x, y, z)
            m_InitialRays[index].pos = Vec4(0.0f, 0.0f, 0.0f, -5.0f);
            
            // Convert pixel coords to normalized device coords [-1, 1]
            float ndc_x = (2.0f * x / static_cast<float>(m_Width)) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / static_cast<float>(m_Height));
            
            // Apply field of view and aspect ratio
            float px = ndc_x * tan(fov * 0.5f) * aspect;
            float py = ndc_y * tan(fov * 0.5f);
            
            // Ray direction in spacetime (dt/dλ, dx/dλ, dy/dλ, dz/dλ)
            // For now, simple forward-pointing rays with slight time component
            Vec4 direction(1.0f, px, py, 1.0f); // Moving forward in time and space
            m_InitialRays[index].vel = glm::normalize(direction);
        }
    }
}

void Renderer::renderFallback() {
    // Simple fallback rendering in case OpenCL fails
    static float time = 0.0f;
    time += 0.016f;
    
    std::vector<float> pixelData(m_Width * m_Height * 4);
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = (y * m_Width + x) * 4;
            float ndc_x = (2.0f * x / m_Width) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / m_Height);
            
            // Fallback pattern (red tinted to indicate fallback mode)
            pixelData[index + 0] = 1.0f; // R - red tint indicates fallback
            pixelData[index + 1] = 0.5f + 0.3f * sin(ndc_x * 5.0f + time);  // G
            pixelData[index + 2] = 0.5f + 0.3f * sin(ndc_y * 5.0f + time);  // B
            pixelData[index + 3] = 1.0f;  // A
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    
    std::cout << "Using fallback rendering (OpenCL failed)" << std::endl;
}

unsigned int Renderer::getOutputTexture() const {
    return m_OutputTextureID;
}