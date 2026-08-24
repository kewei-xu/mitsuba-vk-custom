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
class CoatedMicrograinBSDF final : public BSDF<Float, Spectrum> {

public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    CoatedMicrograinBSDF(const Properties &props) : Base(props) {
        m_coat_ior = lookup_ior(props, "coat_ior", "water");
        m_coat_thickness = props.get_texture<Texture>("coat_thickness", 1.0f);

        size_t bsdf_count = 0;

        for (auto &prop : props.objects()) {
            auto *bsdf = prop.try_get<BSDF<Float, Spectrum>>();
            if (!bsdf) 
                continue;
            if (bsdf_count == 0) {
                auto *grain_bsdf = dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf);
                if (!grain_bsdf)
                    Throw("CoatedMicrograinBSDF: The first BSDF must be a MicrograinBSDF.");
                m_micrograin_bsdf = grain_bsdf;
                bsdf_count++;
            } else if (bsdf_count == 1) {
                if (dynamic_cast<MicrograinBSDF<Float, Spectrum> *>(bsdf) ||  
                    dynamic_cast<PolyMicrograin<Float, Spectrum> *>(bsdf)) {
                    Throw("CoatedMicrograinBSDF: The second BSDF cannot be a MicrograinBSDF or PolyMicrograin.");
                }
                m_bulk_bsdf = bsdf;
                bsdf_count++;
            } else {
                Throw("CoatedMicrograinBSDF: Cannot specify more than two child BSDFs.");
            }
        }

        if (!m_micrograin_bsdf || !m_bulk_bsdf){
            Throw("CoatedMicrograinBSDF: Must specify exactly two child BSDFs: a MicrograinBSDF and a bulk BSDF.");
        }

        m_flags = m_micrograin_bsdf->flags() | m_bulk_bsdf->flags();
        m_components.clear();

        for (size_t i = 0; i < m_micrograin_bsdf->component_count(); ++i)
            m_components.push_back(m_micrograin_bsdf->flags(i));    
        for (size_t i = 0; i < m_bulk_bsdf->component_count(); ++i)
            m_components.push_back(m_bulk_bsdf->flags(i));    
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "CoatedMicrograinBSDF[" << std::endl
            << "  coat_ior = " << m_coat_ior << "," << std::endl
            << "  coat_thickness = " << m_coat_thickness->to_string() << "," << std::endl
            << "  micrograin_bsdf = " << m_micrograin_bsdf->to_string() << ","
            << std::endl
            << "  bulk_bsdf = " << m_bulk_bsdf->to_string() << "," << std::endl
            << "]";
        return oss.str();
    }

    MI_INLINE Float eval_coat_height(const SurfaceInteraction3f &si, 
                                     Mask active = true) const {
        return m_coat_thickness->eval_1(si, active);
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
        const bool has_micrograin_specular = has_flag(m_micrograin_bsdf->flags(), 
                                                      BSDFFlags::GlossyReflection);
        const bool has_micrograin_diffuse = has_flag(m_micrograin_bsdf->flags(), 
                                                     BSDFFlags::DiffuseReflection);

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Mask top_in = cos_theta_i > 0.f;
        Mask top_out = cos_theta_o > 0.f;
        
        Float tau_0 = m_micrograin_bsdf->eval_tau_0(si, active);
        Float l = this->eval_coat_height(si, active);
        auto [M, abs_det_M] = m_micrograin_bsdf->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);
        Matrix3f M_T     = dr::transpose(M);

        // two directions wi and wo in spherical micrograin space
        Vector3f wi_1 = dr::normalize(M_inv * si.wi);
        Vector3f wo_1 = dr::normalize(M_inv * wo);

        Float rho = -dr::log(1.f - tau_0) / (dr::Pi<Float> * abs_det_M);
        Float tau_l = 1.f - dr::exp(-rho * dr::Pi<Float> * (1.f - l*l) * abs_det_M);
        // coating part's visibility (weight) 
        Float Vp_l = (1.f - tau_l); // when below_in + below_out
        Vp_l = dr::select(top_in & top_out, 
                          Vp_l * G2_HD(tau_0, wi_1, wo_1, l), Vp_l); // when top_in + top_out
        Vp_l = dr::select(top_in & (!top_out), 
                          Vp_l * G1_HD(tau_0, wi_1, l), Vp_l); // when top_in + below_out
        Vp_l = dr::select((!top_in) & top_out, 
                          Vp_l * G1_HD(tau_0, wo_1, l), Vp_l); // when below_in + top_out
        
        //-----------------------------------------------------------------//
        // skip coating layer component here, since it is a direc function //
        //-----------------------------------------------------------------//

        //------------------------------------------------------------------//
        // emmerged part - above coating, only contain micrograin component //
        // (eval only under top_in + top_out) ------------------------------//
        //------------------------------------------------------------------//
    
        // Micrograin BSDF emmerged  
        // (Specular)
        Vector3f m = dr::normalize(si.wi + wo);
        // Mask valid_spec = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        // compute micrograin NDF 
        Vector3f m_1 = dr::normalize(M_T * m);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float D_       = abs_det_M / (norm_sqr * norm_sqr) * 
                         NDF_1<Float>(tau_0, m_1); 
        D_ *= (tau_0 / tau_l);
        // compute GAF
        //Vector3f wi_1  = dr::normalize(M_inv * si.wi);
        //Vector3f wo_1  = dr::normalize(M_inv * wo);
        Float h_m_1 = Frame3f::cos_theta(m_1);
        Mask G2_local  = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        Float G2_dist  = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        Float G_       = dr::select(G2_local, G2_dist, 0.f);
        // compute fresnel
        Spectrum F = m_micrograin_bsdf->eval_fresnel(ctx, si, wo, m, active);
        Mask above_valid_specular = h_m_1 > l;
        Spectrum value_above = D_ * G_ * F / (4.f * cos_theta_i /*cos_theta_o*/);
        value_above = dr::select(above_valid_specular, value_above, 0.f);
        // (Diffuse)
        // sample micrograin normal
        Vector3f m_1_samp = square_to_sphere_micrograin<Float>(tau_0, sample2_extra);
        Vector3f m_samp = dr::normalize(M_inv_T * m_1_samp); 
        // compute micrograin NDF and normal sample pdf
        norm_sqr = dr::squared_norm(M_T * m_samp);
        D_       = abs_det_M / (norm_sqr * norm_sqr) * NDF_1<Float>(tau_0, m_1_samp); 
        D_ *= (tau_0 / tau_l);
        Float pdf_m = D_ * Frame3f::cos_theta(m_samp);
        // compute GAF
        h_m_1 = Frame3f::cos_theta(m_1_samp);
        G2_local  = (dr::dot(si.wi, m_samp) > 0.f) & (dr::dot(wo, m_samp) > 0.f);
        G2_dist = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        G_ = dr::select(G2_local, G2_dist, 0.f);
        // compute diffuse coefficient
        Spectrum diff_coef = m_micrograin_bsdf->eval_weighted_albedo(si, wo, m_samp, active);
        Spectrum value_above_diff = diff_coef * D_ * G_ / pdf_m / (cos_theta_i /*cos_theta_o*/);
        // surface micro-normal must be a valid sample
        value_above_diff = dr::select(dr::neq(pdf_m, 0.f), value_above_diff, 0.f);
        // normal is indeed above coating layer
        Mask above_valid_diffuse = h_m_1 > l;
        value_above = dr::select(above_valid_diffuse, value_above + value_above_diff, value_above);
        // all type of micrograins are opaque, 
        // it's BRDF can only be evaluated when top_in + top_out 
        value_above = dr::select(top_in & top_out, value_above, 0.f);
        
        //------------------------------------------------------------------//
        // immersed part - below coating, contain micrograin + bulk --------//
        //------------------------------------------------------------------//
        
        // compute refracted directions (only valid when top_in + top_out)
        auto [r_i, cos_theta_t_in, eta_it_in, eta_ti_in] = fresnel(cos_theta_i, Float(m_coat_ior)); // assuming exterior ior is 1.0
        //Float t_i = 1.f - r_i;
        Vector3f wi_r = -refract(si.wi, cos_theta_t_in, eta_ti_in);
        auto [r_o, cos_theta_t_out, eta_it_out, eta_ti_out] = fresnel(cos_theta_o, Float(m_coat_ior)); // assuming exterior ior is 1.0
        Vector3f wo_r = -refract(wo, cos_theta_t_out, eta_ti_out);
        // something need to do here to avoid total internal reflection (TIR)
        Mask have_in_transmission = dr::neq(cos_theta_t_in, 0.f);
        Mask have_out_transmission = dr::neq(cos_theta_t_out, 0.f);

        Float cos_theta_i_r = Frame3f::cos_theta(wi_r);
        Float cos_theta_o_r = Frame3f::cos_theta(wo_r);
        // compute transmission weight through coating layer
        // eta_ti^2 is the Jacobian of the refracted directions
        Float T_c = 1.f; // when below_in + below_out
                         // since model didnt contain any intereflections between coating
                         // below_in + below_out basically means light path not entering coating
                         // reflected inside transmitted bulk material, so no loss on energy
        T_c = dr::select(top_in & top_out, 
                         (1.f - r_i) * (1.f - r_o) * eta_ti_in * eta_ti_in, T_c); // when top_in + top_out
                                                                                  // (in this case: eta_ti_in = eta_ti_out)
        T_c = dr::select(top_in & (!top_out), 
                         (1.f - r_i) * eta_ti_in * eta_ti_in, T_c); // when top_in + below_out
        T_c = dr::select((!top_in) & top_out, 
                         (1.f - r_o) * eta_ti_out * eta_ti_out, T_c); // when below_in + top_out
        
        // reflected directions transfered in spherical micrograin space
        Vector3f wi_r_1 = dr::normalize(M_inv * wi_r);
        Vector3f wo_r_1 = dr::normalize(M_inv * wo_r);
        // two weights for immersed part: one for micrograin BSDF, one for bulk BSDF
        Float tau_b_l = (tau_0 - tau_l) / (1.f - tau_l);
        // visiblity weight for bulk BSDF in coating
        Float Vp_b_l = (1.f - tau_b_l); // when below_in + below_out
        Vp_b_l = dr::select(top_in & top_out, 
                            Vp_b_l * G2_HD_0<Float>(tau_0, wi_r_1, wo_r_1) / dr::maximum(1e-8f, G2_HD<Float>(tau_0, wi_1, wo_1, l)), 
                            Vp_b_l); // when top_in + top_out
        Vp_b_l = dr::select(top_in & (!top_out), 
                            Vp_b_l * G1_HD_0<Float>(tau_0, wi_r_1) / dr::maximum(1e-8f, G1_HD<Float>(tau_0, wi_1, l)),
                            Vp_b_l); // when top_in + below_out
        Vp_b_l = dr::select((!top_in) & top_out, 
                            Vp_b_l * G1_HD_0<Float>(tau_0, wo_r_1) / dr::maximum(1e-8f, G1_HD<Float>(tau_0, wo_1, l)),
                            Vp_b_l); // when below_in + top_out 
        
        // micrograin BSDF immersed (eval only under top_in + top_out)
        SurfaceInteraction3f si_immersed(si); si_immersed.wi = wi_r;
        // (Specular)
        m = dr::normalize(wi_r + wo_r);
        m_1 = dr::normalize(M_T * m);
        norm_sqr = dr::squared_norm(M_T * m);
        D_       = abs_det_M / (norm_sqr * norm_sqr) * 
                   NDF_1<Float>(tau_0, m_1); 
        D_ /= (1.f - tau_l / tau_0);
        // compute GAF
        h_m_1 = Frame3f::cos_theta(m_1);
        G2_local  = (dr::dot(wi_r, m) > 0.f) & (dr::dot(wo_r, m) > 0.f);
        G2_dist  = G2_HD<Float>(tau_0, wi_r_1, wo_r_1, h_m_1) / 
                   dr::maximum(1e-8f, G2_HD<Float>(tau_0, wi_1, wo_1, l));
        G_       = dr::select(G2_local, G2_dist, 0.f);
        // compute fresnel
        F = m_micrograin_bsdf->eval_fresnel(ctx, si_immersed, wo_r, m, m_coat_ior, active); 
        Spectrum value_below = D_ * G_ * F / (4.f * cos_theta_i_r /*cos_theta_o_r */);
        // normal is indeed below coating layer
        Mask below_valid_specular = h_m_1 <= l;
        value_below = dr::select(below_valid_specular, tau_b_l * value_below, 0.f);
        // (Diffuse)
        // compute micrograin NDF and normal sample pdf
        norm_sqr = dr::squared_norm(M_T * m_samp);
        D_       = abs_det_M / (norm_sqr * norm_sqr) * NDF_1<Float>(tau_0, m_1_samp); 
        D_ /= (1.f - tau_l / tau_0);
        pdf_m = D_ * Frame3f::cos_theta(m_samp);
        // compute GAF
        h_m_1 = Frame3f::cos_theta(m_1_samp);
        G2_local  = (dr::dot(wi_r, m_samp) > 0.f) & (dr::dot(wo_r, m_samp) > 0.f);
        G2_dist = G2_HD<Float>(tau_0, wi_r_1, wo_r_1, h_m_1) / 
                  dr::maximum(1e-8f, G2_HD<Float>(tau_0, wi_1, wo_1, l));
        G_ = dr::select(G2_local, G2_dist, 0.f);
        // compute diffuse coefficient
        diff_coef = m_micrograin_bsdf->eval_weighted_albedo(si_immersed, wo_r, m_samp, m_coat_ior, active);
        Spectrum value_below_diff = diff_coef * D_ * G_ / pdf_m / (cos_theta_i_r /*cos_theta_o_r */);
        // surface micro-normal must be a valid sample
        value_below_diff = dr::select(dr::neq(pdf_m, 0.f), value_below_diff, 0.f);
        // normal is indeed below coating layer
        value_below = dr::select(above_valid_diffuse, value_below, value_below + tau_b_l * value_below_diff);
        // all type of micrograins are opaque, 
        // it's BRDF can only be evaluated when top_in + top_out 
        value_below = dr::select(top_in & top_out & have_in_transmission & have_out_transmission, 
                                 value_below, 0.f);
        
        // immersed bulk surface (need to account all 4 cases of top_in/out + below_in/out)

        // create a new surface interaction with refracted wi
        SurfaceInteraction3f si_bulk(si); // below_in
        si_bulk.wi = dr::select(top_in, wi_r, si_bulk.wi); // top_in
        Vector3f bulk_wo = wo; // below_out
        bulk_wo = dr::select(top_out, wo_r, bulk_wo); // top_out
        // if TIR happend in either direction, the bulk BSDF will return 0 
        // (only coating reflection and above part have contribution)
        Mask bulk_active =
            active &
            ((!top_in) | have_in_transmission) &
            ((!top_out) | have_out_transmission);

        value_below += dr::select(bulk_active, Vp_b_l * m_bulk_bsdf->eval(ctx, si_bulk, bulk_wo, active), 0.f);
        // Spectrum value_bulk = 0.f;
        // value_bulk = dr::select(top_in & top_out & have_in_transmission & have_out_transmission, 
        //                          Vp_b_l * m_bulk_bsdf->eval(ctx, si_bulk, wo_r, active), value_bulk);
        // value_bulk = dr::select(top_in & ~top_out & have_in_transmission, 
        //                          Vp_b_l * m_bulk_bsdf->eval(ctx, si_bulk, wo, active), value_bulk);
        // value_bulk = dr::select(~top_in & top_out & have_out_transmission, 
        //                          Vp_b_l * m_bulk_bsdf->eval(ctx, si, wo_r, active), value_bulk);
        // value_bulk = dr::select(~top_in & ~top_out, 
        //                          Vp_b_l * m_bulk_bsdf->eval(ctx, si, wo, active), value_bulk);
        // value_below += value_bulk;
        // value_below += dr::select(top_in & top_out & have_in_transmission & have_out_transmission, 
        //                           Vp_b_l * m_bulk_bsdf->eval(ctx, si_bulk, bulk_wo, active), 0.f);
        // value_below += dr::select(top_in & ~top_out & have_in_transmission, 
        //                           Vp_b_l * m_bulk_bsdf->eval(ctx, si_bulk, wo, active), 0.f);
        // value_below += dr::select(~top_in & top_out & have_out_transmission, 
        //                           Vp_b_l * m_bulk_bsdf->eval(ctx, si, wo_r, active), 0.f);
        // value_below += dr::select(~top_in & ~top_out, 
        //                           Vp_b_l * m_bulk_bsdf->eval(ctx, si, wo, active), 0.f);
        Spectrum value = tau_l * value_above + Vp_l * T_c * value_below;
        // if(dr::any_nested(dr::isnan(value))) {
        //     Log(Error, "NaN detected in CoatedMicrograinBSDF::eval_ex(): ");
        // }
        return dr::select(active, value, 0.f);
    }

    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Mask top_in = cos_theta_i > 0.f;
        Mask top_out = cos_theta_o > 0.f;
        
        Float tau_0 = m_micrograin_bsdf->eval_tau_0(si, active);
        Float l = eval_coat_height(si, active);
        auto [M, abs_det_M] = m_micrograin_bsdf->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);
        Matrix3f M_T     = dr::transpose(M);

        Float rho = -dr::log(1.f - tau_0) / (dr::Pi<Float> * abs_det_M);
        Float tau_l = 1.f - dr::exp(-rho * dr::Pi<Float> * (1.f - l*l) * abs_det_M);
        

        Float pdf_ = 0.f; // hierarchy path pdf 

        Vector3f wi_1  = dr::normalize(M_inv * si.wi);
        Float G1_dist = G1_HD<Float>(tau_0, wi_1, l);
        // auto [G1_dist, sig_s, has_shadow_i] = G1_HD_debug<Float>(tau_0, wi_1, l);
        // Float cos_theta_i_1 = Frame3f::cos_theta(wi_1);
        // Float sin_theta_i_1 = Frame3f::sin_theta(wi_1);
        // sample probability for emmerged micrograin (above coating layer)
        Float p_e = tau_l / (tau_l + (1.f - tau_l) * G1_dist);
        // sample probability for spec component
        Float p_es = m_micrograin_bsdf->specular_component_sampling_probability(cos_theta_i);
        // normal sampling pdf (spec)
        Vector3f m = dr::normalize(si.wi + wo);
        Vector3f m_1 = dr::normalize(M_T * m);
        Float h_m_1 = Frame3f::cos_theta(m_1);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float pdf_wo_spec = dr::select(h_m_1 > l, 1.f, 0.f) * 
                            abs_det_M / (norm_sqr * norm_sqr) * 
                            tau_0 / tau_l * NDF_1<Float>(tau_0, m_1) * 
                            Frame3f::cos_theta(m) / 
                            (4.f * dr::dot(m, wo));
        
        // normal sampling pdf (diff)
        Float pdf_wo_diff = warp::square_to_cosine_hemisphere_pdf(wo);
        
        pdf_ += dr::select(top_in & top_out, 
                           p_e * (p_es * pdf_wo_spec + (1.f - p_es) * pdf_wo_diff), 
                           0.f);

        // sample probability for coating part
        Float p_c = 1.f - p_e;
        // sample probability for reflection 
        //Float r_i = std::get<0>(fresnel(cos_theta_i, Float(m_coat_ior))); // assuming exterior ior is 1.0
        
        // compute refracted directions (eval only valid when top_in + top_out)
        auto [r_i, cos_theta_t_in, eta_it_in, eta_ti_in] = fresnel(cos_theta_i, Float(m_coat_ior)); // assuming exterior ior is 1.0
        Vector3f wi_r = -refract(si.wi, cos_theta_t_in, eta_ti_in);
        auto [r_o, cos_theta_t_out, eta_it_out, eta_ti_out] = fresnel(cos_theta_o, Float(m_coat_ior)); // assuming exterior ior is 1.0
        Vector3f wo_r = -refract(wo, cos_theta_t_out, eta_ti_out);
        
        // sample probability for transmission (immersed part)
        Float t_i = 1.f - r_i;

        Mask have_in_transmission = dr::neq(cos_theta_t_in, 0.f);
        Mask have_out_transmission = dr::neq(cos_theta_t_out, 0.f);
        
        Float cos_theta_i_r = dr::maximum(1e-8f, Frame3f::cos_theta(wi_r));
        Float cos_theta_o_r = dr::maximum(1e-8f, Frame3f::cos_theta(wo_r));
        // reflected directions transfered in spherical micrograin space
        Vector3f wi_r_1 = dr::normalize(M_inv * wi_r);
        Vector3f wo_r_1 = dr::normalize(M_inv * wo_r);
        // sample probability for immersed micrograin (below coating layer)
        Float tau_b_l = (tau_0 - tau_l) / (1.f - tau_l);
        Float V_pb = (1.f - tau_b_l) * G1_HD_0<Float>(tau_0, wi_r_1) / dr::maximum(1e-8f, G1_HD<Float>(tau_0, wi_1, l));
        Float p_bg = tau_b_l / (tau_b_l + V_pb);
        p_bg = dr::select(top_in & top_out & have_in_transmission & have_out_transmission, p_bg, 0.f);
        // sample probability for spec component
        Float p_bgs = m_micrograin_bsdf->specular_component_sampling_probability(cos_theta_i_r, Float(m_coat_ior));
        // normal sampling pdf (spec)
        m = dr::normalize(wi_r + wo_r);
        m_1 = dr::normalize(M_T * m);
        h_m_1 = Frame3f::cos_theta(m_1);
        norm_sqr = dr::squared_norm(M_T * m);
        pdf_wo_spec = dr::select(h_m_1 > l, 0.f, 1.f) * 
                      abs_det_M / (norm_sqr * norm_sqr) / 
                      (1.f - tau_l / tau_0) * NDF_1<Float>(tau_0, m_1) *
                      Frame3f::cos_theta(m) /
                      (4.f * dr::dot(m, wo_r));
        Float J_refract = ((eta_ti_out * eta_ti_out) * cos_theta_o / cos_theta_o_r); // Jacobian of the refracted directions
        pdf_wo_spec *= J_refract;
        // sample probability for diffuse component
        pdf_wo_diff = warp::square_to_cosine_hemisphere_pdf(wo_r) * J_refract;
        
        pdf_ += dr::select(top_in & top_out & have_in_transmission & have_out_transmission, 
                           p_c * t_i * (p_bg * (p_bgs * pdf_wo_spec + (1.f - p_bgs) * pdf_wo_diff)), 
                           0.f);

        // create new intersection direction for bulk BSDF (need eval under all 4 cases of top_in/out + below_in/out)
        SurfaceInteraction3f si_bulk(si); // below_in
        si_bulk.wi = dr::select(top_in, wi_r, si_bulk.wi); // top_in
        Vector3f bulk_wo = wo; // below_out
        bulk_wo = dr::select(top_out, wo_r, bulk_wo); // top_out
        Float pdf_wo_bulk = m_bulk_bsdf->pdf(ctx, si_bulk, bulk_wo, active);
        pdf_wo_bulk = dr::select(top_out, pdf_wo_bulk * J_refract, pdf_wo_bulk); // Jacobian of the refracted directions    
        pdf_ += p_c * t_i * (1.f - p_bg) * pdf_wo_bulk;

        // if(dr::any_nested(dr::isnan(pdf_))) {
        //     Log(Error, "NaN detected in CoatedMicrograinBSDF::pdf(): ");
        // }

        return pdf_;
    }

    std::pair<BSDFSample3f, Spectrum> 
    sample_ex(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si, 
              Float sample1, 
              const Point2f &sample2, 
              const Point2f &sample2_extra, 
              Mask active) const override {
        
        Float cos_theta_i = Frame3f::cos_theta(si.wi);

        Mask top_in = cos_theta_i > 0.f;

        Float tau_0 = m_micrograin_bsdf->eval_tau_0(si, active);
        Float l = eval_coat_height(si, active);
        auto [M, abs_det_M] = m_micrograin_bsdf->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);
        Matrix3f M_T     = dr::transpose(M);

        Float rho = -dr::log(1.f - tau_0) / (dr::Pi<Float> * abs_det_M);
        Float tau_l = 1.f - dr::exp(-rho * dr::Pi<Float> * (1.f - l*l) * abs_det_M);
        
        Vector3f wi_1  = dr::normalize(M_inv * si.wi);
        Float G1_dist = G1_HD<Float>(tau_0, wi_1, l);

        // compute refracted directions (only valid when top_in)
        auto [r_i, cos_theta_t, eta_it, eta_ti] = fresnel(cos_theta_i, Float(m_coat_ior)); // assuming exterior ior is 1.0
        Mask have_transmission = dr::neq(cos_theta_t, 0.f);
        Vector3f wi_r = -refract(si.wi, cos_theta_t, eta_ti);
        // auto [r_o, cos_theta_t, eta_it, eta_ti] = fresnel(cos_theta_o, Float(m_coat_ior)); // assuming exterior ior is 1.0
        // Vector3f wo_r = -refract(wo, cos_theta_t, eta_ti);
        Float cos_theta_i_r = Frame3f::cos_theta(wi_r);
        //Float cos_theta_o_r = Frame3f::cos_theta(wo_r);
        // reflected directions transfered in spherical micrograin space
        Vector3f wi_r_1 = dr::normalize(M_inv * wi_r);
        //Vector3f wo_r_1 = dr::normalize(M_inv * wo_r);
        
        // sample probability for emmerged micrograin (above coating layer)
        Float p_e = tau_l / (tau_l + (1.f - tau_l) * G1_dist);
        // sample probability for spec component
        Float p_es = m_micrograin_bsdf->specular_component_sampling_probability(cos_theta_i);
        // sample probability for coating reflection
        //Float r_i = std::get<0>(fresnel(cos_theta_i, Float(m_coat_ior))); // assuming exterior ior is 1.0
        // sample probability for immersed micrograin (below coating layer)
        Float tau_b_l = (tau_0 - tau_l) / (1.f - tau_l);
        Float V_pb = (1.f - tau_b_l) * G1_HD_0<Float>(tau_0, wi_r_1) / 
                                       dr::maximum(1e-8f, G1_HD<Float>(tau_0, wi_1, l));
        Float p_bg = tau_b_l / (tau_b_l + V_pb);
        p_bg = dr::select(top_in & have_transmission, p_bg, 0.f);
        // sample probability for immersed micrograin's spec component
        Float p_bgs = m_micrograin_bsdf->specular_component_sampling_probability(cos_theta_i_r, Float(m_coat_ior));
        
        // compute each path in the hierarchy and select one path to sample per lane
        
        Mask emmerged_selected = (sample1 < p_e);
        Float s1_emmerged = dr::select(emmerged_selected, 
                                       sample1 / p_e, 0.f);
        Float s1_coating = dr::select(emmerged_selected, 
                                      0.f, (sample1 - p_e) / (1.f - p_e));
        // emmerged micrograin path 
        Mask emmerged_spec_selected = (s1_emmerged < p_es) & emmerged_selected & top_in; 
        Mask emmerged_diff_selected = (!emmerged_spec_selected) & emmerged_selected & top_in;
        // emmerged micrograin's specular component path
        // (start -> emmerged_micrograin -> specular)
        Vector3f m_1 = square_to_sphere_micrograin_emmerged<Float>(tau_0, tau_l, sample2);
        Vector3f m = dr::normalize(M_inv_T * m_1);
        Vector3f wo_em_spec = reflect(si.wi, Normal3f(m));
        emmerged_spec_selected &= Frame3f::cos_theta(wo_em_spec) > 0.f; // ensure is a valid sample
        // emmerged micrograin's diffuse component path
        // (start -> emmerged_micrograin -> diffuse)
        Vector3f wo_em_diff = warp::square_to_cosine_hemisphere(sample2);
        emmerged_diff_selected &= Frame3f::cos_theta(wo_em_diff) > 0.f; // ensure is a valid sample

        // coating layer path
        Mask coating_reflection_selected = (s1_coating < r_i) & !emmerged_selected & top_in;
        Float s1_immersed = dr::select(coating_reflection_selected, 
                                       0.f, (s1_coating - r_i) / (1.f - r_i));
        // coating dirac reflection path
        // (start -> coating -> dirac reflection)
        Vector3f wo_coat_reflect = reflect(si.wi, Normal3f(0.f, 0.f, 1.f));
        coating_reflection_selected &= Frame3f::cos_theta(wo_coat_reflect) > 0.f; // ensure is a valid sample
                                                                                  // although it should always be true when top_in is true
        // eval dirac reflection
        Float Vp_l = (1.f - tau_l) * G2_HD(tau_0, si.wi, wo_coat_reflect, l);
        // simplfication here: cos_theta_i = cos_theta_o, since the coating layer is assumed to be flat
        //                     F(i) / F(i), since F(i) is the sample pdf of coating reflection path
        Spectrum value_coat_reflect = Vp_l / dr::maximum(1e-8f, (1.f - p_e));

        // immersed path
        Mask immersed_micrograin_selected = (s1_immersed < p_bg) & 
                                             !emmerged_selected &
                                             !coating_reflection_selected;
        Mask immersed_bulk_selected = (!immersed_micrograin_selected) & 
                                       !emmerged_selected &
                                       !coating_reflection_selected;
        Float s1_immersed_micrograin = dr::select(immersed_micrograin_selected, 
                                                  s1_immersed / p_bg, 0.f);
        Float s1_immersed_bulk = dr::select(immersed_micrograin_selected, 
                                            0.f, (s1_immersed - p_bg) / (1.f - p_bg));
        
        // immersed micrograin path
        Mask immersed_micrograin_spec_selected = (s1_immersed_micrograin < p_bgs) & 
                                                  !emmerged_selected &
                                                  !coating_reflection_selected &
                                                  immersed_micrograin_selected & top_in;
        Mask immersed_micrograin_diff_selected = (!immersed_micrograin_spec_selected) & 
                                                  !emmerged_selected &
                                                  !coating_reflection_selected &
                                                  immersed_micrograin_selected & top_in;
        // immersed micrograin's specular component path
        // (start -> coating -> immersed_part -> micrograin -> specular)
        Vector3f m_1_r = square_to_sphere_micrograin_immersed<Float>(tau_0, tau_l, sample2);
        Vector3f m_r = dr::normalize(M_inv_T * m_1_r);
        Vector3f wo_r_im_spec = reflect(wi_r, Normal3f(m_r));
        immersed_micrograin_spec_selected &= Frame3f::cos_theta(wo_r_im_spec) > 0.f; // ensure is a valid sample (top out)
        auto [F_exit_spec, cos_theta_ext_spec, 
              eta_it_spec, eta_ti_spec] = 
              fresnel(-Frame3f::cos_theta(wo_r_im_spec), Float(m_coat_ior)); // assuming exterior ior is 1.0
        Vector3f wo_im_spec = refract(-wo_r_im_spec, cos_theta_ext_spec, eta_ti_spec); 
        // since model didnt account intereflections in coating, 
        // need to exclude the exit path here when TIR
        immersed_micrograin_spec_selected &= dr::neq(cos_theta_ext_spec, 0.f);
        // ensure top out
        immersed_micrograin_spec_selected &= Frame3f::cos_theta(wo_im_spec) > 0.f;

        // immersed micrograin's diffuse component path
        // (start -> coating -> immersed_part -> micrograin -> diffuse)
        Vector3f wo_r_im_diff = warp::square_to_cosine_hemisphere(sample2);
        immersed_micrograin_diff_selected &= Frame3f::cos_theta(wo_r_im_diff) > 0.f; // ensure is a valid sample
                                                                                     // normally upper hemisphere cw sampling should always be valid here
        auto [F_exit_diff, cos_theta_ext_diff, 
              eta_it_diff, eta_ti_diff] = 
              fresnel(-Frame3f::cos_theta(wo_r_im_diff), Float(m_coat_ior)); // assuming exterior ior is 1.0
        Vector3f wo_im_diff = refract(-wo_r_im_diff, cos_theta_ext_diff, eta_ti_diff);
        // since model didnt account intereflections in coating, 
        // need to exclude the exit path here when TIR
        immersed_micrograin_diff_selected &= dr::neq(cos_theta_ext_diff, 0.f);
        //ensure top out
        immersed_micrograin_diff_selected &= Frame3f::cos_theta(wo_im_diff) > 0.f;

        // immersed bulk path
        // (start -> coating -> immersed_part -> bulk)
        // Mask immersed_bulk_selected_top_in = top_in & immersed_bulk_selected;
        // Mask immersed_bulk_selected_below_in = (~top_in) & immersed_bulk_selected;
        SurfaceInteraction3f si_bulk(si); // below in
        si_bulk.wi = dr::select(top_in, wi_r, si_bulk.wi); // top in
        Float s1_bulk = s1_immersed_bulk; // top in
        s1_bulk = dr::select((!top_in), sample1, s1_bulk); // below in
        auto [bs_bulk, _] = m_bulk_bsdf->sample_ex(ctx, si_bulk, 
                                                   s1_bulk, 
                                                   sample2, sample2_extra, active);
        //only valid when top_in
        auto [F_exit_bulk, cos_theta_ext_bulk, 
              eta_it_bulk, eta_ti_bulk] = 
              fresnel(-Frame3f::cos_theta(bs_bulk.wo), Float(m_coat_ior)); // assuming exterior ior is 1.0
        
        // NEED TO RECONSIDER: since model didnt account intereflections in coating!!!
        Mask bulk_active =
            ((!(Frame3f::cos_theta(bs_bulk.wo) > 0.f)) | dr::neq(cos_theta_ext_bulk, 0.f));
        immersed_bulk_selected &= (bulk_active & dr::neq(bs_bulk.pdf, 0.f));
        
        Vector3f wo_im_bulk = dr::select(Frame3f::cos_theta(bs_bulk.wo) > 0.f & dr::neq(cos_theta_ext_bulk, 0.f), // depending on if sampled direction is top_out or below_out 
                                         refract(-bs_bulk.wo, cos_theta_ext_bulk, eta_ti_bulk), 
                                         bs_bulk.wo);

        // select path and compute the corresponding BSDF value and pdf per lane
        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum value = 0.f;
        // emmerged micrograin's specular component is selected
        dr::masked(bs.wo, emmerged_spec_selected) = wo_em_spec;
        dr::masked(bs.sampled_type, emmerged_spec_selected) = +BSDFFlags::GlossyReflection;
        dr::masked(bs.pdf, emmerged_spec_selected) = this->pdf(ctx, si, wo_em_spec, active);
        dr::masked(value, emmerged_spec_selected & dr::neq(bs.pdf, 0.f)) = this->eval_ex(ctx, si, wo_em_spec, sample2_extra, active) / bs.pdf;
        // emmerged micrograin's diffuse component is selected
        dr::masked(bs.wo, emmerged_diff_selected) = wo_em_diff;
        dr::masked(bs.sampled_type, emmerged_diff_selected) = +BSDFFlags::DiffuseReflection;
        dr::masked(bs.pdf, emmerged_diff_selected) = this->pdf(ctx, si, wo_em_diff, active);
        dr::masked(value, emmerged_diff_selected & dr::neq(bs.pdf, 0.f)) = this->eval_ex(ctx, si, wo_em_diff, sample2_extra, active) / bs.pdf;
        // coating reflection is selected
        dr::masked(bs.sampled_type, coating_reflection_selected) = +BSDFFlags::Delta1DReflection;
        dr::masked(bs.wo, coating_reflection_selected) = wo_coat_reflect;
        dr::masked(bs.pdf, coating_reflection_selected) = dr::detach((1.f - p_e) * r_i);
        dr::masked(value, coating_reflection_selected & dr::neq(bs.pdf, 0.f)) = value_coat_reflect;
        // immersed micrograin's specular component is selected
        dr::masked(bs.wo, immersed_micrograin_spec_selected) = wo_im_spec;
        dr::masked(bs.sampled_type, immersed_micrograin_spec_selected) = +BSDFFlags::GlossyReflection;
        dr::masked(bs.pdf, immersed_micrograin_spec_selected) = this->pdf(ctx, si, wo_im_spec, active);
        dr::masked(value, immersed_micrograin_spec_selected & dr::neq(bs.pdf, 0.f)) = this->eval_ex(ctx, si, wo_im_spec, sample2_extra, active) / bs.pdf;
        // immersed micrograin's diffuse component is selected
        dr::masked(bs.wo, immersed_micrograin_diff_selected) = wo_im_diff;
        dr::masked(bs.sampled_type, immersed_micrograin_diff_selected) = +BSDFFlags::DiffuseReflection;
        dr::masked(bs.pdf, immersed_micrograin_diff_selected) = this->pdf(ctx, si, wo_im_diff, active);
        dr::masked(value, immersed_micrograin_diff_selected & dr::neq(bs.pdf, 0.f)) = this->eval_ex(ctx, si, wo_im_diff, sample2_extra, active) / bs.pdf;
        // immersed bulk path is selected
        dr::masked(bs.wo, immersed_bulk_selected) = wo_im_bulk;
        dr::masked(bs.sampled_type, immersed_bulk_selected) = bs_bulk.sampled_type;
        dr::masked(bs.pdf, immersed_bulk_selected) = this->pdf(ctx, si, wo_im_bulk, active);
        dr::masked(value, immersed_bulk_selected & dr::neq(bs.pdf, 0.f)) = this->eval_ex(ctx, si, wo_im_bulk, sample2_extra, active) / bs.pdf;
        
        // if(dr::any_nested(dr::isnan(value))) {
        //     Log(Error, "NaN detected in CoatedMicrograinBSDF::sample_ex(): ");
        // }

        return { bs, value & active };
    }

    MI_DECLARE_CLASS(CoatedMicrograinBSDF)

private:
    ScalarFloat m_coat_ior; // Refractive index of the coating layer
    ref<Texture> m_coat_thickness; //[0,1] thickness of the coating layer, 0 = no coating, 1 = full coating
    ref<MicrograinBSDF<Float, Spectrum>> m_micrograin_bsdf; 
    ref<BSDF<Float, Spectrum>> m_bulk_bsdf; 
    MI_TRAVERSE_CB(Base, 
                   m_coat_ior, m_coat_thickness,
                   m_micrograin_bsdf, m_bulk_bsdf)
};


MI_EXPORT_PLUGIN(CoatedMicrograinBSDF)
NAMESPACE_END(mitsuba)