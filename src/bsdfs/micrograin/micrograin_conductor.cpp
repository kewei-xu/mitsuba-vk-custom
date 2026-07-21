#include <mitsuba/core/fwd.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/string.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/ior.h>
#include <mitsuba/render/texture.h>
#include "micrograin.h"


NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class MicrograinConductor final : public MicrograinBSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(MicrograinBSDF, m_flags, m_components, m_tau_0, m_a, m_b, m_c, m_d, m_radius)
    MI_IMPORT_TYPES(Texture)
    MicrograinConductor(const Properties &props) : Base(props) {
        std::string_view material = props.get<std::string_view>("material", "none");
        if (props.has_property("eta") || material == "none") {
            m_eta = props.get_unbounded_texture<Texture>("eta", 0.f);
            m_k   = props.get_unbounded_texture<Texture>("k", 1.f);
            if (material != "none")
                Throw("Should specify either (eta, k) or material, not both.");
        } else {
            std::tie(m_eta, m_k) = complex_ior_from_file<Spectrum, Texture>(
                props.get<std::string_view>("material", "Cu"));
        }

        if (props.has_property("specular_reflectance"))
            m_specular_reflectance = props.get_texture<Texture>("specular_reflectance", 1.f);

        m_flags = BSDFFlags::GlossyReflection | BSDFFlags::FrontSide;
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
        callback->put("eta", m_eta, ParamFlags::NonDifferentiable);
        callback->put("k", m_k, ParamFlags::NonDifferentiable);
        if (m_specular_reflectance)
            callback->put("specular_reflectance", m_specular_reflectance, ParamFlags::NonDifferentiable);
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "MicroGrainConductor[" << std::endl
            << "  filling_factor = " << string::indent(m_tau_0) << ","
            << std::endl
            << "  a = " << string::indent(m_a) << "," << std::endl
            << "  b = " << string::indent(m_b) << "," << std::endl
            << "  c = " << string::indent(m_c) << "," << std::endl
            << "  d = " << string::indent(m_d) << "," << std::endl
            << "  eta = " << string::indent(m_eta) << "," << std::endl
            << "  k = " << string::indent(m_k) << "," << std::endl;
            if (m_specular_reflectance) 
                oss << "  specular_reflectance = " << string::indent(m_specular_reflectance) << "," << std::endl;
            oss << "]";
        return oss.str();
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &ctx,
                                             const SurfaceInteraction3f &si,
                                             Float /*sample1*/,
                                             const Point2f &sample2,
                                             Mask active) const override {
        Float tau_0      = this->eval_tau_0(si, active);
        Matrix3f M       = this->eval_stretching_matrix3f(si, active);
        Matrix3f M_inv_T = dr::transpose(dr::inverse(M));
        

        // sample micro-normal
        Vector3f m_1 = square_to_sphere_micrograin<Float>(tau_0, sample2);
        Normal3f m   = dr::normalize(M_inv_T * m_1);

        Vector3f wo = reflect(si.wi, m);
        BSDFSample3f bs(wo);

        bs.pdf = pdf(ctx, si, wo, active);

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Mask valid = (dr::neq(bs.pdf, 0.f)) & (cos_theta_o > 0.f) & (cos_theta_i > 0.f) &
                     (dr::dot(si.wi, m)) > 0.f & (dr::dot(wo, m) > 0.f);

        Spectrum value = dr::select(valid & active,
                                    eval(ctx, si, bs.wo, active) / bs.pdf, 0.f);

        return { bs, value };
    }

    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Vector3f m = dr::normalize(wo + si.wi);

        Mask valid = (cos_theta_o > 0.f) & (cos_theta_i > 0.f) &
                     (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);

        Float tau_0          = this->eval_tau_0(si, active);
        auto [M, abs_det_M]  = this->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_T   = dr::transpose(M);
        Vector3f m_1   = dr::normalize(M_T * m);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float coeff    = abs_det_M / (norm_sqr * norm_sqr);
        Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
        Float cos_theta_m = Frame3f::cos_theta(m);
        // sample pdf
        Float pdf_ = D_ * cos_theta_m;
        // jacobian half vector
        pdf_ /= (4.f * dr::dot(wo, m));
        
        return dr::select(active & valid, pdf_, 0.f);
    }


    Spectrum eval(const BSDFContext &ctx,
                  const SurfaceInteraction3f &si,
                  const Vector3f &wo,
                  Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Float tau_0 = this->eval_tau_0(si, active);
        auto [M, abs_det_M]  = this->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv = dr::inverse(M);
        Matrix3f M_T   = dr::transpose(M);
        
        Vector3f m = dr::normalize(si.wi + wo);
        // compute NDF
        Vector3f m_1 = dr::normalize(M_T * m);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float coeff    = abs_det_M / (norm_sqr * norm_sqr);
        Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
        // compute GAF
        Vector3f wi_1 = dr::normalize(M_inv * si.wi);
        Vector3f wo_1 = dr::normalize(M_inv * wo);
        Float h_m_1   = Frame3f::cos_theta(m_1);
        Mask G2_local = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        Float G2_dist = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        Float G_      = dr::select(G2_local, G2_dist, 0.f);
        // compute Fresnel
        dr::Complex<UnpolarizedSpectrum> eta_c(m_eta->eval(si, active),
                                               m_k->eval(si, active));
        Spectrum F;
        if constexpr (is_polarized_v<Spectrum>) {
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : wo;
            F = mueller::specular_reflection(UnpolarizedSpectrum(dot(wo_hat, m)), eta_c);
            Vector3f s_axis_in  = dr::cross(m, -wo_hat);
            Vector3f s_axis_out = dr::cross(m, wi_hat);
            Mask collinear = dr::all(s_axis_in ==  Vector3f(0));
            s_axis_in  = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_in));
            s_axis_out = dr::select(collinear, Vector3f(1, 0, 0),
                                               dr::normalize(s_axis_out));
            F = mueller::rotate_mueller_basis(F,
                                              -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat),
                                               wi_hat, s_axis_out, mueller::stokes_basis(wi_hat));
        } else {
            F = fresnel_conductor(UnpolarizedSpectrum(dr::dot(si.wi, m)), eta_c);
        }

        Spectrum value = D_ * G_ * F / (4.f * cos_theta_i /* * cos_theta_o*/);

        if (m_specular_reflectance) {
            value *= m_specular_reflectance->eval(si, active);
        }

        Mask valid = cos_theta_o > 0.f & cos_theta_i > 0.f &
                     dr::dot(si.wi, m) > 0.f & dr::dot(wo, m) > 0.f;
        
        return dr::select(active & valid, value /* * cos_theta_o*/, 0.f);
    }

    Spectrum eval_fresnel(const BSDFContext &ctx,
                          const SurfaceInteraction3f &si, 
                          const Vector3f &wo,
                          const Vector3f &m, 
                          Mask active = true) const override {
        dr::Complex<UnpolarizedSpectrum> eta_c(m_eta->eval(si, active),
                                               m_k->eval(si, active));
        Spectrum F;
        if constexpr (is_polarized_v<Spectrum>) {
            Vector3f wo_hat = ctx.mode == TransportMode::Radiance ? wo : si.wi,
                     wi_hat = ctx.mode == TransportMode::Radiance ? si.wi : wo;
            F               = mueller::specular_reflection(
                UnpolarizedSpectrum(dot(wo_hat, m)), eta_c);
            Vector3f s_axis_in  = dr::cross(m, -wo_hat);
            Vector3f s_axis_out = dr::cross(m, wi_hat);
            Mask collinear      = dr::all(s_axis_in == Vector3f(0));
            s_axis_in           = dr::select(collinear, Vector3f(1, 0, 0),
                                             dr::normalize(s_axis_in));
            s_axis_out          = dr::select(collinear, Vector3f(1, 0, 0),
                                             dr::normalize(s_axis_out));
            F                   = mueller::rotate_mueller_basis(
                F, -wo_hat, s_axis_in, mueller::stokes_basis(-wo_hat), wi_hat,
                s_axis_out, mueller::stokes_basis(wi_hat));
        } else {
            F = fresnel_conductor(UnpolarizedSpectrum(dr::dot(si.wi, m)),
                                  eta_c);
        }
        if (m_specular_reflectance) {
            F *= m_specular_reflectance->eval(si, active);
        }
        return F;
    }

    Float specular_component_sampling_probability(
        const Float /* cos_theta_i*/) const override { 
        return 1.f; 
    }


    MI_DECLARE_CLASS(MicrograinConductor)

private:
    ref<Texture> m_eta, m_k; // Relative refractive index (real component)
                             // Relative refractive index (imaginary component).
    ref<Texture> m_specular_reflectance;
    MI_TRAVERSE_CB(Base, m_eta, m_k, m_specular_reflectance)
};


MI_EXPORT_PLUGIN(MicrograinConductor)
NAMESPACE_END(mitsuba)