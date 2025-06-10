#include "Renderer.h"
#include "Physics/IMetric.h"
#include <glad/glad.h>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <cmath>
#include <chrono>
#include <regex>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenCL 3.0 includes
#ifdef _WIN32
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 300
    #define CL_HPP_MINIMUM_OPENCL_VERSION 200
    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4996)
    #endif
    #include <CL/opencl.hpp>
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#else
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 300
    #define CL_HPP_MINIMUM_OPENCL_VERSION 200
    #include <CL/opencl.hpp>
#endif

std::string loadKernelSource(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open kernel file: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

// Extract POCL version from platform version string
std::pair<int, int> extractPOCLVersion(const std::string& versionStr) {
    std::regex poclRegex(R"(PoCL\s+(\d+)\.(\d+))");
    std::smatch matches;
    if (std::regex_search(versionStr, matches, poclRegex)) {
        return {std::stoi(matches[1]), std::stoi(matches[2])};
    }
    return {0, 0}; // Unknown version
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height), m_OutputTextureID(0) {
    std::cout << "Initializing OpenCL Renderer..." << std::endl;
    
    try {
        // Get platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found!");
        }
        
        // Select platform and analyze capabilities
        cl::Platform platform = platforms[0];
        std::string platformName = platform.getInfo<CL_PLATFORM_NAME>();
        std::string platformVersion = platform.getInfo<CL_PLATFORM_VERSION>();
        
        bool isPOCL = (platformName.find("Portable Computing Language") != std::string::npos ||
                       platformName.find("POCL") != std::string::npos);
        bool isNVIDIA = (platformName.find("NVIDIA") != std::string::npos);
        
        // Check POCL version for true OpenCL 3.0 support
        bool hasRealOpenCL30 = false;
        if (isPOCL) {
            auto [major, minor] = extractPOCLVersion(platformVersion);
            hasRealOpenCL30 = (major >= 7); // Only POCL 7.0+ is truly OpenCL 3.0 conformant
            std::cout << "POCL " << major << "." << minor << " detected" << std::endl;
            if (!hasRealOpenCL30) {
                std::cout << "Warning: POCL < 7.0 has limited OpenCL 3.0 support" << std::endl;
            }
        } else if (isNVIDIA) {
            hasRealOpenCL30 = true; // Assume NVIDIA drivers support OpenCL 3.0
            std::cout << "NVIDIA OpenCL detected" << std::endl;
        }
        
        // Get devices
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
        if (devices.empty()) {
            throw std::runtime_error("No OpenCL devices found!");
        }
        
        m_Device = std::make_unique<cl::Device>(devices[0]);
        
        // Device info
        std::string deviceName = m_Device->getInfo<CL_DEVICE_NAME>();
        cl_uint computeUnits = m_Device->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
        
        std::cout << "Device: " << deviceName << " (" << computeUnits << " compute units)" << std::endl;
        
        // Create context and queue
        m_Context = std::make_unique<cl::Context>(*m_Device);
        m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, *m_Device);
        
        // Store platform info for later use
        m_IsPOCL = isPOCL;
        m_IsNVIDIA = isNVIDIA;
        m_HasRealOpenCL30 = hasRealOpenCL30;
        
        createResources(width, height);
        
        std::cout << "OpenCL Renderer initialized successfully!" << std::endl;
        
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        throw std::runtime_error("Failed to initialize OpenCL");
    }
}

Renderer::~Renderer() {
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }
}

void Renderer::createResources(int width, int height) {
    m_Width = width;
    m_Height = height;

    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }

    // Create OpenGL texture
    glGenTextures(1, &m_OutputTextureID);
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Create OpenCL resources
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    m_OutputImage = std::make_unique<cl::Image2D>(*m_Context, CL_MEM_WRITE_ONLY, format, width, height);

    m_InitialRays.resize(width * height);
    m_RayBuffer = std::make_unique<cl::Buffer>(*m_Context, CL_MEM_READ_ONLY, sizeof(Ray) * m_InitialRays.size());
}

std::string Renderer::generateCompilerOptions(IMetric* metric) const {
    if (!metric) return "";
    
    auto tensor = metric->getMetricTensor(Vec4(0.0, 0.0, 0.0, 0.0));
    
    std::string options;
    
    if (m_HasRealOpenCL30) {
        // Use OpenCL 3.0 for conformant implementations
        options = " -cl-std=CL3.0";
    } else {
        // Fall back to OpenCL 2.0 for limited implementations
        options = " -cl-std=CL2.0";
    }
    
    // Standard optimization flags supported by both POCL and NVIDIA
    options += " -cl-mad-enable";
    options += " -cl-fast-relaxed-math";
    options += " -cl-finite-math-only";
    options += " -cl-single-precision-constant";
    
    if (m_IsNVIDIA) {
        // NVIDIA-specific optimizations
        options += " -cl-denorms-are-zero";
        options += " -cl-unsafe-math-optimizations";
    } else if (m_IsPOCL) {
        // Conservative POCL optimizations
        options += " -cl-unsafe-math-optimizations";
    }
    
    // Metric tensor values
    options += " -DMETRIC_G00=" + std::to_string(tensor[0][0].real);
    options += " -DMETRIC_G11=" + std::to_string(tensor[1][1].real);
    options += " -DMETRIC_G22=" + std::to_string(tensor[2][2].real);
    options += " -DMETRIC_G33=" + std::to_string(tensor[3][3].real);
    
    return options;
}

void Renderer::compileKernel(IMetric* metric) {
    if (!metric) return;
    
    static bool compiling = false;
    if (compiling) return;
    compiling = true;
    
    try {
        std::string kernelSource = loadKernelSource("kernels/raytracer.cl");
        cl::Program program(*m_Context, kernelSource);
        std::string options = generateCompilerOptions(metric);
        
        try {
            program.build({*m_Device}, options.c_str());
        } catch (const cl::BuildError& err) {
            std::cerr << "Kernel build failed:" << std::endl;
            auto buildLog = err.getBuildLog();
            for (const auto& log : buildLog) {
                std::cerr << log.second << std::endl;
            }
            
            // Try minimal fallback
            std::string fallbackOptions = " -cl-std=CL1.2 -cl-mad-enable";
            fallbackOptions += " -DMETRIC_G00=" + std::to_string(metric->getMetricTensor(Vec4(0,0,0,0))[0][0].real);
            fallbackOptions += " -DMETRIC_G11=" + std::to_string(metric->getMetricTensor(Vec4(0,0,0,0))[1][1].real);
            fallbackOptions += " -DMETRIC_G22=" + std::to_string(metric->getMetricTensor(Vec4(0,0,0,0))[2][2].real);
            fallbackOptions += " -DMETRIC_G33=" + std::to_string(metric->getMetricTensor(Vec4(0,0,0,0))[3][3].real);
            
            program.build({*m_Device}, fallbackOptions.c_str());
            std::cout << "Using OpenCL 1.2 fallback compilation" << std::endl;
        }

        m_Kernel = std::make_unique<cl::Kernel>(program, "trace_rays");
        m_LastMetricName = metric->getName();
        
    } catch (const std::exception& err) {
        std::cerr << "Kernel compilation error: " << err.what() << std::endl;
        compiling = false;
        throw;
    }
    
    compiling = false;
}

void Renderer::render(IMetric* metric) {
    if (!metric) return;
    
    try {
        // Compile kernel if needed
        if (!m_Kernel || m_LastMetricName != metric->getName()) {
            compileKernel(metric);
        }
        
        if (!m_Kernel) {
            renderFallback();
            return;
        }
        
        // Setup rays
        setupRays();
        
        // Transfer data
        m_Queue->enqueueWriteBuffer(*m_RayBuffer, CL_FALSE, 0, sizeof(Ray) * m_InitialRays.size(), m_InitialRays.data());
        
        // Set arguments and execute
        m_Kernel->setArg(0, *m_RayBuffer);
        m_Kernel->setArg(1, *m_OutputImage);
        
        cl::NDRange globalSize(m_Width * m_Height);
        cl::NDRange localSize = m_IsPOCL ? cl::NDRange(64) : cl::NDRange(256);
        
        m_Queue->enqueueNDRangeKernel(*m_Kernel, cl::NullRange, globalSize, localSize);

        // Read result
        std::vector<float> pixelData(m_Width * m_Height * 4);
        cl::array<size_t, 3> origin = {0, 0, 0};
        cl::array<size_t, 3> region = {static_cast<size_t>(m_Width), static_cast<size_t>(m_Height), 1};
        
        m_Queue->enqueueReadImage(*m_OutputImage, CL_TRUE, origin, region, 0, 0, pixelData.data());
        
        // Update texture
        glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        
        m_Queue->finish();
        
    } catch (const cl::Error& err) {
        std::cerr << "Render error: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        renderFallback();
    }
}

void Renderer::setupRays() {
    static float time = 0.0f;
    time += 0.016f;
    
    const float fov = 60.0f * M_PI / 180.0f;
    const float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = y * m_Width + x;
            
            m_InitialRays[index].sx = x;
            m_InitialRays[index].sy = y;
            m_InitialRays[index].terminated = 0;
            m_InitialRays[index].padding1 = 0;
            m_InitialRays[index].padding2 = 0;
            
            m_InitialRays[index].pos = Vec4(0.0f, 0.0f, 0.0f, -5.0f);
            
            float ndc_x = (2.0f * x / static_cast<float>(m_Width)) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / static_cast<float>(m_Height));
            
            float px = ndc_x * tan(fov * 0.5f) * aspect;
            float py = ndc_y * tan(fov * 0.5f);
            
            Vec4 direction(1.0f, px, py, 1.0f);
            m_InitialRays[index].vel = glm::normalize(direction);
        }
    }
}

void Renderer::renderFallback() {
    static float time = 0.0f;
    time += 0.016f;
    
    std::vector<float> pixelData(m_Width * m_Height * 4);
    for (int y = 0; y < m_Height; ++y) {
        for (int x = 0; x < m_Width; ++x) {
            int index = (y * m_Width + x) * 4;
            float ndc_x = (2.0f * x / m_Width) - 1.0f;
            float ndc_y = 1.0f - (2.0f * y / m_Height);
            
            pixelData[index + 0] = 1.0f; // Red indicates fallback
            pixelData[index + 1] = 0.5f + 0.3f * sin(ndc_x * 5.0f + time);
            pixelData[index + 2] = 0.5f + 0.3f * sin(ndc_y * 5.0f + time);
            pixelData[index + 3] = 1.0f;
        }
    }
    
    glBindTexture(GL_TEXTURE_2D, m_OutputTextureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_FLOAT, pixelData.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

unsigned int Renderer::getOutputTexture() const {
    return m_OutputTextureID;
}