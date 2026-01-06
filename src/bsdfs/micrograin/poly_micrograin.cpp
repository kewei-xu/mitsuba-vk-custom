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
class PolyMicrograinBSDF final : public PolyMicrograin<Float, Spectrum> {
public:
    MI_IMPORT_BASE(PolyMicrograin, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    PolyMicrograinBSDF(const Properties &props) : Base(props) {
        // has diffuse ?
        m_has_diffuse_component = false;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            if (has_flag(m_micrograin_bsdfs[i]->flags(),
                         BSDFFlags::DiffuseReflection)) {
                m_has_diffuse_component = true;
                break;
            }
        }
    }

    void traverse(TraversalCallback *callback) override {
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            if (m_micrograin_bsdfs[i]) {
                std::string name = "micrograin_bsdf_" + std::to_string(i+1);
                callback->put(name, m_micrograin_bsdfs[i], ParamFlags::NonDifferentiable);
            }
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "PolyMicrograin[" << std::endl;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            oss << "  micrograin_bsdf_" << (i+1) << "=" << string::indent(m_micrograin_bsdfs[i]->to_string()) << "," << std::endl;
        }
        oss << "]";
        return oss.str();
    }

    Spectrum eval(const BSDFContext &ctx,
                  const SurfaceInteraction3f &si,
                  const Vector3f &wo, 
                  Mask active) const override {
        if (!m_has_diffuse_component) {
            Throw("eval function of pure specular "
                  "material to support "
                  "integrator <pathv1> is still under development ");
            return 0.f;
        } else {
            Throw("eval function not supported for anisotropic rough diffuse "
                  "material due to failed generation stochastic samples, use "
                  "integrator <pathv2> instead ");
            return 0.f;
        }
    }

    std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext &ctx, 
           const SurfaceInteraction3f &si,
           Float sample1, 
           const Point2f &sample2,
           Mask active) const override {
        if (!m_has_diffuse_component) {
            Throw("sample function of pure specular "
                  "material to support "
                  "integrator <pathv1> is still under development ");
            return { dr::zeros<BSDFSample3f>(), 0.f };
        } else {
            Throw("sample function not supported for anisotropic rough diffuse "
                  "material due to failed generation stochastic samples, use "
                  "integrator <pathv2> instead ");
            return { dr::zeros<BSDFSample3f>(), 0.f };
        }
    }

    //std::pair<BSDFSample3f, Spectrum>
    //sample_ex(const BSDFContext &ctx, 
    //          const SurfaceInteraction3f &si,
    //          Float sample1, 
    //          const Point2f &sample2, 
    //          const Point2f &sample2_extra,
    //          Mask active) const override {

    //    Float cos_theta_i = Frame3f::cos_theta(si.wi);
    //    active &= cos_theta_i > 0.f;
    //    //----------ascending sorting----------//
    //    Float a[NbGrainMax], b[NbGrainMax], c[NbGrainMax], d[NbGrainMax],
    //        r[NbGrainMax], tau_0[NbGrainMax], spec_sampling_proba[NbGrainMax];
    //    for (size_t i = 0; i < NbGrainMax; ++i) {
    //        a[i]     = 0.f;
    //        b[i]     = 0.f;
    //        c[i]     = 0.f;
    //        d[i]     = 0.f;
    //        r[i]     = 0.f; 
    //        tau_0[i] = 0.f;
    //        spec_sampling_proba[i] = 0.f;
    //    }
    //    for (size_t i = 0; i < m_bsdf_count; ++i) {
    //        a[i]     = m_micrograin_bsdfs[i]->eval_a(si, active);
    //        b[i]     = m_micrograin_bsdfs[i]->eval_b(si, active);
    //        c[i]     = m_micrograin_bsdfs[i]->eval_c(si, active);
    //        d[i]     = m_micrograin_bsdfs[i]->eval_d(si, active);
    //        r[i]     = m_micrograin_bsdfs[i]->eval_radius(si, active);
    //        tau_0[i] = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
    //        spec_sampling_proba[i] =
    //            m_micrograin_bsdfs[i]->specular_component_sampling_probability(
    //                cos_theta_i);
    //    }
    //    this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
    //                                spec_sampling_proba);

    //    Float global_tau_0 = this->eval_global_tau_0(si, active);

    //    Float term_kappa  = 1.f - global_tau_0;
    //    Float term_lambda = this->term_lambda_base(si, active);

    //    Float h_lower = 0.f;
    //    Float h_upper = 0.f;
    //
    //    
    //    BSDFSample3f bs   = dr::zeros<BSDFSample3f>();
    //    
    //    Spectrum result(0.f);
    //    Float p_level_cum = 0.f;

    //    // start sampling
    //    for (size_t i = 0; i < m_bsdf_count; ++i) { // for layers (heightfield)
    //        h_upper = r[i];
    //        Float p_level = this->proba_level(global_tau_0, term_kappa,
    //                                          term_lambda, h_upper, h_lower);
    //        Float p_level_cum_pre = p_level_cum;
    //        p_level_cum += p_level;

    //        Mask level_selected = (sample1 >= p_level_cum_pre) & (sample1 < p_level_cum);

    //        Float sample1_1 = (sample1 - p_level_cum_pre) / p_level;

    //        Float p_type_cum = 0.f;

    //        for (size_t j = i; j < m_bsdf_count; ++j) { // for micrograin types in layer i
    //            Float p_type = this->proba_level_type(tau_0[j], r[j], term_lambda);
    //            Float p_type_cum_pre = p_type_cum;
    //            p_type_cum += p_type;

    //            Mask type_selected = level_selected &
    //                                 (sample1_1 >= p_type_cum_pre) &
    //                                 (sample1_1 < p_type_cum);

    //            Float sample1_2 = (sample1_1 - p_type_cum_pre) / p_type;
    //            Mask spec_selected = level_selected & type_selected &
    //                                 (sample1_2 < spec_sampling_proba[j]);

    //            Matrix3f M(a[j], b[j], 0.f, c[j], d[j], 0.f, 0.f, 0.f, 1.f);
    //            Float abs_det_M = dr::abs(a[j] * d[j] - b[j] * c[j]);
    //            Matrix3f M_inv  = dr::inverse(M);
    //            Matrix3f M_inv_T = dr::transpose(M_inv);

    //            Vector3f wh_1 = square_to_sphere_micrograin_conditional_level_and_type(
    //                    term_lambda, r[j], h_upper, h_lower, sample2
    //            );

    //            Normal3f wh = dr::normalize(M_inv_T * wh_1);
    //            Vector3f wo = dr::select(
    //                spec_selected,
    //                reflect(si.wi, wh),
    //                warp::square_to_cosine_hemisphere(sample2)
    //            );
    //            Float cos_theta_o = Frame3f::cos_theta(wo);
    //            Float pdf_ = this->pdf(ctx, si, wo, active);
    //            BSDFSample3f bs_tmp(wo);
    //            bs_tmp.pdf = pdf_;
    //            bs_tmp.sampled_type = dr::select(
    //                spec_selected,
    //                +BSDFFlags::GlossyReflection,
    //                +BSDFFlags::DiffuseReflection);
    //            Mask pdf_valid = dr::neq(pdf_, 0.f);
    //            dr::masked(bs, level_selected && type_selected) = bs_tmp;
    //            dr::masked(result, level_selected && type_selected) = dr::select(
    //                    active & Frame3f::cos_theta(wo) > 0.f & pdf_valid &,
    //                this->eval_ex(ctx, si, wo, sample2_extra, active) / pdf_, 
    //                0.f
    //            );
    //        }
    //        h_lower = h_upper;
    //        Float cur_term_kappa = 1.f - tau_0[i];
    //        Float cur_term_lambda =
    //            dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));

    //        term_kappa /= cur_term_kappa;
    //        term_lambda /= cur_term_lambda;
    //    }
    //    /*Mask accident_NaN = dr::any(dr::isnan(result));
    //    return { bs, result & ~accident_NaN };*/
    //    return { bs, result };
    //}


    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              Float sample1, const Point2f &sample2,
              const Point2f &sample2_extra, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;

        // ---------- ascending sorting (same as your code) ----------
        Float a[NbGrainMax], b[NbGrainMax], c[NbGrainMax], d[NbGrainMax];
        Float r[NbGrainMax], tau_0[NbGrainMax], spec_sampling_proba[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            a[k] = b[k] = c[k] = d[k] = 0.f;
            r[k] = tau_0[k] = spec_sampling_proba[k] = 0.f;
        }

        for (size_t k = 0; k < m_bsdf_count; ++k) {
            a[k]     = m_micrograin_bsdfs[k]->eval_a(si, active);
            b[k]     = m_micrograin_bsdfs[k]->eval_b(si, active);
            c[k]     = m_micrograin_bsdfs[k]->eval_c(si, active);
            d[k]     = m_micrograin_bsdfs[k]->eval_d(si, active);
            r[k]     = m_micrograin_bsdfs[k]->eval_radius(si, active);
            tau_0[k] = m_micrograin_bsdfs[k]->eval_tau_0(si, active);
            spec_sampling_proba[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
                                    spec_sampling_proba);

        // ---------- global terms (same as your code) ----------
        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Float term_kappa  = 1.f - global_tau_0;
        Float term_lambda = this->term_lambda_base(si, active);

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        // ---------- outputs ----------
        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result(0.f);

        // ---------- "chosen" storage (per-lane) ----------
        // Store exactly what is needed to reproduce the same math for the
        // selected (i,j)
        Float ch_a = 0.f, ch_b = 0.f, ch_c = 0.f, ch_d = 0.f;
        Float ch_r = 0.f, ch_tau0 = 0.f, ch_p_spec = 0.f;
        Float ch_h_lower = 0.f, ch_h_upper = 0.f;
        Float ch_term_lambda = 1.f;
        Float ch_sample1_2   = 0.f;   // sample1_2 used for spec/diff choice
        Mask ch_selected     = false; // level_selected & type_selected
        Mask ch_spec_selected =
            false; // level_selected & type_selected & (sample1_2 < p_spec)

        Float p_level_cum = 0.f;

        // ---------- sampling selection loops (same probability model as your
        // code) ----------
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            h_upper = r[i];

            Float p_level         = this->proba_level(global_tau_0, term_kappa,
                                                      term_lambda, h_upper, h_lower);
            Float p_level_cum_pre = p_level_cum;
            p_level_cum += p_level;

            Mask level_selected =
                (sample1 >= p_level_cum_pre) & (sample1 < p_level_cum);

            // IMPORTANT: keep division EXACTLY as your original code (no
            // guards)
            Float sample1_1 = (sample1 - p_level_cum_pre) / p_level;

            Float p_type_cum = 0.f;

            for (size_t j = i; j < m_bsdf_count; ++j) {
                Float p_type =
                    this->proba_level_type(tau_0[j], r[j], term_lambda);
                Float p_type_cum_pre = p_type_cum;
                p_type_cum += p_type;

                Mask type_selected = level_selected &
                                     (sample1_1 >= p_type_cum_pre) &
                                     (sample1_1 < p_type_cum);

                // IMPORTANT: keep division EXACTLY as your original code (no
                // guards)
                Float sample1_2 = (sample1_1 - p_type_cum_pre) / p_type;

                Mask spec_selected = level_selected & type_selected &
                                     (sample1_2 < spec_sampling_proba[j]);

                // Commit the chosen parameters ONLY where selected (exactly one
                // pair per lane)
                dr::masked(ch_a, type_selected) = a[j];
                dr::masked(ch_b, type_selected) = b[j];
                dr::masked(ch_c, type_selected) = c[j];
                dr::masked(ch_d, type_selected) = d[j];

                dr::masked(ch_r, type_selected)      = r[j];
                dr::masked(ch_tau0, type_selected)   = tau_0[j];
                dr::masked(ch_p_spec, type_selected) = spec_sampling_proba[j];

                dr::masked(ch_h_lower, type_selected) = h_lower;
                dr::masked(ch_h_upper, type_selected) = h_upper;

                // store the layer's term_lambda (critical for identical wh_1
                // sampling)
                dr::masked(ch_term_lambda, type_selected) = term_lambda;

                // store sample1_2 and spec selection (for identical spec/diff
                // branching)
                dr::masked(ch_sample1_2, type_selected)     = sample1_2;
                dr::masked(ch_spec_selected, type_selected) = spec_selected;

                ch_selected |= (level_selected & type_selected);
            }

            // advance layer (same as your code)
            h_lower = h_upper;

            Float cur_term_kappa = 1.f - tau_0[i];
            Float cur_term_lambda =
                dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));

            term_kappa /= cur_term_kappa;
            term_lambda /= cur_term_lambda;
        }

        // If nothing was selected (numerical edge), keep zero (same practical
        // behavior)
        Mask ok = active & ch_selected;

        // ---------- reproduce the EXACT same sampling math for the selected
        // (i,j) ----------
        Matrix3f M(ch_a, ch_b, 0.f, ch_c, ch_d, 0.f, 0.f, 0.f, 1.f);

        Float abs_det_M = dr::abs(ch_a * ch_d - ch_b * ch_c);
        (void) abs_det_M; // kept for parity (not strictly needed here)

        Matrix3f M_inv   = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);

        Vector3f wh_1 = square_to_sphere_micrograin_conditional_level_and_type(
            ch_term_lambda, ch_r, ch_h_upper, ch_h_lower, sample2);

        Normal3f wh = dr::normalize(M_inv_T * wh_1);

        Vector3f wo = dr::select(ch_spec_selected, reflect(si.wi, wh),
                                 warp::square_to_cosine_hemisphere(sample2));

        Float cos_theta_o = Frame3f::cos_theta(wo);

        // In your original code you did: active &= cos_theta_o > 0;
        // For identical math on the selected branch, use a local mask (does not
        // change outcome)
        Mask active_sel = ok & (cos_theta_o > 0.f);

        Float pdf_ = this->pdf(ctx, si, wo, active_sel);
        BSDFSample3f bs_tmp(wo);
        bs_tmp.pdf = pdf_;
        bs_tmp.sampled_type =
            dr::select(ch_spec_selected, +BSDFFlags::GlossyReflection,
                       +BSDFFlags::DiffuseReflection);

        Mask pdf_valid = dr::neq(pdf_, 0.f);

        // Exactly like: dr::masked(bs, level_selected && type_selected) =
        // bs_tmp;
        dr::masked(bs, ch_selected) = bs_tmp;

        // Exactly like:
        // dr::masked(result, level_selected && type_selected) =
        //     dr::select(active & pdf_valid, eval_ex(...)/pdf_, 0)
        Spectrum val = this->eval_ex(ctx, si, wo, sample2_extra, active_sel);
        Spectrum contrib = dr::select(active_sel & pdf_valid, val / pdf_, 0.f);
        dr::masked(result, ch_selected) = contrib;

        return { bs, result };
    }


    //Float pdf(const BSDFContext &ctx, 
    //          const SurfaceInteraction3f &si,
    //          const Vector3f &wo, 
    //          Mask active) const override {
    //    Float cos_theta_i = Frame3f::cos_theta(si.wi);
    //    Float cos_theta_o = Frame3f::cos_theta(wo);
    //    Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
    //    active &= valid_general;

    //    //----------ascending sorting----------//
    //    Float a[NbGrainMax], 
    //          b[NbGrainMax], 
    //          c[NbGrainMax], 
    //          d[NbGrainMax],
    //          r[NbGrainMax], 
    //          tau_0[NbGrainMax],
    //          spec_sampling_proba[NbGrainMax];
    //    for (size_t i = 0; i < NbGrainMax; ++i) {
    //        a[i]                   = 0.f;
    //        b[i]                   = 0.f;
    //        c[i]                   = 0.f;
    //        d[i]                   = 0.f;
    //        r[i]                   = 0.f;
    //        tau_0[i]               = 0.f;
    //        spec_sampling_proba[i] = 0.f;
    //    }
    //    for (size_t i = 0; i < m_bsdf_count; ++i) {
    //        a[i]     = m_micrograin_bsdfs[i]->eval_a(si, active);
    //        b[i]     = m_micrograin_bsdfs[i]->eval_b(si, active);
    //        c[i]     = m_micrograin_bsdfs[i]->eval_c(si, active);
    //        d[i]     = m_micrograin_bsdfs[i]->eval_d(si, active);
    //        r[i]     = m_micrograin_bsdfs[i]->eval_radius(si, active);
    //        tau_0[i] = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
    //        spec_sampling_proba[i] = m_micrograin_bsdfs[i]->specular_component_sampling_probability(cos_theta_i);
    //    }
    //    this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
    //                                spec_sampling_proba);

    //    Float global_tau_0 = this->eval_global_tau_0(si, active);

    //    Float term_kappa = 1.f - global_tau_0;
    //    Float term_lambda = this->term_lambda_base(si, active);

    //    Float h_lower = 0.f;
    //    Float h_upper = 0.f;

    //    //------------ specular variables -----------//
    //    Vector3f wh = dr::normalize(si.wi + wo);
    //    Float cos_theta_wh = Frame3f::cos_theta(wh);

    //    Mask valid_spec = (dr::dot(si.wi, wh) > 0.f) & (dr::dot(wo, wh) > 0.f);

    //    //---- compute pdf ----//
    //    Float pdf_ = 0.f;
    //    for (size_t i = 0; i < m_bsdf_count; ++i) { // for layers (heightfield)

    //        h_upper = r[i];
    //        Float p_level = this->proba_level(global_tau_0, term_kappa, term_lambda, h_upper, h_lower);
    //        Mask valid_level = dr::neq(h_upper - h_lower, 0.f);

    //        for (size_t j = i; j < m_bsdf_count; ++j) { // for micrograin types in layer i

    //            Float p_type = this->proba_level_type(tau_0[j], r[j], term_lambda);

    //            Matrix3f M(a[j], b[j], 0.f, c[j], d[j], 0.f, 0.f, 0.f, 1.f);
    //            Float abs_det_M = dr::abs(a[j] * d[j] - b[j] * c[j]);

    //            Matrix3f M_T = dr::transpose(M);
    //            
    //            //------ spec pdf -------//
    //            Vector3f wh_1_tmp = M_T * wh;
    //            Vector3f wh_1     = dr::normalize(wh_1_tmp);
    //            Float norm_sqr    = dr::squared_norm(wh_1_tmp);
    //            Float hv_height = Frame3f::cos_theta(wh_1) * r[j];
    //            Mask inRange    = (hv_height > h_lower) & (hv_height <= h_upper);
    //            Float pdf_spec = this->square_to_micrograin_conditional_level_and_type_pdf(
    //                term_lambda, r[j],
    //                h_upper, h_lower,
    //                abs_det_M, norm_sqr, wh
    //            ) / (4.f * dr::dot(wo, wh));
    //            pdf_ += dr::select(active & valid_level & inRange & valid_spec,
    //                               p_level * p_type * spec_sampling_proba[j] * pdf_spec, 0.f);
    //            //------ diff pdf -------//
    //            Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);
    //            
    //            pdf_ += dr::select(active & valid_level,
    //                               p_level * p_type * (1.f - spec_sampling_proba[j]) * pdf_diff, 0.f);
    //        }
    //        h_lower = h_upper;
    //        Float cur_term_kappa = 1.f - tau_0[i];
    //        Float cur_term_lambda = dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));
    //        
    //        term_kappa /= cur_term_kappa;
    //        term_lambda /= cur_term_lambda;
    //    }
    //    /*Mask accident_NaN = dr::isnan(pdf_);
    //    return dr::select(accident_NaN, 0.f, pdf_);*/
    //    return pdf_;
    //}
    
    
    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        active &= valid_general;

        // ---------- gather + sort (same as your code) ----------
        Float a[NbGrainMax], b[NbGrainMax], c[NbGrainMax], d[NbGrainMax];
        Float r[NbGrainMax], tau_0[NbGrainMax], spec_sampling_proba[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            a[k] = b[k] = c[k] = d[k] = 0.f;
            r[k] = tau_0[k] = spec_sampling_proba[k] = 0.f;
        }
        for (size_t k = 0; k < m_bsdf_count; ++k) {
            a[k]     = m_micrograin_bsdfs[k]->eval_a(si, active);
            b[k]     = m_micrograin_bsdfs[k]->eval_b(si, active);
            c[k]     = m_micrograin_bsdfs[k]->eval_c(si, active);
            d[k]     = m_micrograin_bsdfs[k]->eval_d(si, active);
            r[k]     = m_micrograin_bsdfs[k]->eval_radius(si, active);
            tau_0[k] = m_micrograin_bsdfs[k]->eval_tau_0(si, active);
            spec_sampling_proba[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
                                    spec_sampling_proba);

        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Float term_kappa  = 1.f - global_tau_0;
        Float term_lambda = this->term_lambda_base(si, active);

        // ---------- specular variables ----------
        Vector3f wh     = dr::normalize(si.wi + wo);
        Mask valid_spec = (dr::dot(si.wi, wh) > 0.f) & (dr::dot(wo, wh) > 0.f);

        // ---------- precompute per-j constants for spec pdf ----------
        Matrix3f M_T[NbGrainMax];
        Float abs_det_M[NbGrainMax];
        Vector3f wh_1_tmp[NbGrainMax];
        Float norm_sqr[NbGrainMax];
        Float hv_height[NbGrainMax];

        for (size_t j = 0; j < NbGrainMax; ++j) {
            M_T[j]       = Matrix3f(0.f);
            abs_det_M[j] = 0.f;
            wh_1_tmp[j]  = Vector3f(0.f);
            norm_sqr[j]  = 0.f;
            hv_height[j] = 0.f;
        }

        for (size_t j = 0; j < m_bsdf_count; ++j) {
            Matrix3f M(a[j], b[j], 0.f, c[j], d[j], 0.f, 0.f, 0.f, 1.f);

            abs_det_M[j] = dr::abs(a[j] * d[j] - b[j] * c[j]);
            M_T[j]       = dr::transpose(M);

            Vector3f tmp = M_T[j] * wh;
            wh_1_tmp[j]  = tmp;
            norm_sqr[j]  = dr::squared_norm(tmp);

            Vector3f wh_1 = dr::normalize(tmp);
            hv_height[j]  = Frame3f::cos_theta(wh_1) * r[j];
        }

        // ---------- compute pdf (same math, less repetition) ----------
        Float pdf_ = 0.f;

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {

            h_upper          = r[i];
            Float p_level    = this->proba_level(global_tau_0, term_kappa,
                                                 term_lambda, h_upper, h_lower);
            Mask valid_level = dr::neq(h_upper - h_lower, 0.f);

            for (size_t j = i; j < m_bsdf_count; ++j) {

                Float p_type =
                    this->proba_level_type(tau_0[j], r[j], term_lambda);

                Mask inRange =
                    (hv_height[j] > h_lower) & (hv_height[j] <= h_upper);

                // spec pdf (exact same call, just use cached abs_det_M[j],
                // norm_sqr[j])
                Float pdf_spec =
                    this->square_to_micrograin_conditional_level_and_type_pdf(
                        term_lambda, r[j], h_upper, h_lower, abs_det_M[j],
                        norm_sqr[j], wh) /
                    (4.f * dr::dot(wo, wh));

                pdf_ += dr::select(
                    active & valid_level & inRange & valid_spec,
                    p_level * p_type * spec_sampling_proba[j] * pdf_spec, 0.f);

                // diff pdf (same)
                Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);
                pdf_ +=
                    dr::select(active & valid_level,
                               p_level * p_type *
                                   (1.f - spec_sampling_proba[j]) * pdf_diff,
                               0.f);
            }

            h_lower = h_upper;

            Float cur_term_kappa = 1.f - tau_0[i];
            Float cur_term_lambda =
                dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));

            term_kappa /= cur_term_kappa;
            term_lambda /= cur_term_lambda;
        }

        return pdf_;
    }


    //Spectrum eval_ex(const BSDFContext &ctx, 
    //                 const SurfaceInteraction3f &si,
    //                 const Vector3f &wo, 
    //                 const Point2f &sample2_extra,
    //                 Mask active) const override {
    //    Float cos_theta_i  = Frame3f::cos_theta(si.wi);
    //    Float cos_theta_o  = Frame3f::cos_theta(wo);
    //    Float global_tau_0 = this->eval_global_tau_0(si, active);

    //    Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
    //    active &= valid_general;
    //    //----------specular variables----------//
    //    // half vector
    //    Vector3f h        = dr::normalize(si.wi + wo);
    //    Float cos_theta_h = Frame3f::cos_theta(h);
    //    // G local(h)
    //    Mask G_local_h = (dr::dot(si.wi, h) > 0.f) & (dr::dot(wo, h) > 0.f);

    //    Spectrum value(0.f);
    //    for (size_t i = 0; i < m_bsdf_count; ++i) {

    //        Float tau_0 = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
    //        Float r     = m_micrograin_bsdfs[i]->eval_radius(si, active);
    //        auto [M, abs_det_M] =
    //            m_micrograin_bsdfs[i]->eval_stretching_matrix3f_and_abs_det(
    //                si, active);
    //        Matrix3f M_T = dr::transpose(M);
    //        Matrix3f M_inv   = dr::inverse(M);
    //        Matrix3f M_inv_T = dr::transpose(M_inv);
    //        //-------- eval specular ---------//
    //        Vector3f h_tmp      = M_T * h;
    //        Vector3f h_1 = dr::normalize(h_tmp);
    //        Float norm_sqr   = dr::squared_norm(h_tmp);
    //        Float cos_theta_h_1 = Frame3f::cos_theta(h_1);
    //        // joint NDF(h)
    //        Float coeff      = abs_det_M / (norm_sqr * norm_sqr);
    //        Float expo_numer = r * r * cos_theta_h_1 * cos_theta_h_1;
    //        Float shared_prod =
    //            this->shared_product(si, expo_numer, active & G_local_h);
    //        Float D_type_normal_joint = coeff * dr::log(1.f - tau_0) *
    //                                    shared_prod * -dr::InvPi<Float> /
    //                                    global_tau_0;
    //        // type dist GAF(h)
    //        Float height = cos_theta_h_1 * r;
    //        Float G_dist = this->shared_g_dist(si, wo, height, active & G_local_h);
    //        // fresnel(h)
    //        Spectrum F = m_micrograin_bsdfs[i]->eval_fresnel(ctx, si, h, active & G_local_h);
    //        // final brdf
    //        Spectrum brdf_spec = D_type_normal_joint * F * G_dist /
    //                            (4.f * cos_theta_i /*cos_theta_o*/);
    //        value += dr::select(active & G_local_h, brdf_spec, 0.f);

    //        //-------- eval diffuse ---------//
    //        // sample micronormal(m)
    //        Vector3f m_1 =
    //            square_to_sphere_micrograin<Float>(tau_0, sample2_extra);
    //        Vector3f m = dr::normalize(M_inv_T * m_1);
    //        Float cos_theta_m_1 = Frame3f::cos_theta(m_1);
    //        // G local(m)
    //        Mask G_local_m = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
    //        // sample pdf(m)
    //        norm_sqr = dr::squared_norm(M_T * m);
    //        coeff    = abs_det_M / (norm_sqr * norm_sqr);
    //        Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
    //        Float pdf_m    = D_ * Frame3f::cos_theta(m);
    //        Mask valid_normal_sample = dr::neq(pdf_m, 0.f);
    //        // joint NDF(m)
    //        expo_numer = r * r * cos_theta_m_1 * cos_theta_m_1;
    //        shared_prod = this->shared_product(si, expo_numer, active & G_local_m);
    //        D_type_normal_joint = coeff * dr::log(1.f - tau_0) *
    //                              shared_prod * -dr::InvPi<Float> /
    //                              global_tau_0;
    //        // type dist GAF(m)
    //        height = Frame3f::cos_theta(m_1) * r;
    //        G_dist = this->shared_g_dist(si, wo, height, active & G_local_m);
    //        // reflectance
    //        Spectrum reflect = m_micrograin_bsdfs[i]->eval_weighted_albedo(
    //            si, wo, m, active & G_local_m & valid_normal_sample);
    //        // final brdf
    //        Spectrum brdf_diffuse = reflect * D_type_normal_joint * G_dist /
    //                                pdf_m / (cos_theta_i /*cos_theta_o*/);

    //        value += dr::select(active & G_local_m & valid_normal_sample,
    //                            brdf_diffuse, 0.f);
    //        
    //    }
    //    /*Mask accident_NaN = dr::any(dr::isnan(value));
    //    return dr::select(accident_NaN, 0.f, value);*/
    //    return value;
    //}


    Spectrum eval_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                     const Vector3f &wo, const Point2f &sample2_extra,
                     Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        active &= valid_general;

        // half vector
        Vector3f h     = dr::normalize(si.wi + wo);
        Mask G_local_h = (dr::dot(si.wi, h) > 0.f) & (dr::dot(wo, h) > 0.f);

        // ---------------- cache per-type (k) ----------------
        Float tau0[NbGrainMax], rr[NbGrainMax], abs_det[NbGrainMax];
        Matrix3f M[NbGrainMax], M_T[NbGrainMax], M_inv[NbGrainMax],
            M_inv_T[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            tau0[k] = rr[k] = abs_det[k] = 0.f;
            M[k] = M_T[k] = M_inv[k] = M_inv_T[k] = Matrix3f(0.f);
        }

        for (size_t k = 0; k < m_bsdf_count; ++k) {
            tau0[k] = m_micrograin_bsdfs[k]->eval_tau_0(si, active);
            rr[k]   = m_micrograin_bsdfs[k]->eval_radius(si, active);

            auto [Mk, detk] =
                m_micrograin_bsdfs[k]->eval_stretching_matrix3f_and_abs_det(
                    si, active);

            M[k]       = Mk;
            abs_det[k] = detk;

            M_T[k]     = dr::transpose(Mk);
            M_inv[k]   = dr::inverse(Mk);
            M_inv_T[k] = dr::transpose(M_inv[k]);
        }

        // ---------------- cache wi_1 / wo_1 for g_dist ----------------
        Vector3f wi1[NbGrainMax], wo1[NbGrainMax];
        for (size_t k = 0; k < NbGrainMax; ++k) {
            wi1[k] = Vector3f(0.f);
            wo1[k] = Vector3f(0.f);
        }
        for (size_t k = 0; k < m_bsdf_count; ++k) {
            wi1[k] = dr::normalize(M_inv[k] * si.wi);
            wo1[k] = dr::normalize(M_inv[k] * wo);
        }

        // ---------------- local shared_product (math-identical)
        // ----------------
        auto shared_product_cached = [&](const Float expo_numer,
                                         Mask act) -> Float {
            Float value = 1.f;
            for (size_t k = 0; k < m_bsdf_count; ++k) {
                Float expo_coeff = expo_numer / (rr[k] * rr[k]);
                Float exponent   = dr::maximum(1.f - expo_coeff, 0.f);
                Float base       = 1.f - tau0[k];
                Float value_tmp  = dr::pow(base, exponent);
                value *= dr::select(act, value_tmp, 1.f);
            }
            return value;
        };

        // ---------------- local g_dist (math-identical to shared_g_dist)
        // ----------------
        auto g_dist_from_height = [&](const Float height, Mask act) -> Float {
            Float value = 1.f;
            for (size_t k = 0; k < m_bsdf_count; ++k) {
                Float h_m_1   = height / rr[k];
                Float g2_dist = G2_HD<Float>(tau0[k], wi1[k], wo1[k], h_m_1);
                value *= dr::select(act, g2_dist, 1.f);
            }
            return value;
        };

        // ---------------- main eval ----------------
        Spectrum value(0.f);

        for (size_t i = 0; i < m_bsdf_count; ++i) {

            // ======== specular (same math) ========
            Vector3f h_tmp      = M_T[i] * h;
            Vector3f h_1        = dr::normalize(h_tmp);
            Float norm_sqr      = dr::squared_norm(h_tmp);
            Float cos_theta_h_1 = Frame3f::cos_theta(h_1);

            Float coeff      = abs_det[i] / (norm_sqr * norm_sqr);
            Float expo_numer = rr[i] * rr[i] * cos_theta_h_1 * cos_theta_h_1;

            Float shared_prod =
                shared_product_cached(expo_numer, active & G_local_h);

            Float D_type_normal_joint = coeff * dr::log(1.f - tau0[i]) *
                                        shared_prod * -dr::InvPi<Float> /
                                        global_tau_0;

            Float height = cos_theta_h_1 * rr[i];
            Float G_dist = g_dist_from_height(height, active & G_local_h);

            Spectrum F = m_micrograin_bsdfs[i]->eval_fresnel(
                ctx, si, h, active & G_local_h);

            Spectrum brdf_spec =
                D_type_normal_joint * F * G_dist / (4.f * cos_theta_i);

            value += dr::select(active & G_local_h, brdf_spec, 0.f);

            // ======== diffuse (same math) ========
            Vector3f m_1 =
                square_to_sphere_micrograin<Float>(tau0[i], sample2_extra);
            Vector3f m = dr::normalize(M_inv_T[i] * m_1);

            Float cos_theta_m_1 = Frame3f::cos_theta(m_1);

            Mask G_local_m = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);

            // sample pdf(m)
            Float norm_sqr_m         = dr::squared_norm(M_T[i] * m);
            Float coeff_m            = abs_det[i] / (norm_sqr_m * norm_sqr_m);
            Float D_                 = coeff_m * NDF_1<Float>(tau0[i], m_1);
            Float pdf_m              = D_ * Frame3f::cos_theta(m);
            Mask valid_normal_sample = dr::neq(pdf_m, 0.f);

            // joint NDF(m)
            Float expo_numer_m = rr[i] * rr[i] * cos_theta_m_1 * cos_theta_m_1;

            Float shared_prod_m =
                shared_product_cached(expo_numer_m, active & G_local_m);

            Float D_type_normal_joint_m = coeff_m * dr::log(1.f - tau0[i]) *
                                          shared_prod_m * -dr::InvPi<Float> /
                                          global_tau_0;

            // type dist GAF(m)
            Float height_m = Frame3f::cos_theta(m_1) * rr[i];
            Float G_dist_m = g_dist_from_height(height_m, active & G_local_m);

            // reflectance
            Spectrum reflect = m_micrograin_bsdfs[i]->eval_weighted_albedo(
                si, wo, m, active & G_local_m & valid_normal_sample);

            Spectrum brdf_diffuse = reflect * D_type_normal_joint_m * G_dist_m /
                                    pdf_m / (cos_theta_i);

            value += dr::select(active & G_local_m & valid_normal_sample,
                                brdf_diffuse, 0.f);
        }

        return value;
    }





    //**********************************************************//
    //*-------------- helper functions (poly) -----------------*//
    //**********************************************************//

    //---------------global variable compute---------------//

    MI_INLINE Float shared_product(const SurfaceInteraction3f &si, 
                                   const Float expo_numer, 
                                   Mask active = true) const {
        Float value = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i){
            Float tau_0      = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            Float r          = m_micrograin_bsdfs[i]->eval_radius(si, active);
            Float expo_coeff = expo_numer / (r*r);
            Float value_tmp = dr::pow((1.f - tau_0), dr::maximum(1.f - expo_coeff, 0.f));
            value *= dr::select(active, value_tmp, 1.f);
        }
        return value;
    }

    MI_INLINE Float shared_g_dist(const SurfaceInteraction3f &si, 
                                  const Vector3f &wo, 
                                  const Float height, 
                                  Mask active = true) const {
        Float value = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i){
            Matrix3f M       = m_micrograin_bsdfs[i]->eval_stretching_matrix3f(si, active);
            Matrix3f M_inv   = dr::inverse(M);
            Vector3f wi_1    = dr::normalize(M_inv * si.wi);
            Vector3f wo_1    = dr::normalize(M_inv * wo);
            Float tau_0      = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            Float r          = m_micrograin_bsdfs[i]->eval_radius(si, active);
            Float h_m_1      = height / r;
            Float g2_dist    = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
            value *= dr::select(active, g2_dist, 1.f);
        }
        return value;
    } 

    MI_INLINE Float term_lambda_base(const SurfaceInteraction3f &si, 
                                     Mask active = true) const {
        Float value = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float tau_0 = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            Float r     = m_micrograin_bsdfs[i]->eval_radius(si, active);
            Float value_tmp = dr::pow(1.f - tau_0, -1.f / (r * r));
            value *= dr::select(active, value_tmp, 1.f);
        }
        return value;
    }

    MI_INLINE Float proba_level(const Float global_tau_0, 
                                const Float term_kappa,
                                const Float term_lambda, 
                                const Float h_upper, const Float h_lower) const {
        Float h_upper_sqr = h_upper * h_upper;
        Float h_lower_sqr = h_lower * h_lower;
        Float value       = term_kappa *
                            (dr::pow(term_lambda, h_upper_sqr) -
                             dr::pow(term_lambda, h_lower_sqr)) /
                             global_tau_0;
        return dr::clamp(value, 0.f, 1.f);
    }

    MI_INLINE Float proba_level_type(const Float tau_0, 
                                     const Float r,
                                     const Float term_lambda) const {
        Float value = dr::log(dr::pow(1.f - tau_0, -1.f/(r*r))) / dr::log(term_lambda);
        return dr::clamp(value, 0.f, 1.f);
    }

    MI_INLINE Vector3f square_to_sphere_micrograin_conditional_level_and_type(const Float term_lambda,
                                                                              const Float r, 
                                                                              const Float h_upper, const Float h_lower,
                                                                              const Point2f &sample2) const {
        Float s1 = sample2.x();
        Float s2 = sample2.y();

        Float phi_m = dr::TwoPi<Float> * s1;
        Float theta_m = dr::acos(dr::sqrt(
                            dr::log((1.f - s2) * dr::pow(term_lambda, h_upper * h_upper) + 
                            s2 * dr::pow(term_lambda, h_lower * h_lower)) /
                            (r * r) / dr::log(term_lambda)
                        ));
        return Vector3f(
            dr::sin(theta_m) * dr::cos(phi_m),
            dr::sin(theta_m) * dr::sin(phi_m),
            dr::cos(theta_m)
        );
    }

    MI_INLINE Float square_to_micrograin_conditional_level_and_type_pdf(const Float term_lambda,
                                                                        const Float r, 
                                                                        const Float h_upper, const Float h_lower, 
                                                                        const Float abs_det_M, const Float norm_sqr, 
                                                                        const Vector3f &m) const {
        Float cos_theta_m = Frame3f::cos_theta(m);
        Float coeff       = abs_det_M / (norm_sqr * norm_sqr);
        Float r_sqr       = r * r;
        Float numer = r_sqr * dr::log(term_lambda) *
                      dr::pow(term_lambda, r_sqr * cos_theta_m * cos_theta_m / norm_sqr);
        Float denom = (dr::pow(term_lambda, h_upper * h_upper) - 
                       dr::pow(term_lambda, h_lower * h_lower));
        return dr::maximum(coeff * dr::InvPi<Float> * cos_theta_m * numer / denom, 0.f);
    }
    
    //-------------sorting functions-------------------//

    // swap and b if mask is true
    template <typename T, typename Mask>
    MI_INLINE void swap_if(T &a, T &b, const Mask &mask) const {
        T a_old = a, b_old = b;
        a = dr::select(mask, b_old, a_old);
        b = dr::select(mask, a_old, b_old);
    }
    
    // sort16 network for bsdf components based on radius
    template <typename T, typename... Ts>
    MI_INLINE void sort16_bsdf_by_radius(T (&r)[NbGrainMax], 
                                         size_t count, 
                                         Mask active, 
                                         Ts (&... arrays)[NbGrainMax]) const {
        auto pair_valid = [&](size_t i, size_t j) -> Mask {
            return active & (i < count) & (j < count);
        };

        auto CS = [&](size_t i, size_t j) {
            Mask valid = pair_valid(i, j);
            Mask mask  = (r[i] > r[j]) & valid; // ascending order
            swap_if(r[i], r[j], mask);
            (swap_if(arrays[i], arrays[j], mask), ...);
        };
        // ---- Bitonic sort for 16 ----
        // stage size 2
        CS(0, 1); CS(2, 3); CS(4, 5);  CS(6, 7);  CS(8, 9);   CS(10, 11); CS(12, 13); CS(14, 15);
        // stage size 4
        CS(0, 2); CS(1, 3); CS(4, 6);  CS(5, 7);  CS(8, 10);  CS(9, 11);  CS(12, 14); CS(13, 15);
        CS(1, 2); CS(5, 6); CS(9, 10); CS(13, 14);
        // stage size 8
        CS(0, 4); CS(1, 5); CS(2, 6);  CS(3, 7);  CS(8, 12);  CS(9, 13);  CS(10, 14); CS(11, 15);
        CS(0, 2); CS(1, 3); CS(4, 6);  CS(5, 7);  CS(8, 10);  CS(9, 11);  CS(12, 14); CS(13, 15);
        CS(1, 2); CS(3, 4); CS(5, 6);  CS(9, 10); CS(11, 12); CS(13, 14);
        // stage size 16 (final merge)
        CS(0, 8); CS(1, 9); CS(2, 10); CS(3, 11); CS(4, 12);  CS(5, 13);  CS(6, 14);  CS(7, 15);
        CS(0, 4); CS(1, 5); CS(2, 6);  CS(3, 7);  CS(8, 12);  CS(9, 13);  CS(10, 14); CS(11, 15);
        CS(0, 2); CS(1, 3); CS(4, 6);  CS(5, 7);  CS(8, 10);  CS(9, 11);  CS(12, 14); CS(13, 15);
        CS(1, 2); CS(3, 4); CS(5, 6);  CS(7, 8);  CS(9, 10);  CS(11, 12); CS(13, 14);
    }

    

    


    MI_DECLARE_CLASS(PolyMicrograinBSDF)
private:
    
    bool m_has_diffuse_component;

    MI_TRAVERSE_CB(Base, m_has_diffuse_component)
};


MI_EXPORT_PLUGIN(PolyMicrograinBSDF)
NAMESPACE_END(mitsuba)