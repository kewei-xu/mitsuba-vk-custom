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
class MicrograinPlastic final : public MicrograinBSDF<Float, Spectrum>{
public:
    MI_IMPORT_BASE(MicrograinBSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    MicrograinPlastic(const Properties &props) : Base(props) {
        ScalarFloat int_ior = lookup_ior(props, "int_ior", "polypropylene");
        ScalarFloat ext_ior = lookup_ior(props, "ext_ior", "air");
        if (int_ior < 0.f || ext_ior < 0.f || int_ior == ext_ior) {
            Throw("The interior and exterior indices of "
                  "refraction must be positive and differ!");
        }
        m_eta = int_ior / ext_ior;
        m_diffuse_reflectance = props.get_texture<Texture>("diffuse_reflectance", 0.5f);
        if (props.has_property("specular_reflectance")) {
            m_specular_reflectance = props.get_texture<Texture>("specular_reflectance", 0.5f);
        }
        Float d_mean = m_diffuse_reflectance->mean();
        Float s_mean = std::get<0>(fresnel(Float(0.f), m_eta));
        if (m_specular_reflectance) {
            s_mean *= m_specular_reflectance->mean();
        }
        m_specular_sampling_weight = s_mean / (s_mean + d_mean);

        auto glossy_flags  = BSDFFlags::GlossyReflection | BSDFFlags::FrontSide;
        auto diffuse_flags = BSDFFlags::DiffuseReflection | BSDFFlags::FrontSide;

        m_flags = glossy_flags | diffuse_flags;
        m_flags = m_flags | BSDFFlags::Anisotropic;

        m_components.clear();
        m_components.push_back(m_flags);
    }

    void traverse(TraversalCallback *callback) override {
        callback->put("filling_factor", m_tau_0, ParamFlags::NonDifferentiable);
        callback->put("a", m_a, ParamFlags::NonDifferentiable);
        callback->put("b", m_b, ParamFlags::NonDifferentiable);
        callback->put("c", m_c, ParamFlags::NonDifferentiable);
        callback->put("d", m_d, ParamFlags::NonDifferentiable);
        callback->put("diffuse_reflectance", m_diffuse_reflectance, ParamFlags::NonDifferentiable);
        callback->put("eta", m_eta, ParamFlags::NonDifferentiable);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "MicroGrainDiffuse[" << std::endl
            << "  filling_factor = " << string::indent(m_tau_0) << ","
            << std::endl
            << "  a = " << string::indent(m_a) << "," << std::endl
            << "  b = " << string::indent(m_b) << "," << std::endl
            << "  c = " << string::indent(m_c) << "," << std::endl
            << "  d = " << string::indent(m_d) << "," << std::endl
            << "  diffuse_reflectance = "
            << string::indent(m_diffuse_reflectance) << "," << std::endl
            << "  eta = " << string::indent(m_eta) << "," << std::endl
            << "]";
        return oss.str();
    }

    Spectrum eval(const BSDFContext & /* ctx*/,
                  const SurfaceInteraction3f & /* si*/,
                  const Vector3f & /* wo*/, 
                  Mask /* active*/) const override {
        Throw("eval function not supported for anisotropic rough diffuse "
              "material due to failed generation stochastic samples, use "
              "integrator <pathv2> instead ");
        return 0.f;
    }
    std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext & /* ctx*/, 
           const SurfaceInteraction3f & /* si*/,
           Float /* sample1 */, 
           const Point2f & /* sample2*/,
           Mask /* active*/) const override {
        Throw("sample function not supported for anisotropic rough diffuse "
              "material due to failed generation stochastic samples, use "
              "integrator <pathv2> instead ");
        return { dr::zeros<BSDFSample3f>(), 0.f };
    }

    Spectrum eval(const BSDFContext &ctx, 
                  const SurfaceInteraction3f &si,
                  const Vector3f &wo, 
                  const Point2f &sample2_extra,
                  Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Float tau_0 = this->eval_tau_0(si, active);
        auto [M, abs_det_M]  = this->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv   = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);
        Matrix3f M_T     = dr::transpose(M);

        //-------- compute specular part ----------//
        Vector3f m = dr::normalize(si.wi + wo);
        // compute NDF 
        Vector3f m_1  = dr::normalize(M_T * m);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float coeff    = abs_det_M / (norm_sqr * norm_sqr);
        Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
        Mask valid_spec = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        // compute GAF
        Vector3f wi_1  = dr::normalize(M_inv * si.wi);
        Vector3f wo_1  = dr::normalize(M_inv * wo);
        Float h_m_1 = Frame3f::cos_theta(m_1);
        Mask G2_local  = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        Float G2_dist  = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        Float G_       = dr::select(G2_local, G2_dist, 0.f);
        // compute Fresnel
        Float F = std::get<0>(fresnel(dr::dot(si.wi, m), m_eta));
        Spectrum value_spec = D_ * G_ * F / (4.f * cos_theta_i /*cos_theta_o*/);
        if (m_specular_reflectance) {
            value_spec *= m_specular_reflectance->eval(si, active);
        }
        //-------- compute diffuse part ----------//
        Spectrum Kd = m_diffuse_reflectance->eval(si, active);
        // sample micronormal
        m_1 = square_to_sphere_micrograin<Float>(tau_0, sample2_extra);
        m   = dr::normalize(M_inv_T * m_1);
        // compute NDF and sample pdf
        norm_sqr = dr::squared_norm(M_T * m);
        coeff    = abs_det_M / (norm_sqr * norm_sqr);
        D_       = coeff * NDF_1<Float>(tau_0, m_1);
        Float pdf_m = D_ * Frame3f::cos_theta(m);
        // compute GAF
        wi_1 = dr::normalize(M_inv * si.wi);
        wo_1 = dr::normalize(M_inv * wo);
        h_m_1   = Frame3f::cos_theta(m_1);
        G2_local = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        G2_dist  = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        G_            = dr::select(G2_local, G2_dist, 0.f);
        // compute diffuse coefficient
        Float cos_theta_im = dr::clamp(dr::dot(si.wi, m), 0.f, 1.f);
        Float cos_theta_om = dr::clamp(dr::dot(wo, m), 0.f, 1.f);
        
        Float f_i = std::get<0>(fresnel(cos_theta_im, m_eta));
        Float f_o = std::get<0>(fresnel(cos_theta_om, m_eta));

        Float T = (1.f - f_o) * (1.f - f_i);

        Spectrum value_diff = cos_theta_im * cos_theta_om * Kd * T * dr::InvPi<Float> * 
                              D_ * G_ / pdf_m / (cos_theta_i /*cos_theta_o*/);
        
        Mask valid = (cos_theta_i > 0.f) & (cos_theta_o > 0.f) & valid_spec & dr::neq(pdf_m, 0.f);
        return dr::select(active & valid, value_spec + value_diff, 0.f);
    }

    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {
        Float tau_0 = this->eval_tau_0(si, active);
        auto [M, abs_det_M]  = this->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_T = dr::transpose(M);

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Float f_i  = std::get<0>(fresnel(cos_theta_i, m_eta));
        Float p_spec = f_i * m_specular_sampling_weight;
        Float p_diff = (1.f - f_i) * (1.f - m_specular_sampling_weight);
        p_spec       = p_spec / (p_spec + p_diff);
        p_diff       = 1.f - p_spec;
        Vector3f hm    = dr::normalize(si.wi + wo);
        Vector3f hm_1  = dr::normalize(M_T * hm);
        Float norm_sqr = dr::squared_norm(M_T * hm);
        Float coeff    = abs_det_M / (norm_sqr * norm_sqr);
        Float D_       = coeff * NDF_1<Float>(tau_0, hm_1);
        Float cos_theta_m = Frame3f::cos_theta(hm);
        Float pdf_spec    = D_ * cos_theta_m;
        pdf_spec /= (4.f * dr::dot(wo, hm));
        Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);
        
        Float pdf_ = p_spec * pdf_spec + p_diff * pdf_diff;

        Mask valid = (cos_theta_i > 0.f) & (cos_theta_o > 0.f) &
                     (dr::dot(si.wi, hm) > 0.f) & (dr::dot(wo, hm) > 0.f);
        return dr::select(active & valid, pdf_, 0.f);
    }

    std::pair<BSDFSample3f, Spectrum>
    sample(const BSDFContext &ctx, 
           const SurfaceInteraction3f &si,
           Float sample1, 
           const Point2f &sample2,
           const Point2f &sample2_extra, 
           Mask active) const override {
        Float tau_0 = this->eval_tau_0(si, active);
        Matrix3f M  = this->eval_stretching_matrix3f(si, active);
        Matrix3f M_inv_T = dr::transpose(dr::inverse(M));

        Float cos_theta_i = Frame3f::cos_theta(si.wi);

        Float f_i            = std::get<0>(fresnel(cos_theta_i, m_eta));
        Float proba_specular = f_i * m_specular_sampling_weight;
        Float proba_diffuse  = (1.f - f_i) * (1.f - m_specular_sampling_weight);

        proba_specular = proba_specular / (proba_specular + proba_diffuse);
        proba_diffuse  = 1.f - proba_specular;

        Mask specular_selected = sample1 < proba_specular;

        // sample micro-normal for specular part
        Vector3f m_1 = square_to_sphere_micrograin<Float>(tau_0, sample2);
        Normal3f m   = dr::normalize(M_inv_T * m_1);

        Vector3f wo = dr::select(specular_selected, reflect(si.wi, m),
                                 warp::square_to_cosine_hemisphere(sample2));

        Float pdf_ = pdf(ctx, si, wo, active);

        BSDFSample3f bs(wo);
        bs.pdf = pdf_;

        bs.sampled_type = dr::select(specular_selected, +BSDFFlags::GlossyReflection,
                                                        +BSDFFlags::DiffuseReflection);

        bs.sampled_component = dr::select(specular_selected, 0, 1);

        // Ensure that this is a valid sample
        Mask valid = dr::neq(bs.pdf, 0.f) & cos_theta_i > 0.f & Frame3f::cos_theta(bs.wo) > 0.f;

        Spectrum value = dr::select(valid & active, 
                                    eval(ctx, si, bs.wo, sample2_extra, active) / bs.pdf, 0.f);

        return { bs, value };
    }

    Spectrum eval_fresnel(const SurfaceInteraction3f &si,
                          const Vector3f &m,
                          Mask active = true) const override {
        Spectrum F             = std::get<0>(fresnel(dr::dot(si.wi, m), m_eta));
        if (m_specular_reflectance) {
            F *= m_specular_reflectance->eval(si, active);
        }
        return F;
    }

    Spectrum eval_weighted_albedo(const SurfaceInteraction3f &si,
                                  const Vector3f &wo, 
                                  const Vector3f &m,
                                  Mask active = true) const override {
        Spectrum Kd        = m_diffuse_reflectance->eval(si, active);

        Float cos_theta_im = dr::clamp(dr::dot(si.wi, m), 0.f, 1.f);
        Float cos_theta_om = dr::clamp(dr::dot(wo, m), 0.f, 1.f);

        Float f_i = std::get<0>(fresnel(cos_theta_im, m_eta));
        Float f_o = std::get<0>(fresnel(cos_theta_om, m_eta));

        Float T = (1.f - f_o) * (1.f - f_i);

        Spectrum value = Kd * T * dr::InvPi<Float> * cos_theta_im * cos_theta_om;

        return value;
    }

    Float specular_component_sampling_probability(
        const Float cos_theta_i) const override {
        Float f_i            = std::get<0>(fresnel(cos_theta_i, m_eta));
        Float proba_specular = f_i * m_specular_sampling_weight;
        Float proba_diffuse  = (1.f - f_i) * (1.f - m_specular_sampling_weight);
        proba_specular = proba_specular / (proba_specular + proba_diffuse);
        return dr::clamp(proba_specular, 0.f, 1.f);
    }

    MI_DECLARE_CLASS(MicrograinPlastic)

private:
    ref<Texture> m_diffuse_reflectance;
    ref<Texture> m_specular_reflectance;
    Float m_eta;
    Float m_specular_sampling_weight;

    MI_TRAVERSE_CB(Base, 
                   m_diffuse_reflectance, m_specular_reflectance,
                   m_eta, m_specular_sampling_weight)

};



MI_EXPORT_PLUGIN(MicrograinPlastic)
NAMESPACE_END(mitsuba)