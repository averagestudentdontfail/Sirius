#include "MinkowskiMetric.h"

// The functions that the PluginManager will use to create/destroy instances
extern "C" IMetric* createMetric() {
    return new MinkowskiMetric();
}

extern "C" void destroyMetric(IMetric* metric) {
    delete metric;
}

Tensor2D<4, 4> MinkowskiMetric::getMetricTensor(const Vec4& position) const {
    Tensor2D<4, 4> g{}; // Initialize to all zeros
    g[0][0] = -1.0; // g_tt
    g[1][1] =  1.0; // g_xx
    g[2][2] =  1.0; // g_yy
    g[3][3] =  1.0; // g_zz
    return g;
}

std::array<Tensor2D<4, 4>, 4> MinkowskiMetric::getMetricDerivatives(const Vec4& position) const {
    // Derivatives of a constant metric are all zero
    return std::array<Tensor2D<4, 4>, 4>{};
}