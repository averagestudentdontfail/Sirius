#pragma once

#include "Math/Vec.h"
#include "Math/Dual.h"
#include "Core/Config.h"
#include <array>

// Define our 4D spacetime vector and tensor types
template<int Rows, int Cols>
using Tensor2D = std::array<std::array<Dual<double>, Cols>, Rows>;

class IMetric {
public:
    virtual ~IMetric() = default;

    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;

    virtual const Config& getParameters() const = 0;
    virtual void setParameter(const std::string& key, double value) = 0;

    // Calculates the metric tensor g_ab(x)
    virtual Tensor2D<4, 4> getMetricTensor(const Vec4& position) const = 0;

    // Calculates the partial derivatives dg_ab/dx^c
    virtual std::array<Tensor2D<4, 4>, 4> getMetricDerivatives(const Vec4& position) const = 0;
};