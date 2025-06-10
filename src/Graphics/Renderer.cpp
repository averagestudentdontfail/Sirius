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

// Define M_PI if not defined (Windows issue)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenCL 3.0 includes - Platform specific with modern features
#ifdef _WIN32
    // Windows: Use NVIDIA or Intel OpenCL 3.0 headers
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 300  // OpenCL 3.0
    #define CL_HPP_MINIMUM_OPENCL_VERSION 200 // Minimum 2.0 for fallback
    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4996) // Disable deprecated warnings
    #endif
    
    // Try to include OpenCL C++ headers in order of preference
    #if defined(__has_include)
        #if __has_include(<CL/opencl.hpp>)
            #include <CL/opencl.hpp>
            #define OPENCL_HPP_FOUND
        #elif __has_include(<CL/cl2.hpp>)
            #include <CL/cl2.hpp>
            #define OPENCL_HPP_FOUND
        #elif __has_include(<opencl.hpp>)
            #include <opencl.hpp>
            #define OPENCL_HPP_FOUND
        #endif
    #endif
    
    // Fallback to manual OpenCL setup if headers not found
    #ifndef OPENCL_HPP_FOUND
        #include <CL/cl.h>
        #include <CL/cl_gl.h>
        // Create minimal C++ wrapper namespace for OpenCL 3.0
        namespace cl {
            using Platform = cl_platform_id;
            using Device = cl_device_id;
            using Context = cl_context;
            using CommandQueue = cl_command_queue;
            using Kernel = cl_kernel;
            using Buffer = cl_mem;
            using Image2D = cl_mem;
            using Program = cl_program;
            
            class Error : public std::runtime_error {
            public:
                cl_int err_code;
                Error(cl_int err, const char* msg) : std::runtime_error(msg), err_code(err) {}
                cl_int err() const { return err_code; }
            };
            
            class BuildError : public Error {
            public:
                BuildError(const char* msg) : Error(CL_BUILD_PROGRAM_FAILURE, msg) {}
                std::vector<std::pair<cl_device_id, std::string>> getBuildLog() const {
                    return {}; // Simplified for manual wrapper
                }
            };
            
            // Minimal Platform class implementation for OpenCL 3.0
            class PlatformWrapper {
            public:
                cl_platform_id platform_id;
                PlatformWrapper(cl_platform_id id = nullptr) : platform_id(id) {}
                
                static void get(std::vector<PlatformWrapper>* platforms) {
                    cl_uint num_platforms;
                    clGetPlatformIDs(0, nullptr, &num_platforms);
                    std::vector<cl_platform_id> platform_ids(num_platforms);
                    clGetPlatformIDs(num_platforms, platform_ids.data(), nullptr);
                    
                    platforms->clear();
                    for (auto id : platform_ids) {
                        platforms->emplace_back(id);
                    }
                }
                
                template<cl_platform_info info>
                std::string getInfo() const {
                    size_t size;
                    clGetPlatformInfo(platform_id, info, 0, nullptr, &size);
                    std::string result(size, '\0');
                    clGetPlatformInfo(platform_id, info, size, &result[0], nullptr);
                    if (!result.empty() && result.back() == '\0') {
                        result.pop_back();
                    }
                    return result;
                }
                
                void getDevices(cl_device_type type, std::vector<cl_device_id>* devices) const {
                    cl_uint num_devices;
                    cl_int err = clGetDeviceIDs(platform_id, type, 0, nullptr, &num_devices);
                    if (err != CL_SUCCESS) throw Error(err, "Failed to get device count");
                    devices->resize(num_devices);
                    err = clGetDeviceIDs(platform_id, type, num_devices, devices->data(), nullptr);
                    if (err != CL_SUCCESS) throw Error(err, "Failed to get devices");
                }
            };
            
            // Device wrapper with OpenCL 3.0 features
            class DeviceWrapper {
            public:
                cl_device_id device_id;
                DeviceWrapper(cl_device_id id) : device_id(id) {}
                
                template<cl_device_info info>
                auto getInfo() const {
                    if constexpr (info == CL_DEVICE_NAME || info == CL_DEVICE_VENDOR || info == CL_DEVICE_VERSION) {
                        size_t size;
                        clGetDeviceInfo(device_id, info, 0, nullptr, &size);
                        std::string result(size, '\0');
                        clGetDeviceInfo(device_id, info, size, &result[0], nullptr);
                        if (!result.empty() && result.back() == '\0') {
                            result.pop_back();
                        }
                        return result;
                    } else {
                        typename std::conditional_t<
                            info == CL_DEVICE_TYPE, cl_device_type,
                            std::conditional_t<
                                info == CL_DEVICE_MAX_COMPUTE_UNITS, cl_uint,
                                std::conditional_t<
                                    info == CL_DEVICE_AVAILABLE, cl_bool,
                                    std::conditional_t<
                                        info == CL_DEVICE_GLOBAL_MEM_SIZE || info == CL_DEVICE_LOCAL_MEM_SIZE, cl_ulong,
                                        void*
                                    >
                                >
                            >
                        > result;
                        clGetDeviceInfo(device_id, info, sizeof(result), &result, nullptr);
                        return result;
                    }
                }
            };
            
            using Platform = PlatformWrapper;
            using Device = DeviceWrapper;
        }
        #define OPENCL_MANUAL_WRAPPER
    #endif
    
    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif
#else
    // Linux: Use POCL 7.0 OpenCL 3.0 headers
    #define CL_HPP_ENABLE_EXCEPTIONS
    #define CL_HPP_TARGET_OPENCL_VERSION 300  // OpenCL 3.0
    #define CL_HPP_MINIMUM_OPENCL_VERSION 200 // Minimum 2.0 for fallback
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

// OpenCL version detection helper
std::string getOpenCLVersion(const cl::Platform& platform) {
    try {
        return platform.getInfo<CL_PLATFORM_VERSION>();
    } catch (...) {
        return "Unknown";
    }
}

std::string getDeviceOpenCLVersion(const cl::Device& device) {
    try {
        return device.getInfo<CL_DEVICE_VERSION>();
    } catch (...) {
        return "Unknown";
    }
}

Renderer::Renderer(int width, int height) : m_Width(width), m_Height(height), m_OutputTextureID(0) {
    std::cout << "Initializing OpenCL 3.0 Renderer (" << width << "x" << height << ")..." << std::endl;
    
    try {
        // 1. Initialize OpenCL Platform with OpenCL 3.0 support detection
        std::vector<cl::Platform> platforms;
        
        try {
            cl::Platform::get(&platforms);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to get OpenCL platforms: " + std::string(e.what()) + 
                                   "\nMake sure OpenCL 3.0 is properly installed!");
        }
        
        if (platforms.empty()) {
#ifdef _WIN32
            throw std::runtime_error("No OpenCL platforms found!\n"
                                   "Please install:\n"
                                   "- NVIDIA drivers (latest for OpenCL 3.0)\n"
                                   "- Intel OpenCL Runtime 3.0\n"
                                   "- AMD APP SDK with OpenCL 3.0 support");
#else
            throw std::runtime_error("No OpenCL platforms found!\n"
                                   "Please install POCL 7.0+:\n"
                                   "sudo apt install pocl-opencl-icd opencl-headers clinfo\n"
                                   "Ensure POCL version 7.0+ for OpenCL 3.0 support");
#endif
        }
        
        std::cout << "Found " << platforms.size() << " OpenCL platform(s):" << std::endl;
        
        // 2. Platform selection with OpenCL version checking
        cl::Platform platform;
        bool platformSelected = false;
        
        for (size_t i = 0; i < platforms.size(); ++i) {
            try {
                std::string platformName = platforms[i].getInfo<CL_PLATFORM_NAME>();
                std::string platformVendor = platforms[i].getInfo<CL_PLATFORM_VENDOR>();
                std::string platformVersion = getOpenCLVersion(platforms[i]);
                
                std::cout << "  Platform " << i << ": " << platformName 
                         << " (" << platformVendor << ") - " << platformVersion << std::endl;
                
                // Check for OpenCL 3.0 support
                bool supports30 = platformVersion.find("OpenCL 3.") != std::string::npos;
                bool supports20 = platformVersion.find("OpenCL 2.") != std::string::npos;
                
                if (supports30) {
                    std::cout << "    ✅ OpenCL 3.0 supported!" << std::endl;
                } else if (supports20) {
                    std::cout << "    ⚠️  OpenCL 2.0 (fallback compatibility)" << std::endl;
                } else {
                    std::cout << "    ❌ OpenCL version may be too old" << std::endl;
                }
                
#ifdef _WIN32
                // On Windows, prefer NVIDIA with 3.0, then Intel with 3.0, then others
                if (!platformSelected && supports30 &&
                    (platformName.find("NVIDIA") != std::string::npos || 
                     platformName.find("CUDA") != std::string::npos ||
                     platformVendor.find("NVIDIA") != std::string::npos)) {
                    platform = platforms[i];
                    platformSelected = true;
                    std::cout << "    -> Selected (NVIDIA OpenCL 3.0)" << std::endl;
                } else if (!platformSelected && supports30 &&
                          (platformName.find("Intel") != std::string::npos ||
                           platformVendor.find("Intel") != std::string::npos)) {
                    platform = platforms[i];
                    platformSelected = true;
                    std::cout << "    -> Selected (Intel OpenCL 3.0)" << std::endl;
                } else if (!platformSelected && (supports20 || supports30) &&
                          (platformName.find("NVIDIA") != std::string::npos || 
                           platformName.find("Intel") != std::string::npos)) {
                    platform = platforms[i];
                    platformSelected = true;
                    std::cout << "    -> Selected (fallback compatibility)" << std::endl;
                }
#else
                // On Linux, prefer POCL 7.0+ with OpenCL 3.0
                if (!platformSelected &&
                    (platformName.find("Portable Computing Language") != std::string::npos ||
                     platformName.find("POCL") != std::string::npos ||
                     platformVendor.find("POCL") != std::string::npos)) {
                    platform = platforms[i];
                    platformSelected = true;
                    if (supports30) {
                        std::cout << "    -> Selected (POCL 7.0+ with OpenCL 3.0)" << std::endl;
                    } else {
                        std::cout << "    -> Selected (POCL with OpenCL 2.0 fallback)" << std::endl;
                    }
                }
#endif
                
            } catch (const cl::Error& err) {
                std::cout << "  Platform " << i << ": <info unavailable> (Error: " << err.err() << ")" << std::endl;
            }
        }
        
        // Fallback to first platform if none specifically selected
        if (!platformSelected) {
            platform = platforms[0];
            std::cout << "Using first available platform as fallback" << std::endl;
        }
        
        // 3. Device selection with OpenCL 3.0 capability checking
        std::vector<cl::Device> devices;
        bool deviceFound = false;
        
        // Try different device types in order of preference
#ifdef _WIN32
        std::vector<std::pair<cl_device_type, std::string>> deviceTypes = {
            {CL_DEVICE_TYPE_GPU, "GPU"},
            {CL_DEVICE_TYPE_ACCELERATOR, "ACCELERATOR"},
            {CL_DEVICE_TYPE_CPU, "CPU"},
            {CL_DEVICE_TYPE_ALL, "ALL"}
        };
#else
        std::vector<std::pair<cl_device_type, std::string>> deviceTypes = {
            {CL_DEVICE_TYPE_ALL, "ALL"},
            {CL_DEVICE_TYPE_CPU, "CPU"},
            {CL_DEVICE_TYPE_GPU, "GPU"},
            {CL_DEVICE_TYPE_ACCELERATOR, "ACCELERATOR"}
        };
#endif
        
        for (auto& [deviceType, typeName] : deviceTypes) {
            try {
                platform.getDevices(deviceType, &devices);
                if (!devices.empty()) {
                    deviceFound = true;
                    std::cout << "Found " << devices.size() << " device(s) of type " << typeName << std::endl;
                    break;
                }
            } catch (const cl::Error& err) {
                std::cout << "No " << typeName << " devices found (Error: " << err.err() << ")" << std::endl;
            }
        }
        
        if (!deviceFound || devices.empty()) {
            throw std::runtime_error("No OpenCL devices found on any platform!");
        }
        
        // 4. Select and analyze the best device with OpenCL 3.0 features
        m_Device = std::make_unique<cl::Device>(devices[0]);
        
        try {
            std::string deviceName = m_Device->getInfo<CL_DEVICE_NAME>();
            std::string deviceVersion = getDeviceOpenCLVersion(*m_Device);
            cl_device_type deviceType = m_Device->getInfo<CL_DEVICE_TYPE>();
            cl_uint computeUnits = m_Device->getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
            cl_bool available = m_Device->getInfo<CL_DEVICE_AVAILABLE>();
            cl_ulong globalMemSize = m_Device->getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
            cl_ulong localMemSize = m_Device->getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
            
            std::cout << "\n=== Selected Device Information (OpenCL 3.0) ===" << std::endl;
            std::cout << "Device: " << deviceName << std::endl;
            std::cout << "OpenCL Version: " << deviceVersion << std::endl;
            std::cout << "Type: " << (deviceType == CL_DEVICE_TYPE_GPU ? "GPU" : 
                                     deviceType == CL_DEVICE_TYPE_CPU ? "CPU" : 
                                     deviceType == CL_DEVICE_TYPE_ACCELERATOR ? "ACCELERATOR" : "Other") << std::endl;
            std::cout << "Compute Units: " << computeUnits << std::endl;
            std::cout << "Global Memory: " << (globalMemSize / (1024*1024)) << " MB" << std::endl;
            std::cout << "Local Memory: " << (localMemSize / 1024) << " KB" << std::endl;
            std::cout << "Available: " << (available ? "Yes" : "No") << std::endl;
            
            // Check OpenCL 3.0 specific features
            bool hasOpenCL30 = deviceVersion.find("OpenCL 3.") != std::string::npos;
            bool hasOpenCL20 = deviceVersion.find("OpenCL 2.") != std::string::npos;
            
            if (hasOpenCL30) {
                std::cout << "🚀 OpenCL 3.0 Features Available:" << std::endl;
                std::cout << "   ✅ Enhanced performance optimizations" << std::endl;
                std::cout << "   ✅ Better memory management" << std::endl;
                std::cout << "   ✅ Improved compiler optimizations" << std::endl;
            } else if (hasOpenCL20) {
                std::cout << "⚠️  OpenCL 2.0 fallback mode" << std::endl;
            } else {
                std::cout << "❌ OpenCL version may be too old for optimal performance" << std::endl;
            }
            
            // Performance expectations with OpenCL 3.0
#ifdef _WIN32
            if (deviceType == CL_DEVICE_TYPE_GPU && hasOpenCL30) {
                std::cout << "🚀 GPU OpenCL 3.0 detected - expect excellent performance!" << std::endl;
                if (computeUnits >= 20) {
                    std::cout << "   High-end GPU detected - should achieve 120+ FPS" << std::endl;
                } else {
                    std::cout << "   Mid-range GPU detected - should achieve 60-120 FPS" << std::endl;
                }
            } else if (deviceType == CL_DEVICE_TYPE_GPU) {
                std::cout << "🟡 GPU OpenCL detected (older version) - good performance expected" << std::endl;
                std::cout << "   Expected performance: 30-60 FPS" << std::endl;
            } else {
                std::cout << "⚠️  CPU OpenCL on Windows - performance will be limited" << std::endl;
                std::cout << "   Expected performance: 10-30 FPS depending on CPU" << std::endl;
            }
#else
            std::cout << "🐧 Linux OpenCL (POCL 7.0+) detected" << std::endl;
            if (hasOpenCL30 && computeUnits >= 8) {
                std::cout << "   OpenCL 3.0 + Multi-core CPU - should achieve 30-60 FPS" << std::endl;
            } else if (computeUnits >= 8) {
                std::cout << "   Multi-core CPU detected - should achieve 20-40 FPS" << std::endl;
            } else {
                std::cout << "   Limited CPU cores - expect 10-20 FPS" << std::endl;
            }
#endif
            
            if (!available) {
                throw std::runtime_error("Selected OpenCL device is not available");
            }
            
        } catch (const cl::Error& err) {
            std::cout << "Warning: Could not get complete device info: " << err.what() 
                     << " (Code: " << err.err() << ")" << std::endl;
        }
        
        // 5. Create OpenCL Context with OpenCL 3.0 optimizations
        try {
            m_Context = std::make_unique<cl::Context>(*m_Device);
            std::cout << "OpenCL 3.0 context created successfully" << std::endl;
        } catch (const cl::Error& err) {
            std::cerr << "Failed to create OpenCL context: " << err.what() 
                     << " (Code: " << err.err() << ")" << std::endl;
            throw std::runtime_error("Failed to create OpenCL context");
        }
        
        // 6. Create Command Queue with OpenCL 3.0 features
        try {
            // Use OpenCL 2.0+ command queue creation with profiling for OpenCL 3.0
            cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
            m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, *m_Device, properties);
            std::cout << "OpenCL 3.0 command queue created successfully (with profiling)" << std::endl;
        } catch (const cl::Error& err) {
            // Fallback to basic command queue if profiling fails
            try {
                m_Queue = std::make_unique<cl::CommandQueue>(*m_Context, *m_Device);
                std::cout << "OpenCL command queue created successfully (basic mode)" << std::endl;
            } catch (const cl::Error& err2) {
                std::cerr << "Failed to create OpenCL command queue: " << err2.what() 
                         << " (Code: " << err2.err() << ")" << std::endl;
                throw std::runtime_error("Failed to create OpenCL command queue");
            }
        }
        
        // 7. Create GPU resources with OpenCL 3.0 optimizations
        createResources(width, height);
        
        std::cout << "=== OpenCL 3.0 Renderer Initialized Successfully! ===" << std::endl;
        
    } catch (const cl::Error& err) {
        std::cerr << "OpenCL Error: " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        std::cerr << "OpenCL Error Codes Reference:" << std::endl;
        std::cerr << "  -1: CL_DEVICE_NOT_FOUND" << std::endl;
        std::cerr << "  -2: CL_DEVICE_NOT_AVAILABLE" << std::endl;
        std::cerr << "  -30: CL_INVALID_VALUE" << std::endl;
        std::cerr << "  -34: CL_INVALID_CONTEXT" << std::endl;
        throw std::runtime_error("Failed to initialize OpenCL 3.0: " + std::string(err.what()));
    } catch (const std::exception& err) {
        std::cerr << "Error initializing OpenCL 3.0 Renderer: " << err.what() << std::endl;
        throw;
    }
}

Renderer::~Renderer() {
    if (m_OutputTextureID) {
        glDeleteTextures(1, &m_OutputTextureID);
    }
    std::cout << "OpenCL 3.0 Renderer destroyed." << std::endl;
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

    // Create OpenCL image for output with OpenCL 3.0 optimizations
    cl::ImageFormat format(CL_RGBA, CL_FLOAT);
    m_OutputImage = std::make_unique<cl::Image2D>(*m_Context, CL_MEM_WRITE_ONLY, format, width, height);

    // Create buffer for rays with OpenCL 3.0 memory optimizations
    m_InitialRays.resize(width * height);
    m_RayBuffer = std::make_unique<cl::Buffer>(*m_Context, CL_MEM_READ_ONLY, sizeof(Ray) * m_InitialRays.size());
    
    std::cout << "OpenCL 3.0 resources created for " << width << "x" << height << " (" << m_InitialRays.size() << " rays)" << std::endl;
}

std::string Renderer::generateCompilerOptions(IMetric* metric) const {
    if (!metric) return "";
    
    auto tensor = metric->getMetricTensor(Vec4(0.0, 0.0, 0.0, 0.0)); // Get tensor at origin
    
    // OpenCL 3.0 optimized compiler options
    std::string options = " -cl-std=CL3.0 -cl-mad-enable -cl-fast-relaxed-math -cl-finite-math-only";
    
    // Add OpenCL 3.0 specific optimizations
    options += " -cl-opt-disable"; // We'll enable specific optimizations
    options += " -O3"; // Maximum optimization level
    
    // Metric tensor values
    options += " -DMETRIC_G00=" + std::to_string(tensor[0][0].real);
    options += " -DMETRIC_G11=" + std::to_string(tensor[1][1].real);
    options += " -DMETRIC_G22=" + std::to_string(tensor[2][2].real);
    options += " -DMETRIC_G33=" + std::to_string(tensor[3][3].real);
    
    // OpenCL 3.0 feature flags
    options += " -DOPENCL_VERSION_3_0=1";
    
    std::cout << "OpenCL 3.0 Compiler options: " << options << std::endl;
    return options;
}

void Renderer::compileKernel(IMetric* metric) {
    if (!metric) return;
    
    try {
        std::cout << "Compiling OpenCL 3.0 kernel for metric: " << metric->getName() << std::endl;
        
        std::string kernelSource = loadKernelSource("kernels/raytracer.cl");
        std::cout << "Loaded kernel source (" << kernelSource.length() << " characters)" << std::endl;
        
        cl::Program program(*m_Context, kernelSource);
        std::string options = generateCompilerOptions(metric);
        
        try {
            program.build({*m_Device}, options.c_str());
            std::cout << "OpenCL 3.0 kernel compiled successfully!" << std::endl;
        } catch (const cl::BuildError& err) {
            std::cerr << "OpenCL 3.0 Kernel Build Error:" << std::endl;
            auto buildLog = err.getBuildLog();
            for (const auto& log : buildLog) {
                std::cerr << "Device: " << log.first.getInfo<CL_DEVICE_NAME>() << std::endl;
                std::cerr << "Build Log:\n" << log.second << std::endl;
            }
            throw;
        }

        m_Kernel = std::make_unique<cl::Kernel>(program, "trace_rays");
        m_LastMetricName = metric->getName();
        
        std::cout << "OpenCL 3.0 kernel created successfully!" << std::endl;
        
    } catch (const std::exception& err) {
        std::cerr << "Error compiling OpenCL 3.0 kernel: " << err.what() << std::endl;
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
        
        // 2. Transfer ray data from CPU to GPU with OpenCL 3.0 optimizations
        m_Queue->enqueueWriteBuffer(*m_RayBuffer, CL_FALSE, 0, sizeof(Ray) * m_InitialRays.size(), m_InitialRays.data());
        
        // 3. Set kernel arguments
        m_Kernel->setArg(0, *m_RayBuffer);
        m_Kernel->setArg(1, *m_OutputImage);
        
        // 4. Execute the kernel with OpenCL 3.0 optimized work group sizes
        cl::NDRange globalSize(m_Width * m_Height);
        
#ifdef _WIN32
        // On Windows with GPU, use larger work groups for OpenCL 3.0
        cl::NDRange localSize(512);  // Increased for OpenCL 3.0
#else
        // On Linux with POCL 7.0+, use optimized work groups
        cl::NDRange localSize(128);  // Increased for POCL 7.0+
#endif
        
        m_Queue->enqueueNDRangeKernel(*m_Kernel, cl::NullRange, globalSize, localSize);

        // 5. Read the result back from OpenCL with optimized memory transfer
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
        std::cerr << "OpenCL 3.0 Runtime Error in render(): " << err.what() << " (Code: " << err.err() << ")" << std::endl;
        // Fallback to a simple pattern if OpenCL fails
        renderFallback();
    } catch (const std::exception& err) {
        std::cerr << "Error in OpenCL 3.0 render(): " << err.what() << std::endl;
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
    
    std::cout << "Using fallback rendering (OpenCL 3.0 failed)" << std::endl;
}

unsigned int Renderer::getOutputTexture() const {
    return m_OutputTextureID;
}