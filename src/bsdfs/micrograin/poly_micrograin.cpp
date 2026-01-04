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

    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              Float sample1, 
              const Point2f &sample2, 
              const Point2f &sample2_extra,
              Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;
        //----------ascending sorting----------//
        Float a[NbGrainMax], b[NbGrainMax], c[NbGrainMax], d[NbGrainMax],
            r[NbGrainMax], tau_0[NbGrainMax], spec_sampling_proba[NbGrainMax];
        for (size_t i = 0; i < NbGrainMax; ++i) {
            a[i]     = 0.f;
            b[i]     = 0.f;
            c[i]     = 0.f;
            d[i]     = 0.f;
            r[i]     = 0.f; 
            tau_0[i] = 0.f;
            spec_sampling_proba[i] = 0.f;
        }
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            a[i]     = m_micrograin_bsdfs[i]->eval_a(si, active);
            b[i]     = m_micrograin_bsdfs[i]->eval_b(si, active);
            c[i]     = m_micrograin_bsdfs[i]->eval_c(si, active);
            d[i]     = m_micrograin_bsdfs[i]->eval_d(si, active);
            r[i]     = m_micrograin_bsdfs[i]->eval_radius(si, active);
            tau_0[i] = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            spec_sampling_proba[i] =
                m_micrograin_bsdfs[i]->specular_component_sampling_probability(
                    cos_theta_i);
        }
        this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
                                    spec_sampling_proba);

        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Float term_kappa  = 1.f - global_tau_0;
        Float term_lambda = this->term_lambda_base(si, active);

        Float h_lower = 0.f;
        Float h_upper = 0.f;
    
        
        BSDFSample3f bs   = dr::zeros<BSDFSample3f>();
        
        Spectrum result(0.f);
        Float p_level_cum = 0.f;

        // start sampling
        for (size_t i = 0; i < m_bsdf_count; ++i) { // for layers (heightfield)
            h_upper = r[i];
            Float p_level = this->proba_level(global_tau_0, term_kappa,
                                              term_lambda, h_upper, h_lower);
            Float p_level_cum_pre = p_level_cum;
            p_level_cum += p_level;

            Mask level_selected = (sample1 >= p_level_cum_pre) & (sample1 < p_level_cum);

            Float sample1_1 = (sample1 - p_level_cum_pre) / p_level;

            Float p_type_cum = 0.f;

            for (size_t j = i; j < m_bsdf_count; ++j) { // for micrograin types in layer i
                Float p_type = this->proba_level_type(tau_0[j], r[j], term_lambda);
                Float p_type_cum_pre = p_type_cum;
                p_type_cum += p_type;

                Mask type_selected = level_selected &
                                     (sample1_1 >= p_type_cum_pre) &
                                     (sample1_1 < p_type_cum);

                Float sample1_2 = (sample1_1 - p_type_cum_pre) / p_type;
                Mask spec_selected = level_selected & type_selected &
                                     (sample1_2 < spec_sampling_proba[j]);

                Matrix3f M(a[j], b[j], 0.f, c[j], d[j], 0.f, 0.f, 0.f, 1.f);
                Float abs_det_M = dr::abs(a[j] * d[j] - b[j] * c[j]);
                Matrix3f M_inv  = dr::inverse(M);
                Matrix3f M_inv_T = dr::transpose(M_inv);

                Vector3f wh_1 = square_to_sphere_micrograin_conditional_level_and_type(
                        term_lambda, r[j], h_upper, h_lower, sample2
                );

                Normal3f wh = dr::normalize(M_inv_T * wh_1);
                Vector3f wo = dr::select(
                    spec_selected,
                    reflect(si.wi, wh),
                    warp::square_to_cosine_hemisphere(sample2)
                );
                Float cos_theta_o = Frame3f::cos_theta(wo);
                active &= cos_theta_o > 0.f;
                Float pdf_ = this->pdf(ctx, si, wo, active);
                BSDFSample3f bs_tmp(wo);
                bs_tmp.pdf = pdf_;
                bs_tmp.sampled_type = dr::select(
                    spec_selected,
                    +BSDFFlags::GlossyReflection,
                    +BSDFFlags::DiffuseReflection);
                Mask pdf_valid = dr::neq(pdf_, 0.f);
                dr::masked(bs, level_selected && type_selected) = bs_tmp;
                dr::masked(result, level_selected && type_selected) = dr::select(
                    active & pdf_valid,
                    this->eval_ex(ctx, si, wo, sample2_extra, active) / pdf_, 
                    0.f
                );
            }
            h_lower = h_upper;
            Float cur_term_kappa = 1.f - tau_0[i];
            Float cur_term_lambda =
                dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));

            term_kappa /= cur_term_kappa;
            term_lambda /= cur_term_lambda;
        }
        Mask accident_NaN = dr::any(dr::isnan(result));
        return { bs, result & ~accident_NaN };
    }


    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);
        Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        active &= valid_general;

        //----------ascending sorting----------//
        Float a[NbGrainMax], 
              b[NbGrainMax], 
              c[NbGrainMax], 
              d[NbGrainMax],
              r[NbGrainMax], 
              tau_0[NbGrainMax],
              spec_sampling_proba[NbGrainMax];
        for (size_t i = 0; i < NbGrainMax; ++i) {
            a[i]                   = 0.f;
            b[i]                   = 0.f;
            c[i]                   = 0.f;
            d[i]                   = 0.f;
            r[i]                   = 0.f;
            tau_0[i]               = 0.f;
            spec_sampling_proba[i] = 0.f;
        }
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            a[i]     = m_micrograin_bsdfs[i]->eval_a(si, active);
            b[i]     = m_micrograin_bsdfs[i]->eval_b(si, active);
            c[i]     = m_micrograin_bsdfs[i]->eval_c(si, active);
            d[i]     = m_micrograin_bsdfs[i]->eval_d(si, active);
            r[i]     = m_micrograin_bsdfs[i]->eval_radius(si, active);
            tau_0[i] = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            spec_sampling_proba[i] = m_micrograin_bsdfs[i]->specular_component_sampling_probability(cos_theta_i);
        }
        this->sort16_bsdf_by_radius(r, m_bsdf_count, active, a, b, c, d, tau_0,
                                    spec_sampling_proba);

        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Float term_kappa = 1.f - global_tau_0;
        Float term_lambda = this->term_lambda_base(si, active);

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        //------------ specular variables -----------//
        Vector3f wh = dr::normalize(si.wi + wo);
        Float cos_theta_wh = Frame3f::cos_theta(wh);

        Mask valid_spec = (dr::dot(si.wi, wh) > 0.f) & (dr::dot(wo, wh) > 0.f);

        //---- compute pdf ----//
        Float pdf_ = 0.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) { // for layers (heightfield)

            h_upper = r[i];
            Float p_level = this->proba_level(global_tau_0, term_kappa, term_lambda, h_upper, h_lower);
            Mask valid_level = dr::neq(h_upper - h_lower, 0.f);

            for (size_t j = i; j < m_bsdf_count; ++j) { // for micrograin types in layer i

                Float p_type = this->proba_level_type(tau_0[j], r[j], term_lambda);

                Matrix3f M(a[j], b[j], 0.f, c[j], d[j], 0.f, 0.f, 0.f, 1.f);
                Float abs_det_M = dr::abs(a[j] * d[j] - b[j] * c[j]);

                Matrix3f M_T = dr::transpose(M);
                
                //------ spec pdf -------//
                Vector3f wh_1_tmp = M_T * wh;
                Vector3f wh_1     = dr::normalize(wh_1_tmp);
                Float norm_sqr    = dr::squared_norm(wh_1_tmp);
                Float hv_height = Frame3f::cos_theta(wh_1);
                Mask inRange    = (hv_height > h_lower) & (hv_height <= h_upper);
                Float pdf_spec = this->square_to_micrograin_conditional_level_and_type_pdf(
                    term_lambda, r[j],
                    h_upper, h_lower,
                    abs_det_M, norm_sqr, wh
                ) / (4.f * dr::dot(wo, wh));
                pdf_ += dr::select(active & valid_level & inRange & valid_spec,
                                   p_level * p_type * spec_sampling_proba[j] * pdf_spec, 0.f);
                //------ diff pdf -------//
                Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);
                
                pdf_ += dr::select(active & valid_level,
                                   p_level * p_type * (1.f - spec_sampling_proba[j]) * pdf_diff, 0.f);
            }
            h_lower = h_upper;
            Float cur_term_kappa = 1.f - tau_0[i];
            Float cur_term_lambda = dr::pow(1.f - tau_0[i], -1.f / (r[i] * r[i]));
            
            term_kappa /= cur_term_kappa;
            term_lambda /= cur_term_lambda;
        }
        Mask accident_NaN = dr::isnan(pdf_);
        return dr::select(accident_NaN, 0.f, pdf_);
    }


    Spectrum eval_ex(const BSDFContext &ctx, 
                     const SurfaceInteraction3f &si,
                     const Vector3f &wo, 
                     const Point2f &sample2_extra,
                     Mask active) const override {
        Float cos_theta_i  = Frame3f::cos_theta(si.wi);
        Float cos_theta_o  = Frame3f::cos_theta(wo);
        Float global_tau_0 = this->eval_global_tau_0(si, active);

        Mask valid_general = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        active &= valid_general;
        //----------specular variables----------//
        // half vector
        Vector3f h        = dr::normalize(si.wi + wo);
        Float cos_theta_h = Frame3f::cos_theta(h);
        // G local(h)
        Mask G_local_h = (dr::dot(si.wi, h) > 0.f) & (dr::dot(wo, h) > 0.f);

        Spectrum value(0.f);
        for (size_t i = 0; i < m_bsdf_count; ++i) {

            Float tau_0 = m_micrograin_bsdfs[i]->eval_tau_0(si, active);
            Float r     = m_micrograin_bsdfs[i]->eval_radius(si, active);
            auto [M, abs_det_M] =
                m_micrograin_bsdfs[i]->eval_stretching_matrix3f_and_abs_det(
                    si, active);
            Matrix3f M_T = dr::transpose(M);
            Matrix3f M_inv   = dr::inverse(M);
            Matrix3f M_inv_T = dr::transpose(M_inv);
            //-------- eval specular ---------//
            
            // joint NDF(h)
            Float norm_sqr   = dr::squared_norm(M_T * h);
            Float coeff      = abs_det_M / (norm_sqr * norm_sqr);
            Float expo_numer = r * r * cos_theta_h * cos_theta_h / norm_sqr;
            Float shared_prod =
                this->shared_product(si, expo_numer, active & G_local_h);
            Float D_type_normal_joint = coeff * dr::log(1.f - tau_0) *
                                        shared_prod * -dr::InvPi<Float> /
                                        global_tau_0;
            // type dist GAF(h)
            Vector3f h_1 = dr::normalize(M_T * h);
            Float height = Frame3f::cos_theta(h_1);
            Float G_dist = this->shared_g_dist(si, wo, height, active & G_local_h);
            // fresnel(h)
            Spectrum F = m_micrograin_bsdfs[i]->eval_fresnel(ctx, si, h, active & G_local_h);
            // final brdf
            Spectrum brdf_spec = D_type_normal_joint * F * G_dist /
                                (4.f * cos_theta_i /*cos_theta_o*/);
            value += dr::select(active & G_local_h, brdf_spec, 0.f);

            //-------- eval diffuse ---------//
            // sample micronormal(m)
            Vector3f m_1 =
                square_to_sphere_micrograin<Float>(tau_0, sample2_extra);
            Vector3f m = dr::normalize(M_inv_T * m_1);
            // G local(m)
            Mask G_local_m = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
            // sample pdf(m)
            norm_sqr = dr::squared_norm(M_T * m);
            coeff    = abs_det_M / (norm_sqr * norm_sqr);
            Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
            Float pdf_m    = D_ * Frame3f::cos_theta(m);
            Mask valid_normal_sample = dr::neq(pdf_m, 0.f);
            // joint NDF(m)
            Float cos_theta_m = Frame3f::cos_theta(m);
            expo_numer = r * r * cos_theta_m * cos_theta_m / norm_sqr;
            shared_prod = this->shared_product(si, expo_numer, active & G_local_m);
            D_type_normal_joint = coeff * dr::log(1.f - tau_0) *
                                  shared_prod * -dr::InvPi<Float> /
                                  global_tau_0;
            // type dist GAF(m)
            height = Frame3f::cos_theta(m_1);
            G_dist = this->shared_g_dist(si, wo, height, active & G_local_m);
            // reflectance
            Spectrum reflect = m_micrograin_bsdfs[i]->eval_weighted_albedo(
                si, wo, m, active & G_local_m & valid_normal_sample);
            // final brdf
            Spectrum brdf_diffuse = reflect * D_type_normal_joint * G_dist /
                                    pdf_m / (cos_theta_i /*cos_theta_o*/);

            value += dr::select(active & G_local_m & valid_normal_sample,
                                brdf_diffuse, 0.f);
            
        }
        Mask accident_NaN = dr::any(dr::isnan(value));
        return dr::select(accident_NaN, 0.f, value);
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