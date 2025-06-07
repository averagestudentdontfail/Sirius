#pragma once

#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include <CL/opencl.hpp>
#include <string>
#include <vector>

namespace Sirius::HAL {

class OpenCLContext {
public:
    OpenCLContext();
    ~OpenCLContext() = default;

    const cl::Context& GetContext() const { return m_Context; }
    const cl::Device& GetDevice() const { return m_Device; }
    const cl::CommandQueue& GetQueue() const { return m_Queue; }

    void PrintInfo() const;

private:
    void SelectPlatformAndDevice();

    cl::Platform m_Platform;
    cl::Device m_Device;
    cl::Context m_Context;
    cl::CommandQueue m_Queue;
};

} // namespace Sirius::HAL
