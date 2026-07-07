#pragma once

#include <mitsuba/core/math.h>
#include <mitsuba/core/vector.h>

#include <cmath>
#include <cstdint>
#include <vector>

NAMESPACE_BEGIN(mitsuba)
NAMESPACE_BEGIN(gonio)

struct ScalarRing {
    float theta_min = 0.f;
    float theta_max = 0.f;
    float phi_step = 0.f;
    uint32_t base_index = 0;
    uint32_t patch_count = 0;
};

struct ScalarGrid {
    uint32_t precision = 1;
    uint32_t patch_count = 0;
    float theta_cap = 0.f;
    float cell_solid_angle = 0.f;
    std::vector<ScalarRing> rings;
};

inline uint32_t expected_patch_count(uint32_t precision) {
    switch (precision) {
        case 1: return 1663u;
        case 2: return 26279u;
        case 3: return 53224u;
        case 4: return 523910u;
        default: return 0u;
    }
}

inline float theta_cap(uint32_t precision) {
    switch (precision) {
        case 1: return dr::deg_to_rad(1.987071f);
        case 2: return dr::deg_to_rad(0.4998442f);
        case 3: return dr::deg_to_rad(0.3512243f);
        case 4: return dr::deg_to_rad(0.1119462f);
        default: return 0.f;
    }
}

inline ScalarGrid build_scalar_grid(uint32_t precision) {
    ScalarGrid grid;
    grid.precision = precision;
    grid.patch_count = expected_patch_count(precision);
    grid.theta_cap = theta_cap(precision);

    if (grid.patch_count == 0 || grid.theta_cap == 0.f)
        Throw("Gonio grid precision must be in the range [1, 4].");

    grid.cell_solid_angle =
        2.f * dr::Pi<float> * (1.f - std::cos(grid.theta_cap));

    grid.rings.reserve(512);
    grid.rings.push_back(ScalarRing{
        0.f,
        grid.theta_cap,
        dr::TwoPi<float>,
        0u,
        1u
    });

    double theta_p = grid.theta_cap;
    double radius_p = 2.0 * std::sin(0.5 * theta_p);
    uint32_t k_p = 1u;

    while (theta_p < 0.5 * dr::Pi<double>) {
        double theta = theta_p +
            2.0 * std::sin(0.5 * theta_p) *
            std::sqrt(dr::Pi<double> / (double) k_p);
        double radius = 2.0 * std::sin(0.5 * theta);
        uint32_t k = (uint32_t) std::llround(
            (radius / radius_p) * (radius / radius_p) * (double) k_p);

        theta = std::acos(std::cos(theta_p) -
                          (1.0 - std::cos((double) grid.theta_cap)) *
                          (double) (k - k_p));

        uint32_t patch_count = k - k_p;
        grid.rings.push_back(ScalarRing{
            (float) theta_p,
            (float) theta,
            dr::TwoPi<float> / (float) patch_count,
            k_p,
            patch_count
        });

        theta_p = theta;
        radius_p = 2.0 * std::sin(0.5 * theta_p);
        k_p = k;
    }

    if (k_p != grid.patch_count)
        Throw("Internal gonio grid construction mismatch: expected %u cells, got %u.",
              grid.patch_count, k_p);

    return grid;
}

template <typename Float>
struct GridBuffers {
    using UInt32 = dr::uint32_array_t<Float>;

    DynamicBuffer<Float> theta_max;
    DynamicBuffer<Float> phi_step;
    DynamicBuffer<UInt32> base_index;
    DynamicBuffer<UInt32> patch_count;
    uint32_t ring_count = 0;
};

template <typename Float>
GridBuffers<Float> upload_grid_buffers(const ScalarGrid &grid) {
    using UInt32 = dr::uint32_array_t<Float>;
    std::vector<dr::scalar_t<Float>> theta_max(grid.rings.size()),
                                     phi_step(grid.rings.size());
    std::vector<dr::scalar_t<UInt32>> base_index(grid.rings.size()),
                                      patch_count(grid.rings.size());

    for (size_t i = 0; i < grid.rings.size(); ++i) {
        theta_max[i] = (dr::scalar_t<Float>) grid.rings[i].theta_max;
        phi_step[i] = (dr::scalar_t<Float>) grid.rings[i].phi_step;
        base_index[i] = (dr::scalar_t<UInt32>) grid.rings[i].base_index;
        patch_count[i] = (dr::scalar_t<UInt32>) grid.rings[i].patch_count;
    }

    GridBuffers<Float> result;
    result.theta_max =
        dr::load<DynamicBuffer<Float>>(theta_max.data(), theta_max.size());
    result.phi_step =
        dr::load<DynamicBuffer<Float>>(phi_step.data(), phi_step.size());
    result.base_index =
        dr::load<DynamicBuffer<UInt32>>(base_index.data(), base_index.size());
    result.patch_count =
        dr::load<DynamicBuffer<UInt32>>(patch_count.data(), patch_count.size());
    result.ring_count = (uint32_t) grid.rings.size();
    return result;
}

NAMESPACE_END(gonio)
NAMESPACE_END(mitsuba)
