#include <mitsuba/core/properties.h>
#include <mitsuba/core/progress.h>
#include <mitsuba/core/ray.h>
#include <mitsuba/core/timer.h>
#include <mitsuba/core/util.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/emitter.h>
#include <mitsuba/render/integrator.h>
#include <mitsuba/render/records.h>
#include <mitsuba/render/sampler.h>

#include "../sensors/gonio_sensor.h"

NAMESPACE_BEGIN(mitsuba)

// Light-tracing integrator for gonio measurements. Each sample starts at the
// directional emitter, follows BSDF scattering through the scene, and bins
// the final escaped direction in GonioSensor's spherical film.
MI_VARIANT class GonioTracerIntegrator final : public Integrator<Float, Spectrum> {
public:
    MI_IMPORT_BASE(Integrator, should_stop, aov_names, m_stop, m_timeout,
                   m_render_timer, m_hide_emitters)
    MI_IMPORT_TYPES(Scene, Sensor, Film, Sampler, ImageBlock, BSDF, BSDFPtr)

    GonioTracerIntegrator(const Properties &props) : Base(props) {
        m_samples_per_pass = props.get<uint32_t>("samples_per_pass", (uint32_t) -1);
        m_rr_depth = props.get<int>("rr_depth", 5);
        if (m_rr_depth <= 0)
            Throw("\"rr_depth\" must be set to a value greater than zero!");

        m_max_depth = props.get<int>("max_depth", -1);
        if (m_max_depth < 0 && m_max_depth != -1)
            Throw("\"max_depth\" must be set to -1 (infinite) or a value >= 0");
    }

    TensorXf render(Scene *scene,
                    Sensor *sensor,
                    UInt32 seed = 0,
                    uint32_t spp = 0,
                    bool develop = true,
                    bool evaluate = true) override {
        ScopedPhase sp(ProfilerPhase::Render);
        m_stop = false;

        const auto *gonio_sensor =
            dynamic_cast<const GonioSensor<Float, Spectrum> *>(sensor);

        if (unlikely(gonio_sensor == nullptr))
            Throw("gtracer requires a sensor of type 'gonio'.");

        Film *film = sensor->film();
        Sampler *sampler = sensor->sampler();

        if (spp)
            sampler->set_sample_count(spp);
        uint32_t total_samples = sampler->sample_count();

        uint32_t samples_per_pass =
            (m_samples_per_pass == (uint32_t) -1)
                ? total_samples
                : std::min(m_samples_per_pass, total_samples);

        if (samples_per_pass == 0)
            Throw("gtracer requires a positive sample count.");

        if ((total_samples % samples_per_pass) != 0) {
            Throw("Total samples (%d) must be a multiple of samples_per_pass (%d).",
                  total_samples, samples_per_pass);
        }

        uint32_t n_passes = total_samples / samples_per_pass;
        film->prepare(aov_names());

        TensorXf result;
        m_render_timer.reset();

        // Keep a scalar loop for debugging/reference runs. JIT variants use
        // a wavefront and Dr.Jit's loop recording to process many rays at
        // once, while preserving the same film layout.
        if constexpr (!dr::is_jit_v<Float>) {
            Log(Info, "Starting gonio render job (%u sample%s)",
                total_samples, total_samples == 1 ? "" : "s");

            ref<Sampler> local_sampler = sampler->clone();
            local_sampler->seed(seed);

            ref<ImageBlock> block =
                film->create_block(ScalarVector2u(0), false, false);
            block->set_offset(film->crop_offset());
            block->clear();

            ref<ProgressReporter> progress = new ProgressReporter("Rendering");
            for (uint32_t i = 0; i < total_samples && !should_stop(); ++i) {
                sample(scene, gonio_sensor, local_sampler.get(), block.get());
                local_sampler->advance();

                if ((i + 1u) % 1000u == 0u || i + 1u == total_samples)
                    progress->update((i + 1u) / (ScalarFloat) total_samples);
            }

            film->put_block(block);
            if (develop)
                result = film->develop();
        } else {
            if (n_passes > 1 && !evaluate) {
                Log(Warn, "render(): forcing 'evaluate=true' since multi-pass "
                          "rendering was requested.");
                evaluate = true;
            }

            constexpr size_t wavefront_size_limit = 0xffffffffu;
            if (samples_per_pass > wavefront_size_limit)
                Throw("gtracer samples_per_pass exceeds the 32-bit wavefront limit.");

            Log(Info, "Starting gonio render job (%u sample%s%s)",
                total_samples, total_samples == 1 ? "" : "s",
                n_passes > 1 ? tfm::format(", %u passes", n_passes) : "");

            sampler->set_samples_per_wavefront(samples_per_pass);
            sampler->seed(seed, samples_per_pass);

            ref<ImageBlock> block =
                film->create_block(ScalarVector2u(0), false, false);
            block->set_offset(film->crop_offset());
            block->set_coalesce(false);

            Timer timer;
            for (uint32_t i = 0; i < n_passes; ++i) {
                sample(scene, gonio_sensor, sampler, block.get());

                if (n_passes > 1) {
                    sampler->advance();
                    sampler->schedule_state();
                    dr::eval(block->tensor());
                }
            }

            film->put_block(block);

            if (develop) {
                result = film->develop();
                dr::schedule(result);
            } else {
                film->schedule_storage();
            }

            if (evaluate) {
                dr::eval();

                if (n_passes == 1 && jit_flag(JitFlag::VCallRecord) &&
                    jit_flag(JitFlag::LoopRecord)) {
                    Log(Info, "Code generation finished. (took %s)",
                        util::time_string((float) timer.value(), true));
                    m_render_timer.reset();
                }

                dr::sync_thread();
            }
        }

        if (!m_stop && (evaluate || !dr::is_jit_v<Float>))
            Log(Info, "Rendering finished. (took %s)",
                util::time_string((float) m_render_timer.value(), true));

        return result;
    }

    std::vector<std::string> aov_names() const override { return { }; }

    void sample(const Scene *scene,
                const GonioSensor<Float, Spectrum> *gonio_sensor,
                Sampler *sampler,
                ImageBlock *block) const {
        auto [ray, throughput] = prepare_ray(scene, gonio_sensor, sampler);
        Mask active = dr::max(unpolarized_spectrum(throughput)) != 0.f;

        trace_light_ray(scene, gonio_sensor, sampler, ray, throughput, block, active);
    }

    std::pair<Ray3f, Spectrum> prepare_ray(const Scene *scene,
                                           const Sensor *sensor,
                                           Sampler *sampler) const {
        Float time = sensor->shutter_open();
        if (sensor->shutter_open_time() > 0.f)
            time += sampler->next_1d() * sensor->shutter_open_time();

        Float wavelength_sample = sampler->next_1d();
        Point2f direction_sample = sampler->next_2d(),
                position_sample  = sampler->next_2d();

        auto [ray, ray_weight, emitter] = scene->sample_emitter_ray(
            time, wavelength_sample, direction_sample, position_sample);

        DRJIT_MARK_USED(emitter);
        return { ray, ray_weight };
    }

    void trace_light_ray(const Scene *scene,
                         const GonioSensor<Float, Spectrum> *sensor,
                         Sampler *sampler,
                         Ray3f ray,
                         Spectrum throughput,
                         ImageBlock *block,
                         Mask active = true) const {
        // The loop state must contain every value that changes per lane so
        // Dr.Jit can record the transport loop for scalar and GPU variants.
        Float eta(1.f);
        Int32 depth = 0;

        PreliminaryIntersection3f pi =
            scene->ray_intersect_preliminary(ray, active);
        active &= pi.is_valid();

        if (dr::any_or<true>(active))
            splat_surface_hit(block, active);

        struct LoopState {
            Bool active;
            Int32 depth;
            Ray3f ray;
            Spectrum throughput;
            PreliminaryIntersection3f pi;
            Float eta;
            Sampler *sampler;

            DRJIT_STRUCT(LoopState, active, depth, ray, throughput, pi, eta, sampler)
        } ls = { active, depth, ray, throughput, pi, eta, sampler };

        dr::tie(ls) = dr::while_loop(
            dr::make_tuple(ls),
            [](const LoopState &ls) { return ls.active; },
            [this, scene, sensor, block](LoopState &ls) {
                SurfaceInteraction3f si =
                    ls.pi.compute_surface_interaction(ls.ray, +RayFlags::All);
                BSDFPtr bsdf = si.bsdf(ls.ray);

                ls.depth += 1;

                BSDFContext ctx(TransportMode::Importance);
                auto [bs, bsdf_val] = bsdf->sample(
                    ctx, si, ls.sampler->next_1d(ls.active),
                    ls.sampler->next_2d(ls.active), ls.active);

                Float wi_dot_geo_n = dr::dot(si.n, -ls.ray.d),
                      wo_dot_geo_n = dr::dot(si.n, si.to_world(bs.wo));

                ls.active &= (wi_dot_geo_n * Frame3f::cos_theta(si.wi) > 0.f) &&
                             (wo_dot_geo_n * Frame3f::cos_theta(bs.wo) > 0.f);

                // Convert the sampled BSDF value from the local frame to the
                // geometric-normal measure used by the light-tracing ray.
                Float correction = dr::abs(
                    (Frame3f::cos_theta(si.wi) * wo_dot_geo_n) /
                    (Frame3f::cos_theta(bs.wo) * wi_dot_geo_n));

                ls.throughput *= bsdf_val * correction;
                ls.eta *= bs.eta;

                ls.active &= dr::any(unpolarized_spectrum(ls.throughput) != 0.f);
                if (dr::none_or<false>(ls.active))
                    return;

                // Russian roulette starts after rr_depth and is compensated
                // in throughput to keep the estimator unbiased.
                Mask use_rr = ls.depth > m_rr_depth;
                if (dr::any_or<true>(use_rr)) {
                    Float q = dr::minimum(
                        dr::max(unpolarized_spectrum(ls.throughput)) * dr::square(ls.eta),
                        0.95f
                    );
                    dr::masked(ls.active, use_rr) &= ls.sampler->next_1d(ls.active) < q;
                    dr::masked(ls.throughput, use_rr) *= dr::rcp(q);
                }

                if (dr::none_or<false>(ls.active))
                    return;

                ls.ray = si.spawn_ray(si.to_world(bs.wo));

                PreliminaryIntersection3f next_pi = scene->ray_intersect_preliminary(
                    ls.ray,
                    /* coherent = */ false,
                    /* reorder = */ jit_flag(JitFlag::LoopRecord),
                    /* reorder_hint = */ 0,
                    /* reorder_hint_bits = */ 0,
                    ls.active);

                Mask depth_limit_reached = m_max_depth >= 0 && ls.depth >= m_max_depth,
                     escaped = ls.active && (depth_limit_reached || !next_pi.is_valid());

                if (dr::any_or<true>(escaped))
                    splat_measurement(sensor, block, ls.ray.wavelengths, ls.throughput,
                                      ls.ray, ls.depth, escaped);

                ls.pi = next_pi;
                ls.active &= !depth_limit_reached && next_pi.is_valid();
            },
            "Gonio Tracer Integrator");
    }

    void splat_surface_hit(ImageBlock *block, Mask active = true) const {
        if (dr::none_or<false>(active))
            return;

        Point2f pos(0.5f, 0.5f);
        if (block->channel_count() == 5) {
            Float values[5] = { 0.f, 0.f, 0.f, 0.f, 1.f };
            block->put(pos, values, active);
        } else {
            Float values[4] = { 0.f, 0.f, 0.f, 1.f };
            block->put(pos, values, active);
        }
    }

    void splat_measurement(const GonioSensor<Float, Spectrum> *sensor,
                           ImageBlock *block,
                           const Wavelength &wavelengths,
                           const Spectrum &value,
                           const Ray3f &output_ray,
                           Int32 depth,
                           Mask active = true) const {
        active &= depth > 0 && dr::any(unpolarized_spectrum(value) != 0.f);
        if (dr::none_or<false>(active))
            return;

        // Convert the escaped ray to spherical coordinates. The sensor folds
        // theta when needed and independently chooses the output layer.
        Vector3f dir = dr::normalize(output_ray.d);
        Float phi = dr::atan2(dir.y(), dir.x());
        Float theta = dr::acos(dr::clip(dir.z(), -1.f, 1.f));

        active = sensor->accepts_theta(theta, active);
        if (dr::none_or<false>(active))
            return;

        UInt32 cell = sensor->index(phi, sensor->fold_theta(theta), active);
        UInt32 layer = sensor->layer_index(theta, depth, active);
        UInt32 film_x = sensor->film_x(cell);

        Point2f pos(Float(film_x) + 0.5f, Float(layer) + 0.5f);
        block->put(pos, wavelengths, value, 0.f, 1.f, active);
    }

    std::string to_string() const override {
        return tfm::format("GonioTracerIntegrator[\n"
                           "  max_depth = %i,\n"
                           "  rr_depth = %i\n"
                           "]",
                           m_max_depth, m_rr_depth);
    }

    MI_DECLARE_CLASS(GonioTracerIntegrator)

private:
    uint32_t m_samples_per_pass;
    int m_max_depth;
    int m_rr_depth;
};

MI_EXPORT_PLUGIN(GonioTracerIntegrator)
NAMESPACE_END(mitsuba)
