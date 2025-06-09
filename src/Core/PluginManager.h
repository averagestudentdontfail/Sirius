#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Physics/IMetric.h"

class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    // Scans a directory for .so/.dll files and loads them
    void loadPlugins(const std::string& pluginDir);

    // Returns a list of the names of all loaded metrics
    std::vector<std::string> getMetricNames() const;

    // Returns a pointer to a metric instance by name
    IMetric* getMetric(const std::string& name);

private:
    // A function pointer type for our exported 'create' function
    using CreateMetricFunc = IMetric* (*)();
    // A function pointer type for our exported 'destroy' function
    using DestroyMetricFunc = void (*)(IMetric*);

    struct PluginHandle {
        void* library;
        DestroyMetricFunc destroy;
        std::unique_ptr<IMetric> instance;
    };

    std::vector<PluginHandle> m_LoadedPlugins;
};