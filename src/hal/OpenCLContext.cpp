#include "OpenCLContext.hpp"
#include <iostream>
#include <stdexcept>

namespace Sirius::HAL {

OpenCLContext::OpenCLContext() {
    try {
        SelectPlatformAndDevice();
        m_Context = cl::Context(m_Device);
        m_Queue = cl::CommandQueue(m_Context, m_Device);
    } catch (const std::exception& err) {
        std::cerr << "OpenCL Error: " << err.what() << std::endl;
        throw std::runtime_error("Failed to initialize OpenCL Context.");
    }
}

void OpenCLContext::SelectPlatformAndDevice() {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    if (platforms.empty()) {
        throw std::runtime_error("No OpenCL platforms found.");
    }

    // Heuristic: Prefer platforms with GPU devices
    for (const auto& p : platforms) {
        std::vector<cl::Device> devices;
        try {
            p.getDevices(CL_DEVICE_TYPE_GPU, &devices);
            if (!devices.empty()) {
                m_Platform = p;
                m_Device = devices[0]; // Select the first GPU
                return;
            }
        } catch (const std::exception&) {
            // No GPU devices on this platform, continue
            continue;
        }
    }

    // Fallback to any device if no GPU is found
    m_Platform = platforms[0];
    std::vector<cl::Device> allDevices;
    try {
        m_Platform.getDevices(CL_DEVICE_TYPE_ALL, &allDevices);
        if(allDevices.empty()){
            throw std::runtime_error("No OpenCL devices found on the selected platform.");
        }
        m_Device = allDevices[0];
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to get OpenCL devices: " + std::string(e.what()));
    }
}

void OpenCLContext::PrintInfo() const {
    std::cout << "--- OpenCL Information ---" << std::endl;
    std::cout << "Platform: " << m_Platform.getInfo<CL_PLATFORM_NAME>() << std::endl;
    std::cout << "Device:   " << m_Device.getInfo<CL_DEVICE_NAME>() << std::endl;
    std::cout << "Version:  " << m_Device.getInfo<CL_DEVICE_VERSION>() << std::endl;
    std::cout << "--------------------------" << std::endl;
}

} // namespace Sirius::HAL