#pragma once

#include "Physics/IMetric.h"

class MinkowskiMetric : public IMetric {
public:
    const char* getName() const override { return "Minkowski"; }
    const char* getDescription() const override { return "Flat, empty spacetime."; }

    const Config& getParameters() const override { return m_Config; }
    void setParameter(const std::string& key, double value) override { /* No parameters */ }

    Tensor2D<4, 4> getMetricTensor(const Vec4& position) const override;
    std::array<Tensor2D<4, 4>, 4> getMetricDerivatives(const Vec4& position) const override;
private:
    Config m_Config;
};