#pragma once

#include <mitsuba/core/properties.h>
#include <mitsuba/render/sensor.h>

#include "../common/gonio_grid.h"

#include <algorithm>

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class GonioSensor final : public Sensor<Float, Spectrum> {
public:
    MI_IMPORT_BASE(Sensor, m_film, m_needs_sample_2, m_needs_sample_3)
    MI_IMPORT_TYPES()

    GonioSensor(const Properties &props) : Base(props) {
        m_precision = props.get<uint32_t>("precision", 1u);
        m_merge_l1_l2 = props.get<bool>("merge_l1_l2", false);
        m_record_refraction = props.get<bool>("record_refraction", false);
        m_analytic_measurement = props.get<bool>("analytic_measurement", false);

        m_scalar_grid = gonio::build_scalar_grid(m_precision);
        m_grid.ring_count = (uint32_t) m_scalar_grid.rings.size();
        if constexpr (dr::is_jit_v<Float>)
            m_grid = gonio::upload_grid_buffers<Float>(m_scalar_grid);

        uint32_t layer_count = 1u;
        if (m_analytic_measurement) {
            if (!m_merge_l1_l2) {
                Log(Warn, "GonioSensor: analytic measurements always merge L1/L2.");
                m_merge_l1_l2 = true;
            }
            layer_count = m_record_refraction ? 2u : 1u;
        } else if (m_merge_l1_l2) {
            layer_count = m_record_refraction ? 2u : 1u;
        } else {
            layer_count = m_record_refraction ? 4u : 2u;
        }

        uint32_t header_cells = m_analytic_measurement ? 0u : 1u;
        m_film->set_size(ScalarPoint2u(m_scalar_grid.patch_count + header_cells,
                                       layer_count));
        m_needs_sample_2 = false;
        m_needs_sample_3 = false;
    }

    UInt32 index(Float phi, Float theta, Mask active = true) const {
        phi = dr::select(phi < 0.f, phi + dr::TwoPi<Float>, phi);
        phi = dr::select(phi >= dr::TwoPi<Float>, phi - dr::TwoPi<Float>, phi);
        theta = dr::clip(theta, 0.f, dr::Pi<Float> * 0.5f);

        if constexpr (!dr::is_jit_v<Float>) {
            DRJIT_MARK_USED(active);

            uint32_t ring = 0u;
            ScalarFloat theta_s = dr::slice(theta);
            while (ring + 1u < m_grid.ring_count &&
                   theta_s >= m_scalar_grid.rings[ring].theta_max)
                ++ring;

            const gonio::ScalarRing &ring_data = m_scalar_grid.rings[ring];
            uint32_t patch = (uint32_t) std::floor(dr::slice(phi) / ring_data.phi_step);
            patch = std::min(patch, ring_data.patch_count - 1u);
            return ring_data.base_index + patch;
        }

        UInt32 ring = dr::binary_search<UInt32>(
            0u, m_grid.ring_count,
            [&](UInt32 idx) DRJIT_INLINE_LAMBDA {
                Float theta_max = dr::gather<Float>(m_grid.theta_max, idx, active);
                return theta_max <= theta;
            }
        );
        ring = dr::minimum(ring, (uint32_t) (m_grid.ring_count - 1u));

        UInt32 base = dr::gather<UInt32>(m_grid.base_index, ring, active);
        UInt32 count = dr::gather<UInt32>(m_grid.patch_count, ring, active);
        Float phi_step = dr::gather<Float>(m_grid.phi_step, ring, active);
        UInt32 patch = dr::floor2int<UInt32>(phi / phi_step);
        patch = dr::minimum(patch, count - 1u);

        return base + patch;
    }

    uint32_t precision() const { return m_precision; }
    uint32_t patch_count() const { return m_scalar_grid.patch_count; }
    uint32_t ring_count() const { return m_grid.ring_count; }
    float cell_solid_angle() const { return m_scalar_grid.cell_solid_angle; }
    bool merge_l1_l2() const { return m_merge_l1_l2; }
    bool record_refraction() const { return m_record_refraction; }
    bool analytic_measurement() const { return m_analytic_measurement; }
    uint32_t header_cells() const { return m_analytic_measurement ? 0u : 1u; }

    uint32_t layer_count() const {
        if (m_analytic_measurement || m_merge_l1_l2)
            return m_record_refraction ? 2u : 1u;
        return m_record_refraction ? 4u : 2u;
    }

    UInt32 film_x(UInt32 cell_index) const {
        return cell_index + header_cells();
    }

    Float fold_theta(Float theta) const {
        Float half_pi = dr::Pi<Float> * 0.5f;
        if (m_record_refraction)
            theta = dr::select(theta >= half_pi, theta - half_pi, theta);
        return dr::clip(theta, 0.f, half_pi);
    }

    Mask accepts_theta(Float theta, Mask active = true) const {
        Float half_pi = dr::Pi<Float> * 0.5f;
        if (m_record_refraction)
            return active;
        return active && theta < half_pi;
    }

    UInt32 layer_index(Float theta, Int32 depth, Mask active = true) const {
        DRJIT_MARK_USED(active);

        Float half_pi = dr::Pi<Float> * 0.5f;
        UInt32 hemisphere = dr::select(theta >= half_pi, UInt32(1u), UInt32(0u));

        if (m_analytic_measurement || m_merge_l1_l2)
            return m_record_refraction ? hemisphere : UInt32(0u);

        UInt32 bounce_class = dr::select(depth > 1, UInt32(1u), UInt32(0u));
        return m_record_refraction ? bounce_class * 2u + hemisphere
                                   : bounce_class;
    }

    std::pair<Ray3f, Spectrum> sample_ray(Float /* time */,
                                          Float /* wavelength_sample */,
                                          const Point2f & /* position_sample */,
                                          const Point2f & /* aperture_sample */,
                                          Mask /* active */) const override {
        Throw("GonioSensor is a measurement accumulator and does not sample primary rays.");
        return {};
    }

    std::pair<RayDifferential3f, Spectrum>
    sample_ray_differential(Float /* time */,
                            Float /* wavelength_sample */,
                            const Point2f & /* position_sample */,
                            const Point2f & /* aperture_sample */,
                            Mask /* active */) const override {
        Throw("GonioSensor is a measurement accumulator and does not sample primary rays.");
        return {};
    }

    ScalarBoundingBox3f bbox() const override {
        return ScalarBoundingBox3f();
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "GonioSensor[" << std::endl
            << "  precision = " << m_precision << "," << std::endl
            << "  patches = " << m_scalar_grid.patch_count << "," << std::endl
            << "  rings = " << m_grid.ring_count << "," << std::endl
            << "  merge_l1_l2 = " << m_merge_l1_l2 << "," << std::endl
            << "  record_refraction = " << m_record_refraction << "," << std::endl
            << "  analytic_measurement = " << m_analytic_measurement << "," << std::endl
            << "  film = " << m_film << std::endl
            << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(GonioSensor)

private:
    uint32_t m_precision = 1u;
    bool m_merge_l1_l2 = false;
    bool m_record_refraction = false;
    bool m_analytic_measurement = false;
    gonio::ScalarGrid m_scalar_grid;
    gonio::GridBuffers<Float> m_grid;
};

NAMESPACE_END(mitsuba)
