#include <mitsuba/core/fwd.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/spectrum.h>
#include <mitsuba/core/string.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/ior.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/render/fresnel.h>
#include <mitsuba/core/warp.h>
#include "micrograin.h"

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class BlendPolySurfBulkBSDF final : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    BlendPolySurfBulkBSDF(const Properties &props) : Base(props) {
        size_t bsdf_count = 0;

        for (auto &prop : props.objects()) {
            auto *bsdf = prop.try_get<BSDF<Float, Spectrum>>();
            if (!bsdf)
                continue;

            if (bsdf_count == 0) {
                auto *poly =
                    dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf);
                if (!poly) {
                    Throw("BlendPolySurfBulkBSDF: First (surf) BSDF must "
                          "derive from PolyMicrograin.");
                }
                surf_bsdf = poly;
                bsdf_count++;
            } else if (bsdf_count == 1) {
                if (dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf) ||
                    dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf)) {
                    Throw("BlendPolySurfBulkBSDF: Second (bulk) BSDF cannot be "
                          "PolyMicrograin or Micrograin.");
                }
                bulk_bsdf = bsdf;
                bsdf_count++;
            } else {
                Throw("BlendPolySurfBulkBSDF: Cannot specify more than two "
                      "child BSDFs.");
            }
        }

        if (!surf_bsdf || !bulk_bsdf)
            Throw("BlendPolySurfBulkBSDF: Need exactly two child BSDFs "
                  "(surf=PolyMicrograin, bulk=other).");

        m_flags = surf_bsdf->flags() | bulk_bsdf->flags();
        m_components.clear();

        for (size_t i = 0; i < surf_bsdf->component_count(); ++i)
            m_components.push_back(surf_bsdf->flags(i));
        for (size_t i = 0; i < bulk_bsdf->component_count(); ++i)
            m_components.push_back(bulk_bsdf->flags(i));
    }

    void traverse(TraversalCallback *callback) override {
        callback->put("surf_bsdf", surf_bsdf, ParamFlags::NonDifferentiable);
        callback->put("bulk_bsdf", bulk_bsdf, ParamFlags::NonDifferentiable);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "BlendPolySurfBulkBSDF[" << std::endl
            << "  surf_bsdf = " << string::indent(surf_bsdf->to_string()) << ","
            << std::endl
            << "  bulk_bsdf = " << string::indent(bulk_bsdf->to_string())
            << std::endl
            << "]";
        return oss.str();
    }

    Spectrum eval(const BSDFContext &, const SurfaceInteraction3f &,
                  const Vector3f &, Mask) const override {
        Throw("eval not supported; use integrator <pathv2>.");
        return 0.f;
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &,
                                             const SurfaceInteraction3f &,
                                             Float, const Point2f &,
                                             Mask) const override {
        Throw("sample not supported; use integrator <pathv2>.");
        return { dr::zeros<BSDFSample3f>(), 0.f };
    }

    // ====================== eval_ex ======================
    Spectrum eval_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                     const Vector3f &wo, const Point2f &sample2_extra,
                     Mask active) const override {

        if (!dr::any_or<true>(active))
            return 0.f;

        // build cache once
        typename PolyMicrograin<Float, Spectrum>::PackedCache pc;
        surf_bsdf->build_packed_cache(si, pc, active);

        // reuse cache: global_tau0 + v1 + v2
        Float weight_surf = surf_bsdf->eval_global_tau_0_from_cache(pc, active);
        Float v1_bulk_wi =
            surf_bsdf->eval_visibility1_bulk_from_cache(pc, active);

        Vector3f wo1[PolyMicrograin<Float, Spectrum>::NbGrainMax];
        surf_bsdf->build_wo1_from_cache(pc, wo, wo1, active);
        Float v1_bulk_wo =
            surf_bsdf->eval_visibility1_bulk_from_cache(pc, wo1, active);
        Float v2_bulk =
            surf_bsdf->eval_visibility2_bulk_from_cache(pc, wo1, active);

        Float weight_bulk = eval_bulk_weight(si, wo, weight_surf, v1_bulk_wi,
                                             v1_bulk_wo, v2_bulk, active);

        Mask need_surf = active & (weight_surf > 0.f);
        Mask need_bulk = active & (weight_bulk > 0.f);

        Spectrum surf_val(0.f), bulk_val(0.f);

        if (dr::any_or<true>(need_surf))
            surf_val =
                surf_bsdf->eval_ex(ctx, si, wo, sample2_extra, need_surf);

        if (dr::any_or<true>(need_bulk))
            bulk_val =
                bulk_bsdf->eval_ex(ctx, si, wo, sample2_extra, need_bulk);

        Spectrum result = surf_val * weight_surf + bulk_val * weight_bulk;

        result = dr::select(active, result, 0.f);

        Mask accident_NaN = dr::any(dr::isnan(result));
        return dr::select(accident_NaN, 0.f, result);
    }

    // ====================== pdf ======================
    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {

        if (!dr::any_or<true>(active))
            return 0.f;

        // build cache once
        typename PolyMicrograin<Float, Spectrum>::PackedCache pc;
        surf_bsdf->build_packed_cache(si, pc, active);

        // reuse cache: global_tau0 + v1
        Float weight_surf = surf_bsdf->eval_global_tau_0_from_cache(pc, active);
        Float v1_bulk_wi =
            surf_bsdf->eval_visibility1_bulk_from_cache(pc, active);

        Float w_surf =
            eval_surf_sampling_weight(si, weight_surf, v1_bulk_wi, active);
        w_surf       = dr::clamp(w_surf, 0.f, 1.f);
        Float w_bulk = 1.f - w_surf;

        Mask need_surf = active & (w_surf > 0.f);
        Mask need_bulk = active & (w_bulk > 0.f);

        Float pdf_s = 0.f, pdf_b = 0.f;
        if (dr::any_or<true>(need_surf))
            pdf_s = surf_bsdf->pdf(ctx, si, wo, need_surf);
        if (dr::any_or<true>(need_bulk))
            pdf_b = bulk_bsdf->pdf(ctx, si, wo, need_bulk);

        Float pdf_ = w_surf * pdf_s + w_bulk * pdf_b;

        Mask accident_NaN = dr::isnan(pdf_);
        return dr::select(accident_NaN, 0.f, pdf_);
    }

    // ====================== sample_ex ======================
    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              Float sample1, const Point2f &sample2,
              const Point2f &sample2_extra, Mask active) const override {

        if (!dr::any_or<true>(active))
            return { dr::zeros<BSDFSample3f>(), 0.f };

        // build cache once (for weight_surf + v1, later reuse for v2)
        typename PolyMicrograin<Float, Spectrum>::PackedCache pc;
        surf_bsdf->build_packed_cache(si, pc, active);

        Float weight_surf = surf_bsdf->eval_global_tau_0_from_cache(pc, active);
        Float v1_bulk_wi =
            surf_bsdf->eval_visibility1_bulk_from_cache(pc, active);

        Float w_surf =
            eval_surf_sampling_weight(si, weight_surf, v1_bulk_wi, active);
        w_surf       = dr::clamp(w_surf, 0.f, 1.f);
        Float w_bulk = 1.f - w_surf;

        Mask surf_selected = active & (sample1 < w_surf);
        Mask bulk_selected = active & ~surf_selected;

        Float u1_s = dr::select(w_surf > 0.f, sample1 / w_surf, 0.f);
        Float u1_b = dr::select(w_bulk > 0.f, (sample1 - w_surf) / w_bulk, 0.f);

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result = 0.f;

        if (dr::any_or<true>(surf_selected)) {
            auto [bs_s, val_s] = surf_bsdf->sample_ex(
                ctx, si, u1_s, sample2, sample2_extra, surf_selected);
            dr::masked(bs, surf_selected)     = bs_s;
            dr::masked(result, surf_selected) = val_s;
        }

        if (dr::any_or<true>(bulk_selected)) {
            auto [bs_b, val_b] = bulk_bsdf->sample_ex(
                ctx, si, u1_b, sample2, sample2_extra, bulk_selected);
            dr::masked(bs, bulk_selected)     = bs_b;
            dr::masked(result, bulk_selected) = val_b;
        }

        // mixture pdf
        Mask need_surf = active & (w_surf > 0.f);
        Mask need_bulk = active & (w_bulk > 0.f);

        Float pdf_s = 0.f, pdf_b = 0.f;
        if (dr::any_or<true>(need_surf))
            pdf_s = surf_bsdf->pdf(ctx, si, bs.wo, need_surf);
        if (dr::any_or<true>(need_bulk))
            pdf_b = bulk_bsdf->pdf(ctx, si, bs.wo, need_bulk);

        Float pdf_mix = w_surf * pdf_s + w_bulk * pdf_b;
        bs.pdf        = pdf_mix;

        active &= dr::neq(pdf_mix, 0.f);
        if (!dr::any_or<true>(active))
            return { bs, 0.f };

        // reuse the SAME cache pc to compute v2 (no second cache_packed)
        Vector3f wo1[PolyMicrograin<Float, Spectrum>::NbGrainMax];
        surf_bsdf->build_wo1_from_cache(pc, bs.wo, wo1, active);
        Float v1_bulk_wo =
            surf_bsdf->eval_visibility1_bulk_from_cache(pc, wo1, active);
        Float v2_bulk =
            surf_bsdf->eval_visibility2_bulk_from_cache(pc, wo1, active);

        Float weight_bulk = eval_bulk_weight(
            si, bs.wo, weight_surf, v1_bulk_wi, v1_bulk_wo, v2_bulk, active);

        Float inv_ws = dr::rcp(dr::maximum(w_surf, 1e-8f));
        Float inv_wb = dr::rcp(dr::maximum(w_bulk, 1e-8f));

        result = dr::select(surf_selected, result * weight_surf * inv_ws,
                            result * weight_bulk * inv_wb);

        result = dr::select(active, result, 0.f);

        /*in case!*/
        Mask accident_NaN = dr::any(dr::isnan(result));
        return { bs, result & !accident_NaN };
    }

    // ================= helper weights  =================
    MI_INLINE Float eval_surf_sampling_weight(const SurfaceInteraction3f &si,
                                              const Float global_tau_0,
                                              const Float v1_bulk,
                                              Mask /*active*/ = true) const {
        Float v1 = (1.f - global_tau_0) * v1_bulk;
        return dr::select(Frame3f::cos_theta(si.wi) > 0.f,
                          global_tau_0 / (global_tau_0 + v1), global_tau_0);
    }

    MI_INLINE Float eval_bulk_weight(const SurfaceInteraction3f &si,
                                     const Vector3f &wo,
                                     const Float global_tau_0,
                                     const Float v1_bulk_wi,
                                     const Float v1_bulk_wo,
                                     const Float v2_bulk,
                                     Mask /*active*/ = true) const {
        Float weight = 1.f - global_tau_0;

        Mask wi_top  = (Frame3f::cos_theta(si.wi) > 0.f);
        Mask wo_top  = (Frame3f::cos_theta(wo) > 0.f);
        Mask all_top = wi_top & wo_top;
        Mask wi_only = wi_top & (~wo_top);
        Mask wo_only = wo_top & (~wi_top);

        weight = dr::select(all_top, weight * v2_bulk, weight);
        weight = dr::select(wi_only, weight * v1_bulk_wi, weight);
        weight = dr::select(wo_only, weight * v1_bulk_wo, weight);
        return weight;
    }

    MI_DECLARE_CLASS(BlendPolySurfBulkBSDF)

private:
    ref<PolyMicrograin<Float, Spectrum>> surf_bsdf;
    ref<BSDF<Float, Spectrum>> bulk_bsdf;

    MI_TRAVERSE_CB(Base, surf_bsdf, bulk_bsdf)
};

MI_EXPORT_PLUGIN(BlendPolySurfBulkBSDF)
NAMESPACE_END(mitsuba)
