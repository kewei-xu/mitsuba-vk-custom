#include <mitsuba/core/fwd.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/string.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>
#include <mitsuba/core/warp.h>
#include "micrograin.h"


NAMESPACE_BEGIN(mitsuba)


template <typename Float, typename Spectrum>
class MicrograinDiffuse final : public MicrograinBSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(MicrograinBSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    MicrograinDiffuse(const Properties &props) : Base(props) {
        m_reflectance = props.get_texture<Texture>("reflectance", 0.5f);
        m_flags = BSDFFlags::DiffuseReflection | BSDFFlags::FrontSide;
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
        callback->put("reflectance", m_reflectance, ParamFlags::NonDifferentiable);
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
            << "  reflectance = " << string::indent(m_reflectance) << std::endl
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
    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext & /* ctx*/, 
                                             const SurfaceInteraction3f & /* si*/,
                                             Float /* sample1 */, 
                                             const Point2f & /* sample2*/,
                                             Mask /* active*/) const override {
        Throw("sample function not supported for anisotropic rough diffuse "
              "material due to failed generation stochastic samples, use "
              "integrator <pathv2> instead ");
        return {dr::zeros<BSDFSample3f>(), 0.f};
    }


    Spectrum eval_ex(const BSDFContext &ctx, 
                     const SurfaceInteraction3f &si,
                     const Vector3f &wo, 
                     const Point2f &sample2_extra, 
                     Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        Float tau_0         = this->eval_tau_0(si, active);
        auto [M, abs_det_M] = this->eval_stretching_matrix3f_and_abs_det(si, active);
        Matrix3f M_inv   = dr::inverse(M);
        Matrix3f M_inv_T = dr::transpose(M_inv);
        Matrix3f M_T     = dr::transpose(M);

        // sample micronormal
        Vector3f m_1 = square_to_sphere_micrograin<Float>(tau_0, sample2_extra);
        Vector3f m   = dr::normalize(M_inv_T * m_1);
        // compute NDF and sample pdf
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float coeff    = abs_det_M / (norm_sqr * norm_sqr);
        Float D_       = coeff * NDF_1<Float>(tau_0, m_1);
        Float pdf_m    = D_ * Frame3f::cos_theta(m);
        // compute GAF
        Vector3f wi_1 = dr::normalize(M_inv * si.wi);
        Vector3f wo_1 = dr::normalize(M_inv * wo);
        Float h_m_1   = Frame3f::cos_theta(m_1);
        Float G2_local = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        Float G2_dist  = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        Float G_       = dr::select(G2_local, G2_dist, 0.f);

        Float cos_theta_im = dr::clamp(dr::dot(si.wi, m), 0.f, 1.f);
        Float cos_theta_om = dr::clamp(dr::dot(wo, m), 0.f, 1.f);

        Spectrum value = cos_theta_im * cos_theta_om *
                         m_reflectance->eval(si, active) * dr::InvPi<Float> * 
                         D_ * G_ / pdf_m / cos_theta_i /*/ cos_theta_o*/;
        
        Mask valid = dr::neq(pdf_m, 0.f) & (cos_theta_o > 0.f) & (cos_theta_i > 0.f);

        return dr::select(active & valid, value, 0.f);
    }


    Float pdf(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              const Vector3f &wo, 
              Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi),
              cos_theta_o = Frame3f::cos_theta(wo);
        Mask valid = (cos_theta_i > 0.f) & (cos_theta_o > 0.f);

        Float pdf_ = warp::square_to_cosine_hemisphere_pdf(wo);
        return dr::select(active & valid, pdf_, 0.f);
    }


    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, 
              const SurfaceInteraction3f &si,
              Float /* sample1*/, 
              const Point2f &sample2,
              const Point2f &sample2_extra, 
              Mask active) const override {
        Float cos_theta_i = Frame3f::cos_theta(si.wi);

        Vector3f wo = warp::square_to_cosine_hemisphere(sample2);
        BSDFSample3f bs(wo);
        bs.pdf = pdf(ctx, si, wo, active);
        // Ensure that this is a valid sample
        Mask valid = dr::neq(bs.pdf, 0.f) & 
                     (Frame3f::cos_theta(bs.wo) > 0.f) & (cos_theta_i > 0.f);

        Spectrum value = dr::select(active & valid, 
                                    eval_ex(ctx, si, bs.wo, sample2_extra, active) / bs.pdf, 0.f);

        return { bs, value };
    }

    Spectrum eval_weighted_albedo(const SurfaceInteraction3f &si,
                                  const Vector3f &wo, 
                                  const Vector3f &m,
                                  Mask active = true) const override {
        Float cos_theta_im = dr::clamp(dr::dot(si.wi, m), 0.f, 1.f);
        Float cos_theta_om = dr::clamp(dr::dot(wo, m), 0.f, 1.f);

        Spectrum value = m_reflectance->eval(si, active) * dr::InvPi<Float> *
                         cos_theta_im * cos_theta_om;
        return value;
    }

    Float specular_component_sampling_probability(
    const Float /* cos_theta_i */ ) const override{
        return 0.f;
    }

    MI_DECLARE_CLASS(MicrograinDiffuse)


private:
    ref<Texture> m_reflectance;
    MI_TRAVERSE_CB(Base, m_reflectance)
};


MI_EXPORT_PLUGIN(MicrograinDiffuse) 
NAMESPACE_END(mitsuba)