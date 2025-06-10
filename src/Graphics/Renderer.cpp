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

// OpenCL includes - Platform specific
#ifdef _WIN32
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 200
    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4996) // Disable deprecated warnings
    #endif
    // Try different OpenCL header locations for Windows
    #if __has_include(<CL/cl2.hpp>)
        #include <CL/cl2.hpp>
    #elif __has_include(<CL/opencl.hpp>)
        #include <CL/opencl.hpp>
    #elif __has_include(<opencl.hpp>)
        #include <opencl.hpp>
    #else
        // Fallback to C-style headers if C++ headers not available
        #include <CL/cl.h>
        // Define basic cl namespace for compatibility
        namespace cl {
            using Platform = cl_platform_id;
            using Device = cl_device_id;
            using Context = cl_context;
            using CommandQueue = cl_command_queue;
            using Kernel = cl_kernel;
            using Buffer = cl_mem;
            using Image2D = cl_mem;
            using Program = cl_program;
            using Error = std::runtime_error;
            class BuildError : public std::runtime_error {
            public:
                BuildError(const char* msg) : std::runtime_error(msg) {}
            };
        }
    #endif
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#else
    // Linux OpenCL headers
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 200
    #include <CL/opencl.hpp>
#endif

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
        // 1. Initialize OpenCL Platform
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);
        
        if (platforms.empty()) {
            throw std::runtime_error("No OpenCL platforms found.");
        }
        
        std::cout << "Found " << platforms.size() << " OpenCL platform(s):" << std::endl;
        for (size_t i = 0; i < platforms.size(); ++i) {
            try {
                std::string platformName = platforms[i].getInfo<CL_PLATFORM_NAME>();
                std::cout << "  Platform " << i << ": " << platformName << std::endl;
            } catch (const cl::Error& err) {
                std::cout << "  Platform " << i << ": <name unavailable>" << std::endl;
            }
        }
        
        // Platform selection logic
        cl::Platform platform;
        bool platformSelected = false;
        
#ifdef _WIN32
        // On Windows, prefer NVIDIA CUDA or Intel OpenCL platforms
        for (auto& p : platforms) {
            std::string name = p.getInfo<CL_PLATFORM_NAME>();
            if (name.find("NVIDIA") != std::string::npos || 
                name.find("CUDA") != std::string::npos ||
                name.find("Intel") != std::string::npos) {
                platform = p;
                platformSelected = true;
                std::cout << "Selected GPU platform: " << name << std::endl;
                break;
            }
        }
#else
        // On Linux, prefer any available platform (likely POCL)
        platform = platforms[0];
        platformSelected = true;
        std::cout << "Selected platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
#endif
        
        if (!platformSelected) {
            platform = platforms[0];
            std::cout << "Using first available platform: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
        }
        
        // 2. Get OpenCL Devices
        std::vector<cl::Device> devices;
        bool deviceFound = false;
        
        // Try different device types in order of preference
#ifdef _WIN32
        // On Windows, strongly prefer GPU devices
        std::vector<cl_device_type> deviceTypes = {CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_ALL, CL_DEVICE_TYPE_DEFAULT, CL_DEVICE_TYPE_CPU};
#else
        // On Linux, try all device types (POCL usually shows as CPU)
        std::vector<cl_device_type> deviceTypes = {CL_DEVICE_TYPE_ALL, CL_DEVICE_TYPE_GPU, CL_DEVICE_TYPE_CPU, CL_DEVICE_TYPE_DEFAULT};
#endif
        
        for (auto deviceType : deviceTypes) {
            try {
                platform.getDevices(deviceType, &devices);
                if (!devices.empty()) {
                    deviceFound = true;
                    std::string typeStr = (deviceType == CL_DEVICE_TYPE_GPU) ? "GPU" :
                                         (deviceType == CL_DEVICE_TYPE_CPU) ? "CPU" :
                                         (deviceType == CL_DEVICE_TYPE_ALL) ? "ALL" : "DEFAULT";
                    std::cout << "Found " << devices.size() << " device(s) with CL_DEVICE_TYPE_" << typeStr << std::endl;
                    break;
                }
            } catch (const cl::Error& err) {
                // Continue to next device type
            }
        }
        
        if (!deviceFound || devices.empty()) {
            throw std::runtime_error("No OpenCL devices found on platform: " + platform.getInfo<CL_PLATFORM_NAME>());
        }
        
        // 3. Select and display device information
        m_Device = std::make_unique<cl::Device>(devices[0]);
        
        try {
            std::string deviceName = m_Device->getInfo<CL_DEVICE_NAME>();
            cl_device_type deviceType = m_Device->getInfo<CL_DEVICE_TYPE>();
            cl_uint computeUnits = m_Device->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
            cl_bool available = m_Device->getInfo<CL_DEVICE_AVAILABLE>();
            
            std::cout << "Selected device: " << deviceName << std::endl;
            std::cout << "Device type: " << (deviceType == CL_DEVICE_TYPE_GPU ? "GPU" : 
                                            deviceType == CL_DEVICE_TYPE_CPU ? "CPU" : "Other") << std::endl;
            std::cout << "Compute units: " << computeUnits << std::endl;
            std::cout << "Device available: " << (available ? "Yes" : "No") << std::endl;
            
#ifdef _WIN32
            if (deviceType == CL_DEVICE_TYPE_GPU) {
                std::cout << "🚀 GPU OpenCL detected - expect 60-120 FPS performance!" << std::endl;
            } else {
                std::cout << "⚠️  CPU OpenCL - performance will be limited (~10-20 FPS)" << std::endl;
            }
#else
            std::cout << "🐧 Linux OpenCL (likely POCL) - expect ~10-20 FPS performance" << std::endl;
#endif
            
            if (!available) {
                throw std::runtime_error("Selected OpenCL device is not available");
            }
            
        } catch (const cl::Error& err) {
            std::cout << "Warning: Could not get complete device info: " << err.what() << std::endl;
            // Continue anyway - the device might still work
        }
        
        // 4. Create OpenCL Context
        try {
            m_Context = std::make_unique<cl::Context>(*m_Device);
            std::cout << "OpenCL context created successfully" << std::endl;
        } catch (const cl::Error& err) {
            std::cerr << "Failed to create OpenCL context: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
            throw;
        }
        
        // 5. Create Command Queue
        try {
            m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, *m_Device);
            std::cout << "OpenCL command queue created successfully" << std::endl;
        } catch (const cl::Error& err) {
            std::cerr << "Failed to create OpenCL command queue: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
            throw;
        }
        
        // 6. Create GPU resources
        createResources(width, height);
        
        std::cout << "OpenCL Renderer initialized successfully!" << std::endl;
        
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        throw std::runtime_error("Failed to initialize OpenCL: " + std::string(err.what()));
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
            program.build({*m_Device}, options.c_str());
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
        
#ifdef _WIN32
        // On Windows with GPU, use larger work groups
        cl::NDRange localSize(256);
#else
        // On Linux with CPU, use smaller work groups
        cl::NDRange localSize(64);
#endif
        
        m_Queue->enqueueNDRangeKernel(*m_Kernel, cl::NullRange, globalSize, localSize);

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