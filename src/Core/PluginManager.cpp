#include "PluginManager.h"
#include <filesystem>
#include <iostream>

// For loading shared libraries on Linux/macOS
#include <dlfcn.h>

namespace fs = std::filesystem;

PluginManager::PluginManager() {}

PluginManager::~PluginManager() {
    for (auto& handle : m_LoadedPlugins) {
        // Destroy the metric instance using the function from the plugin
        handle.destroy(handle.instance.release());
        // Unload the shared library
        dlclose(handle.library);
    }
}

void PluginManager::loadPlugins(const std::string& pluginDir) {
    if (!fs::exists(pluginDir) || !fs::is_directory(pluginDir)) {
        std::cerr << "Plugin directory not found: " << pluginDir << std::endl;
        return;
    }

    std::cout << "Loading plugins from: " << pluginDir << std::endl;

    for (const auto& entry : fs::directory_iterator(pluginDir)) {
        if (entry.path().extension() == ".so") {
            void* library = dlopen(entry.path().c_str(), RTLD_LAZY);
            if (!library) {
                std::cerr << "Failed to load plugin " << entry.path() << ": " << dlerror() << std::endl;
                continue;
            }

            // Load the 'createMetric' and 'destroyMetric' symbols
            CreateMetricFunc createFunc = (CreateMetricFunc)dlsym(library, "createMetric");
            DestroyMetricFunc destroyFunc = (DestroyMetricFunc)dlsym(library, "destroyMetric");

            const char* dlsym_error = dlerror();
            if (dlsym_error) {
                std::cerr << "Failed to load symbols from " << entry.path() << ": " << dlsym_error << std::endl;
                dlclose(library);
                continue;
            }

            // Create an instance of the metric
            IMetric* metricInstance = createFunc();
            m_LoadedPlugins.emplace_back(library, destroyFunc, std::unique_ptr<IMetric>(metricInstance));
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