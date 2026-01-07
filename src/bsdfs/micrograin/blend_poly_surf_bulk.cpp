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
            if (!bsdf) continue;

            if (bsdf_count == 0) {
                auto *poly = dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf);
                if (!poly) {
                    Throw("BlendPolySurfBulkBSDF: The first(surf) BSDF "
                          "must be PolyMicrograinBSDF.");
                }
                surf_bsdf = poly;
                bsdf_count++;
            } else if (bsdf_count == 1) {
                // if is PolyMicrograin
                if (dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf) ||
                    dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf)) {
                    Throw("BlendPolySurfBulkBSDF: The second(bulk) BSDF "
                          "cannot be PolyMicrograinBSDF or MicrograinBSDF.");
                }
                bulk_bsdf = bsdf;
                bsdf_count++;
            } else { // more than 2 bsdfs
                Throw("BlendPolySurfBulkBSDF: Cannot specify more than "
                      "two child BSDFs: "
                      "first PolyMicrograin, second a non-Micrograin BSDF.");
            }
        }
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
            << "  surf_bsdf = " << string::indent(surf_bsdf->to_string()) << "," << std::endl
            << "  bulk_bsdf = " << string::indent(bulk_bsdf->to_string()) << std::endl
            << "]";
        return oss.str();
    }

    Spectrum eval(const BSDFContext & /* ctx*/,
                  const SurfaceInteraction3f & /* si*/,
                  const Vector3f & /* wo*/, Mask /* active*/) const override {
        Throw("eval function not supported for anisotropic rough diffuse "
              "material due to failed generation stochastic samples, use "
              "integrator <pathv2> instead ");
        return 0.f;
    }
    std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext & /* ctx*/, const SurfaceInteraction3f & /* si*/,
           Float /* sample1 */, const Point2f & /* sample2*/,
           Mask /* active*/) const override {
        Throw("sample function not supported for anisotropic rough diffuse "
              "material due to failed generation stochastic samples, use "
              "integrator <pathv2> instead ");
        return { dr::zeros<BSDFSample3f>(), 0.f };
    }

    /*Spectrum eval_ex(const BSDFContext &ctx, 
                     const SurfaceInteraction3f &si,
                     const Vector3f &wo, 
                     const Point2f &sample2_extra,
                     Mask active) const override {
        Float weight_surf = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk     = surf_bsdf->eval_visibility1_bulk(si, active);
        Float v2_bulk     = surf_bsdf->eval_visibility2_bulk(si, wo, active);

        Float weight_bulk  = eval_bulk_weight(si, wo, weight_surf, v1_bulk, v2_bulk, active);

        Spectrum surf_bsdf_value = surf_bsdf->eval_ex(ctx, si, wo, sample2_extra, active);
        Spectrum bulk_bsdf_value = bulk_bsdf->eval_ex(ctx, si, wo, sample2_extra, active);

        Spectrum result = surf_bsdf_value * weight_surf + bulk_bsdf_value * weight_bulk;
        return dr::select(active, result, 0.f);
    }*/


    Spectrum eval_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                     const Vector3f &wo, const Point2f &sample2_extra,
                     Mask active) const override {

        // Compute common weights once
        Float weight_surf = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk     = surf_bsdf->eval_visibility1_bulk(si, active);
        Float v2_bulk     = surf_bsdf->eval_visibility2_bulk(si, wo, active);

        Float weight_bulk =
            eval_bulk_weight(si, wo, weight_surf, v1_bulk, v2_bulk, active);

        // Optional: skip heavy BSDF evals when weight is zero (or effectively
        // zero) This DOES NOT change the formula, only avoids useless work.
        Mask need_surf = active & (weight_surf > 0.f);
        Mask need_bulk = active & (weight_bulk > 0.f);

        Spectrum surf_val(0.f);
        Spectrum bulk_val(0.f);

        if (dr::any_or<true>(need_surf)) {
            surf_val =
                surf_bsdf->eval_ex(ctx, si, wo, sample2_extra, need_surf);
        }
        if (dr::any_or<true>(need_bulk)) {
            bulk_val =
                bulk_bsdf->eval_ex(ctx, si, wo, sample2_extra, need_bulk);
        }

        Spectrum result = surf_val * weight_surf + bulk_val * weight_bulk;
        return dr::select(active, result, 0.f);
    }


    /*Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override { 
        Float global_tau_0         = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk              = surf_bsdf->eval_visibility1_bulk(si, active);

        Float sampling_weight_surf = eval_surf_sampling_weight(si, global_tau_0, v1_bulk, active);
        Float sampling_weight_bulk = 1.f - sampling_weight_surf;

        Float pdf_ = sampling_weight_surf * surf_bsdf->pdf(ctx, si, wo, active) +
                     sampling_weight_bulk * bulk_bsdf->pdf(ctx, si, wo, active);
        return pdf_;
    }*/


    //Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
    //          const Vector3f &wo, Mask active) const override {

    //    // Compute once
    //    Float weight_surf = surf_bsdf->eval_global_tau_0(si, active);
    //    Float v1_bulk     = surf_bsdf->eval_visibility1_bulk(si, active);

    //    Float w_surf =
    //        eval_surf_sampling_weight(si, weight_surf, v1_bulk, active);
    //    Float w_bulk = 1.f - w_surf;

    //    // Mixture pdf (same math as before)
    //    Float pdf_s = surf_bsdf->pdf(ctx, si, wo, active);
    //    Float pdf_b = bulk_bsdf->pdf(ctx, si, wo, active);

    //    return w_surf * pdf_s + w_bulk * pdf_b;
    //}

    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {

        // Compute once
        Float weight_surf = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk     = surf_bsdf->eval_visibility1_bulk(si, active);

        Float w_surf =
            eval_surf_sampling_weight(si, weight_surf, v1_bulk, active);
        Float w_bulk = 1.f - w_surf;

        // Mixture pdf (same math as before)
        Float pdf_s = surf_bsdf->pdf(ctx, si, wo, active);
        Float pdf_b = bulk_bsdf->pdf(ctx, si, wo, active);

        return w_surf * pdf_s + w_bulk * pdf_b;
    }


    /*std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              Float sample1, 
              const Point2f &sample2,
              const Point2f &sample2_extra, 
              Mask active) const override {
        Float weight_surf          = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk              = surf_bsdf->eval_visibility1_bulk(si, active);

        Float sampling_weight_surf = eval_surf_sampling_weight(si, weight_surf, v1_bulk, active);
        Float sampling_weight_bulk = 1.f - sampling_weight_surf;

        Mask surf_selected = sample1 < sampling_weight_surf;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result = 0.f;

        std::tie(bs, result) = dr::select(
            surf_selected,
            surf_bsdf->sample_ex(ctx, si,
                                 sample1 / sampling_weight_surf,
                                 sample2, sample2_extra, active),
            bulk_bsdf->sample_ex(ctx, si,
                                (sample1 - sampling_weight_surf) /
                                 sampling_weight_bulk,
                                 sample2, sample2_extra, active));
        bs.pdf = pdf(ctx, si, bs.wo, active);

        active &= dr::neq(bs.pdf, 0.f);

        Float v2_bulk = surf_bsdf->eval_visibility2_bulk(si, bs.wo, active);

        Float weight_bulk = eval_bulk_weight(si, bs.wo, weight_surf, v1_bulk, v2_bulk, active);

        result = dr::select(surf_selected, 
                            result * weight_surf / sampling_weight_surf,
                            result * weight_bulk / sampling_weight_bulk);

        return { bs, result & active};
    }*/


    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              Float sample1, const Point2f &sample2,
              const Point2f &sample2_extra, Mask active) const override {

        // ---- compute once ----
        Float weight_surf = surf_bsdf->eval_global_tau_0(si, active);
        Float v1_bulk     = surf_bsdf->eval_visibility1_bulk(si, active);

        Float sampling_weight_surf =
            eval_surf_sampling_weight(si, weight_surf, v1_bulk, active);
        Float sampling_weight_bulk = 1.f - sampling_weight_surf;

        // pick branch (statistically correct mixture sampling)
        Mask surf_selected = active & (sample1 < sampling_weight_surf);
        Mask bulk_selected = active & ~surf_selected;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result = 0.f;

        // ---- sample SURF only where selected ----
        if (dr::any_or<true>(surf_selected)) {
            Float u1_surf = sample1 / sampling_weight_surf;

            auto [bs_s, res_s] = surf_bsdf->sample_ex(
                ctx, si, u1_surf, sample2, sample2_extra, surf_selected);
            dr::masked(bs, surf_selected)     = bs_s;
            dr::masked(result, surf_selected) = res_s;
        }

        // ---- sample BULK only where selected ----
        if (dr::any_or<true>(bulk_selected)) {
            Float u1_bulk =
                (sample1 - sampling_weight_surf) / sampling_weight_bulk;

            auto [bs_b, res_b] = bulk_bsdf->sample_ex(
                ctx, si, u1_bulk, sample2, sample2_extra, bulk_selected);
            dr::masked(bs, bulk_selected)     = bs_b;
            dr::masked(result, bulk_selected) = res_b;
        }

        // ---- compute mixture pdf inline (avoid calling pdf(), no duplicate
        // global_tau_0/v1_bulk) ---- Let child pdfs handle their own validity
        // checks.
        Float pdf_surf = surf_bsdf->pdf(ctx, si, bs.wo, active);
        Float pdf_bulk = bulk_bsdf->pdf(ctx, si, bs.wo, active);
        bs.pdf =
            sampling_weight_surf * pdf_surf + sampling_weight_bulk * pdf_bulk;

        active &= dr::neq(bs.pdf, 0.f);

        // ---- bulk weight needs v2 at the actually-sampled wo ----
        Float v2_bulk = surf_bsdf->eval_visibility2_bulk(si, bs.wo, active);
        Float weight_bulk =
            eval_bulk_weight(si, bs.wo, weight_surf, v1_bulk, v2_bulk, active);

        // ---- same correction logic as your original code ----
        result = dr::select(surf_selected,
                            result * weight_surf / sampling_weight_surf,
                            result * weight_bulk / sampling_weight_bulk);

        return { bs, result & active };
    }




    //**********************************************************//
    //*-------------- helper functions (poly) -----------------*//
    //**********************************************************//
    Float eval_surf_sampling_weight(const SurfaceInteraction3f &si,
                                    const Float global_tau_0,
                                    const Float v1_bulk,
                                    Mask active = true) const {
        Float v1 = (1.f - global_tau_0) * v1_bulk;
        Float weight =
            dr::select(Frame3f::cos_theta(si.wi) > 0.f,
                       global_tau_0 / (global_tau_0 + v1), global_tau_0);
        return weight;
    }

    Float eval_bulk_weight(const SurfaceInteraction3f &si, 
                           const Vector3f &wo,
                           const Float global_tau_0, 
                           const Float v1_bulk,
                           const Float v2_bulk,                   
                           Mask active = true) const {
        Float weight = 1.f - global_tau_0;
        Mask wi_top  = (Frame3f::cos_theta(si.wi) > 0.f);
        Mask wo_top  = (Frame3f::cos_theta(wo) > 0.f);
        Mask all_top = wi_top & wo_top;

        weight = dr::select(
            all_top,
            weight * v2_bulk,
            weight);
        weight = dr::select(
            wi_top & (~all_top),
            weight * v1_bulk,
            weight);
        weight = dr::select(
            wo_top & (~all_top),
            weight * v1_bulk,
            weight);

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