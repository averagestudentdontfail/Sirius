#include "PluginManager.h"
#include <filesystem>
#include <iostream>

// Platform-specific includes for dynamic loading
#ifdef _WIN32
    #include <windows.h>
    #define LIBRARY_HANDLE HMODULE
    #define LOAD_LIBRARY(path) LoadLibraryA(path)
    #define GET_FUNCTION(handle, name) GetProcAddress(handle, name)
    #define CLOSE_LIBRARY(handle) FreeLibrary(handle)
    #define LIBRARY_EXTENSION ".dll"
    #define GET_ERROR() GetLastError()
#else
    #include <dlfcn.h>
    #define LIBRARY_HANDLE void*
    #define LOAD_LIBRARY(path) dlopen(path, RTLD_LAZY)
    #define GET_FUNCTION(handle, name) dlsym(handle, name)
    #define CLOSE_LIBRARY(handle) dlclose(handle)
    #define LIBRARY_EXTENSION ".so"
    #define GET_ERROR() dlerror()
#endif

namespace fs = std::filesystem;

PluginManager::PluginManager() {}

PluginManager::~PluginManager() {
    for (auto& handle : m_LoadedPlugins) {
        // Destroy the metric instance using the function from the plugin
        handle.destroy(handle.instance.release());
        // Unload the shared library
        CLOSE_LIBRARY((LIBRARY_HANDLE)handle.library);
    }
}

void PluginManager::loadPlugins(const std::string& pluginDir) {
    if (!fs::exists(pluginDir) || !fs::is_directory(pluginDir)) {
        std::cerr << "Plugin directory not found: " << pluginDir << std::endl;
        return;
    }

    std::cout << "Loading plugins from: " << pluginDir << std::endl;

    for (const auto& entry : fs::directory_iterator(pluginDir)) {
        if (entry.path().extension() == LIBRARY_EXTENSION) {
            LIBRARY_HANDLE library = LOAD_LIBRARY(entry.path().string().c_str());
            if (!library) {
#ifdef _WIN32
                DWORD error = GetLastError();
                std::cerr << "Failed to load plugin " << entry.path() << ": Error code " << error << std::endl;
#else
                std::cerr << "Failed to load plugin " << entry.path() << ": " << dlerror() << std::endl;
#endif
                continue;
            }

            // Load the 'createMetric' and 'destroyMetric' symbols
            CreateMetricFunc createFunc = (CreateMetricFunc)GET_FUNCTION(library, "createMetric");
            DestroyMetricFunc destroyFunc = (DestroyMetricFunc)GET_FUNCTION(library, "destroyMetric");

#ifdef _WIN32
            if (!createFunc || !destroyFunc) {
                std::cerr << "Failed to load symbols from " << entry.path() << ": Functions not found" << std::endl;
                CLOSE_LIBRARY(library);
                continue;
            }
#else
            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "Failed to load symbols from " << entry.path() << ": " << dlsym_error << std::endl;
                CLOSE_LIBRARY(library);
                continue;
            }
#endif

            // Create an instance of the metric
            IMetric* metricInstance = createFunc();
            m_LoadedPlugins.emplace_back((void*)library, destroyFunc, std::unique_ptr<IMetric>(metricInstance));
            std::cout << "Successfully loaded metric: " << metricInstance->getName() << std::endl;
        }
    }
}

std::vector<std::string> PluginManager::getMetricNames() const {
    std::vector<std::string> names;
    for (const auto& handle : m_LoadedPlugins) {
        names.push_back(handle.instance->getName());
    }
    return names;
}

IMetric* PluginManager::getMetric(const std::string& name) {
    for (auto& handle : m_LoadedPlugins) {
        if (handle.instance->getName() == name) {
            return handle.instance.get();
        }
    }
    return nullptr;
}