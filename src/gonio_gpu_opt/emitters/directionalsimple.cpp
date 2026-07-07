#include <mitsuba/core/properties.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/scene.h>
#include <mitsuba/render/texture.h>

NAMESPACE_BEGIN(mitsuba)

MI_VARIANT class DirectionalSimpleEmitter final : public Emitter<Float, Spectrum> {
public:
    MI_IMPORT_BASE(Emitter, m_flags, m_to_world, m_needs_sample_3)
    MI_IMPORT_TYPES(Scene, Texture)

    DirectionalSimpleEmitter(const Properties &props) : Base(props) {
        m_aabb = ScalarBoundingBox3f();

        if (props.has_property("direction")) {
            if (props.has_property("to_world"))
                Throw("Only one of the parameters 'direction' and 'to_world' can be specified.");

            ScalarVector3f direction =
                dr::normalize(props.get<ScalarVector3f>("direction"));
            auto [up, unused] = coordinate_system(direction);

            m_to_world = ScalarAffineTransform4f::look_at(
                0.f, ScalarPoint3f(direction), up);
            dr::make_opaque(m_to_world);
        }

        m_irradiance = props.get_emissive_texture<Texture>("irradiance", 1.f);

        if (m_irradiance->is_spatially_varying())
            Throw("Expected a non-spatially varying irradiance spectrum.");

        m_needs_sample_3 = false;
        m_flags = EmitterFlags::Infinite | EmitterFlags::DeltaDirection;
    }

    void traverse(TraversalCallback *cb) override {
        Base::traverse(cb);
        cb->put("irradiance", m_irradiance, ParamFlags::Differentiable);
        cb->put("to_world", m_to_world, ParamFlags::NonDifferentiable);
    }

    void set_scene(const Scene *scene) override {
        if (!scene->bbox().valid())
            Throw("DirectionalSimpleEmitter requires a valid scene bounding box.");

        m_aabb = scene->bbox();
        ScalarFloat padding =
            m_aabb.max.z() - m_aabb.min.z() < 0.1f ? 1.f : 0.1f;
        ScalarVector3f delta(padding);
        m_aabb.min -= delta;
        m_aabb.max += delta;
    }

    Spectrum eval(const SurfaceInteraction3f &, Mask) const override {
        return 0.f;
    }

    std::pair<Ray3f, Spectrum> sample_ray(Float time, Float wavelength_sample,
                                          const Point2f &spatial_sample,
                                          const Point2f &,
                                          Mask active) const override {
        MI_MASKED_FUNCTION(ProfilerPhase::EndpointSampleRay, active);

        const auto trafo = m_to_world.value();
        Vector3f d_global = trafo * Vector3f(0.f, 0.f, 1.f);

        if (!m_aabb.valid())
            Throw("DirectionalSimpleEmitter requires scene setup before sampling rays.");

        Point3f origin = dr::zeros<Point3f>();
        Point3f center = m_aabb.center();
        Vector3f extents = m_aabb.extents();
        Point3f seed(
            m_aabb.min.x() + extents.x() * spatial_sample.x(),
            m_aabb.min.y() + extents.y() * spatial_sample.y(),
            center.z()
        );
        Vector3f wi = -d_global;
        Ray3f pre_ray(seed, wi);
        auto [hit, mint, maxt] = m_aabb.ray_intersect(pre_ray);
        DRJIT_MARK_USED(hit);
        DRJIT_MARK_USED(mint);
        origin = seed + wi * maxt;

        SurfaceInteraction3f si = dr::zeros<SurfaceInteraction3f>();
        si.t = 0.f;
        si.time = time;
        si.p = origin;
        si.uv = spatial_sample;

        auto [wavelengths, wav_weight] =
            sample_wavelengths(si, wavelength_sample, active);

        return {
            Ray3f(origin, d_global, time, wavelengths),
            depolarizer<Spectrum>(wav_weight)
        };
    }

    std::pair<DirectionSample3f, Spectrum>
    sample_direction(const Interaction3f &, const Point2f &, Mask) const override {
        Throw("DirectionalSimpleEmitter does not support sample_direction().");
        return {};
    }

    Spectrum eval_direction(const Interaction3f &it,
                            const DirectionSample3f &,
                            Mask active) const override {
        SurfaceInteraction3f si = dr::zeros<SurfaceInteraction3f>();
        si.wavelengths = it.wavelengths;
        return depolarizer<Spectrum>(m_irradiance->eval(si, active));
    }

    Float pdf_direction(const Interaction3f &,
                        const DirectionSample3f &,
                        Mask) const override {
        return 0.f;
    }

    std::pair<Wavelength, Spectrum>
    sample_wavelengths(const SurfaceInteraction3f &si, Float sample,
                       Mask active) const override {
        return m_irradiance->sample_spectrum(
            si, math::sample_shifted<Wavelength>(sample), active);
    }

    std::pair<PositionSample3f, Float>
    sample_position(Float, const Point2f &, Mask) const override {
        if constexpr (dr::is_jit_v<Float>) {
            return { dr::zeros<PositionSample3f>(),
                     dr::full<Float>(dr::NaN<ScalarFloat>) };
        } else {
            NotImplementedError("sample_position");
        }
    }

    ScalarBoundingBox3f bbox() const override {
        return ScalarBoundingBox3f();
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "DirectionalSimpleEmitter[" << std::endl
            << "  aabb = " << m_aabb << "," << std::endl
            << "  irradiance = " << m_irradiance << std::endl
            << "]";
        return oss.str();
    }

    MI_DECLARE_CLASS(DirectionalSimpleEmitter)

private:
    ref<Texture> m_irradiance;
    ScalarBoundingBox3f m_aabb;
};

MI_EXPORT_PLUGIN(DirectionalSimpleEmitter)
NAMESPACE_END(mitsuba)
