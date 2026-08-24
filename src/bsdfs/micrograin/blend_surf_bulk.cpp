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
class BlendSurfBulkBSDF final : public BSDF<Float, Spectrum> {

public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    BlendSurfBulkBSDF(const Properties &props) : Base(props) {
        size_t bsdf_count = 0;

        for (auto &prop : props.objects()) {
            auto *bsdf = prop.try_get<BSDF<Float, Spectrum>>();
            if (!bsdf)
                continue;

            if (bsdf_count == 0) {
                auto *grain =
                    dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf);
                if (!grain) {
                    Throw("BlendSurfBulkBSDF: First (surf) BSDF must "
                          "derive from Micrograin.");
                }
                surf_bsdf = grain;
                bsdf_count++;
            } else if (bsdf_count == 1) {
                if (dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf) ||
                    dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf)) {
                    Throw("BlendSurfBulkBSDF: Second (bulk) BSDF cannot be "
                          "PolyMicrograin or Micrograin.");
                }
                bulk_bsdf = bsdf;
                bsdf_count++;
            } else {
                Throw("BlendSurfBulkBSDF: Cannot specify more than two "
                      "child BSDFs.");
            }
        }

        if (!surf_bsdf || !bulk_bsdf)
            Throw("BlendSurfBulkBSDF: Need exactly two child BSDFs "
                  "(surf=Micrograin, bulk=other).");

        m_flags = surf_bsdf->flags() | bulk_bsdf->flags();
        m_components.clear();

        for (size_t i = 0; i < surf_bsdf->component_count(); ++i)
            m_components.push_back(surf_bsdf->flags(i));
        for (size_t i = 0; i < bulk_bsdf->component_count(); ++i)
            m_components.push_back(bulk_bsdf->flags(i));
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "BlendSurfBulkBSDF[" << std::endl
            << "  surf_bsdf = " << string::indent(surf_bsdf) << "," << std::endl
            << "  bulk_bsdf = " << string::indent(bulk_bsdf) << std::endl
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
                                             Float, 
                                             const Point2f &, 
                                             Mask) const override {
        Throw("sample not supported; use integrator <pathv2>.");
        return { dr::zeros<BSDFSample3f>(), 0.f };
    }

    Spectrum eval_ex(const BSDFContext &ctx, 
                     const SurfaceInteraction3f &si,
                     const Vector3f &wo, 
                     const Point2f &sample2_extra,
                     Mask active) const override {
        // compute the weight of the micrograin BSDF
        Float tau_0 = surf_bsdf->eval_tau_0(si, active);
        // compute the weight of the bulk BSDF 
        // weight can be different depending on the wo and wi coming from the below surface 
        // (transparent bulk)
        Mask wi_top = Frame3f::cos_theta(si.wi) > 0.f;
        Mask wo_top = Frame3f::cos_theta(wo) > 0.f;
        Mask all_top = wi_top & wo_top;
        Float bulk_weight = 1.f - tau_0;
        bulk_weight = dr::select(all_top, 
                                 bulk_weight * surf_bsdf->eval_visibility2_bulk(si, wo, active), 
                                 bulk_weight); 
        bulk_weight = dr::select(wi_top & (!all_top), 
                                 bulk_weight * surf_bsdf->eval_visibility1_bulk(si, si.wi, active), 
                                 bulk_weight);    
        bulk_weight = dr::select(wo_top & (!all_top), 
                                 bulk_weight * surf_bsdf->eval_visibility1_bulk(si, wo, active), 
                                 bulk_weight);
        //get two bsdf values and combine them with the weights
        Spectrum surf_bsdf_value = surf_bsdf->eval_ex(ctx, si, wo, sample2_extra, active);
        Spectrum bulk_bsdf_value = bulk_bsdf->eval_ex(ctx, si, wo, sample2_extra, active);
        Spectrum value = tau_0 * surf_bsdf_value + bulk_weight * bulk_bsdf_value;
        return dr::select(active, value, 0.f);
    }

    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {
        //micrograin BSDF sampling weight
        Float tau_0 = surf_bsdf->eval_tau_0(si, active);
        Float v1 = surf_bsdf->eval_visibility1_bulk(si, active);
        Float surf_sampling_weight = tau_0 / (tau_0 + (1.f - tau_0) * v1);
        //bulk BSDF sampling weight
        Float bulk_sampling_weight = 1.f - surf_sampling_weight;
        //get two pdf values and combine them with the weights
        Float surf_pdf = surf_bsdf->pdf(ctx, si, wo, active);
        Float bulk_pdf = bulk_bsdf->pdf(ctx, si, wo, active);
        Float value = surf_sampling_weight * surf_pdf + 
                      bulk_sampling_weight * bulk_pdf;
        return value;
    }

    std::pair<BSDFSample3f, Spectrum> 
    sample_ex(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              Float sample1,
              const Point2f &sample2,
              const Point2f &sample2_extra,
              Mask active) const override {
        //micrograin BSDF sampling weight
        Float tau_0 = surf_bsdf->eval_tau_0(si, active);
        Float v1 = surf_bsdf->eval_visibility1_bulk(si, active);
        Float surf_sampling_weight = tau_0 / (tau_0 + (1.f - tau_0) * v1);
        //bulk BSDF sampling weight
        Float bulk_sampling_weight = 1.f - surf_sampling_weight;
        
        Mask surf_selected = sample1 < surf_sampling_weight;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result = 0.f;

        std::tie(bs, result) = dr::select(
            surf_selected,
            surf_bsdf->sample_ex(ctx, si,
                                 sample1 / surf_sampling_weight,
                                 sample2, sample2_extra, active),
            bulk_bsdf->sample_ex(ctx, si,
                                (sample1 - surf_sampling_weight) /
                                 bulk_sampling_weight,
                                 sample2, sample2_extra, active));
        bs.pdf = pdf(ctx, si, bs.wo, active);
        active &= dr::neq(bs.pdf, 0.f);
        Float inv_ssw = dr::rcp(dr::maximum(surf_sampling_weight, 1e-8f));
        Float inv_bsw = dr::rcp(dr::maximum(bulk_sampling_weight, 1e-8f));

        // compute the weight of the bulk BSDF 
        // weight can be different depending on the wo and wi coming from the below surface 
        // (transparent bulk)
        Mask wi_top = Frame3f::cos_theta(si.wi) > 0.f;
        Mask wo_top = Frame3f::cos_theta(bs.wo) > 0.f;
        Mask all_top = wi_top & wo_top;
        Float bulk_weight = 1.f - tau_0;
        bulk_weight = dr::select(all_top, 
                                 bulk_weight * surf_bsdf->eval_visibility2_bulk(si, bs.wo, active), 
                                 bulk_weight); 
        bulk_weight = dr::select(wi_top & (!all_top), 
                                 bulk_weight * surf_bsdf->eval_visibility1_bulk(si, si.wi, active), 
                                 bulk_weight);    
        bulk_weight = dr::select(wo_top & (!all_top), 
                                 bulk_weight * surf_bsdf->eval_visibility1_bulk(si, bs.wo, active), 
                                 bulk_weight);
        result = dr::select(surf_selected, result * tau_0 * inv_ssw, result * bulk_weight * inv_bsw);
        return { bs, result & active };
    }


    MI_DECLARE_CLASS(BlendSurfBulkBSDF)

    
private:
    ref<MicrograinBSDF<Float, Spectrum>> surf_bsdf;
    ref<BSDF<Float, Spectrum>> bulk_bsdf;


    MI_TRAVERSE_CB(Base, surf_bsdf, bulk_bsdf)
    

};

MI_EXPORT_PLUGIN(BlendSurfBulkBSDF)
NAMESPACE_END(mitsuba)