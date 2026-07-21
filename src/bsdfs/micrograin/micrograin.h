#pragma once

#include <mitsuba/core/profiler.h>
#include <mitsuba/render/interaction.h>
#include <drjit/call.h>
#include <mitsuba/render/bsdf.h>

NAMESPACE_BEGIN(mitsuba)

// -----------------------------------------------------------------------
// --------- Micrograin BSDF Plugin (for bsdf/micrograin.cpp) ------------
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// ------------------ helper functions (mono micrograin)------------------
// -----------------------------------------------------------------------

template <typename Float>
std::pair<Vector<Float, 2>, Vector<Float, 2>>
silhouette_points(const Vector<Float, 3> &wi_1, const Vector<Float, 3> &wo_1,
                  Float h_m_1) {
    using Vector2f = Vector<Float, 2>;
    using Frame3f  = Frame<Float>;

    Float cos_theta_i_2 = Frame3f::cos_theta_2(wi_1);
    Float sin_theta_i_2 = Frame3f::sin_theta_2(wi_1);
    Float h_m_1_2       = h_m_1 * h_m_1;
    Float tan_theta_i   = Frame3f::tan_theta(wi_1);
    Float sign          = dr::sign(wi_1.x() * wo_1.y() - wo_1.x() * wi_1.y());

    h_m_1_2       = dr::clamp(h_m_1_2, 0.f, 1.f);
    sin_theta_i_2 = dr::clamp(sin_theta_i_2, 0.f, 1.f);

    Float x_qi = dr::sqrt(1.f - h_m_1_2 / sin_theta_i_2) * sign;
    Float y_qi = -h_m_1 / tan_theta_i;

    auto [sin_phi_i, cos_phi_i] = Frame3f::sincos_phi(wi_1);
    Vector2f pi_p(x_qi * sin_phi_i + y_qi * cos_phi_i,
                  -x_qi * cos_phi_i + y_qi * sin_phi_i);
    Vector2f pi_m(-x_qi * sin_phi_i + y_qi * cos_phi_i,
                  x_qi * cos_phi_i + y_qi * sin_phi_i);
    return { pi_p, pi_m };
}

template <typename Float>
std::pair<Vector<Float, 2>, Vector<Float, 2>>
silhouette_points_0(const Vector<Float, 3> &wi_1,
                    const Vector<Float, 3> &wo_1) {
    using Vector2f = Vector<Float, 2>;
    using Frame3f  = Frame<Float>;
    Float x_qi     = dr::sign(wi_1.x() * wo_1.y() - wo_1.x() * wi_1.y());
    auto [sin_phi_i, cos_phi_i] = Frame3f::sincos_phi(wi_1);
    Vector2f pi_p(x_qi * sin_phi_i, -x_qi * cos_phi_i);
    Vector2f pi_m(-x_qi * sin_phi_i, x_qi * cos_phi_i);
    return { pi_p, pi_m };
}

template <typename Float>
Vector<Float, 2> shadow_point(const Vector<Float, 3> &wo_1, Float h_m_1,
                              const Vector<Float, 3> &t,
                              const Vector<Float, 3> &b) {
    using Vector2f = Vector<Float, 2>;
    using Vector3f = Vector<Float, 3>;
    Float a0       = -t.z() / b.z();
    Float a1       = h_m_1 / b.z();
    Float a0_2     = a0 * a0;
    Float a1_2     = a1 * a1;

    Float sy =
        1.f / dr::sqrt(1.f - dr::clamp(dr::sqr(dr::dot(b, wo_1)), 0.f, 1.f));

    Float sy_2 = sy * sy;

    Float x = (-a0 * a1 / sy_2 - dr::sqrt((a0_2 - a1_2) / sy_2 + 1.f)) /
              (a0_2 / sy_2 + 1.f);
    Float y    = a0 * x + a1;
    Vector3f p = t * x + b * y;
    return Vector2f(p.x(), p.y());
}

template <typename Float>
Vector<Float, 2> shadow_point_0(const Vector<Float, 3> &wo_1,
                                const Vector<Float, 3> &t,
                                const Vector<Float, 3> &b) {
    using Vector2f = Vector<Float, 2>;
    using Vector3f = Vector<Float, 3>;

    Float a0   = -t.z() / b.z();
    Float a0_2 = a0 * a0;

    Float sy =
        1.f / dr::sqrt(1.f - dr::clamp(dr::sqr(dr::dot(b, wo_1)), 0.f, 1.f));
    Float sy_2 = sy * sy;
    Float x    = (-dr::sqrt(a0_2 / sy_2 + 1.f)) / (a0_2 / sy_2 + 1.f);
    Float y    = a0 * x;
    Vector3f p = t * x + b * y;

    return Vector2f(p.x(), p.y());
}

template <typename Float, typename Mask>
Mask isLeft(const Vector<Float, 2> &a, const Vector<Float, 2> &b,
            const Vector<Float, 2> &c) {
    // using Mask = dr::mask_t<Float>;
    Mask mask =
        (b.x() - a.x()) * (c.y() - a.y()) - (b.y() - a.y()) * (c.x() - a.x()) >
        0.f;
    return mask;
}

template <typename Float> Float sigma_s_0(Float cos_theta_i) {
    return (-dr::Pi<Float> + dr::Pi<Float> / cos_theta_i) * 0.5f;
}

template <typename Float> Float sigma_s(Float cos_theta_i, Float cos_theta_m) {
    Float cos_theta_i_2 = cos_theta_i * cos_theta_i;
    Float cos_theta_m_2 = cos_theta_m * cos_theta_m;
    Float sin_theta_i_2 = 1.f - cos_theta_i_2;
    Float sin_theta_m_2 = 1.f - cos_theta_m_2;
    Float tan_theta_i   = dr::sqrt(sin_theta_i_2 / cos_theta_i_2);
    Float tan_theta_m   = dr::sqrt(sin_theta_m_2 / cos_theta_m_2);

    Float sig_e = dr::acos(dr::clamp((1.f / tan_theta_i + tan_theta_i) *
                                         cos_theta_m * cos_theta_i,
                                     -1.f, 1.f)) /
                  cos_theta_i;
    Float sig_c =
        dr::acos(dr::clamp(1.f / (tan_theta_i * tan_theta_m), -1.f, 1.f)) *
        sin_theta_m_2;
    Float sig_t = dr::sqrt(1.f - cos_theta_m_2 / sin_theta_i_2) * tan_theta_i *
                  cos_theta_m;
    return sig_e - sig_c - sig_t;
}

template <typename Float>
Float area_triangle(const Vector<Float, 2> &a, const Vector<Float, 2> &b,
                    const Vector<Float, 2> &c) {
    Float area = a.x() * (b.y() - c.y()) + b.x() * (c.y() - a.y()) +
                 c.x() * (a.y() - b.y());
    return 0.5f * dr::abs(area);
}

template <typename Float>
Float area_sector(const Vector<Float, 2> &p1, const Vector<Float, 2> &p2,
                  Float rad) {
    Float cos_theta =
        dr::clamp(dr::dot(p1, p2) / (dr::norm(p1) * dr::norm(p2)), -1.f, 1.f);
    return (dr::acos(cos_theta) * rad * rad -
            dr::abs(p1.x() * p2.y() - p2.x() * p1.y())) *
           0.5f;
}

template <typename Float>
Float area_sector_i(const Vector<Float, 2> &p, const Vector<Float, 2> &pi,
                    Float h_m_1, const Vector<Float, 3> &wi_1) {
    using Vector2f = Vector<Float, 2>;
    using Matrix2f = dr::Matrix<Float, 2>;

    Vector2f direction = dr::normalize(Vector2f(wi_1.x(), wi_1.y()));
    Float cos_theta_i  = dr::maximum(wi_1.z(), 0.f);
    Float tan_theta_i = dr::sqrt(1.f - cos_theta_i * cos_theta_i) / cos_theta_i;
    Vector2f centered_p  = p - direction * tan_theta_i * h_m_1;
    Vector2f centered_pi = pi - direction * tan_theta_i * h_m_1;

    Matrix2f E(direction.x() / cos_theta_i, -direction.y(), 
               direction.y() / cos_theta_i,  direction.x());

    Matrix2f inv_E = dr::inverse(E);

    centered_p  = inv_E * centered_p;
    centered_pi = inv_E * centered_pi;


    return area_sector<Float>(centered_p, centered_pi, 1.f) * dr::det(E);
}

template <typename Float>
Float sigma_intersection(const Vector<Float, 3> &wi_1,
                         const Vector<Float, 3> &wo_1, Float h_m_1,
                         const Vector<Float, 2> &p, const Vector<Float, 2> &pi,
                         const Vector<Float, 2> &po) {
    Float sin_theta = dr::sqrt(dr::maximum(1.f - h_m_1 * h_m_1, 0.f));

    Float At = area_triangle(p, pi, po);
    Float As = area_sector(pi, po, sin_theta);
    Float Ai = area_sector_i(p, pi, h_m_1, wi_1);
    Float Ao = area_sector_i(p, po, h_m_1, wo_1);

    Float A = At - As + Ai + Ao;
    return A;
}

template <typename Float>
Float sigma_intersection_0(const Vector<Float, 3> &wi,
                           const Vector<Float, 3> &wo,
                           const Vector<Float, 2> &p,
                           const Vector<Float, 2> &pi,
                           const Vector<Float, 2> &po) {
    using Vector3f = Vector<Float, 3>;

    Float At = area_triangle<Float>(p, pi, po);
    Float As = area_sector<Float>(pi, po, 1.f);
    Float Ai = area_sector_i<Float>(p, pi, 0.f, wi);
    Float Ao = area_sector_i<Float>(p, po, 0.f, wo);

    Float A = At - As + Ai + Ao;
    return A;
}

template <typename Float>
Float G2_HD(Float tau_0, const Vector<Float, 3> &wi_1,
            const Vector<Float, 3> &wo_1, Float h_m_1) {
    using Frame3f  = Frame<Float>;
    using Mask     = dr::mask_t<Float>;
    using Vector2f = Vector<Float, 2>;
    using Vector3f = Vector<Float, 3>;

    h_m_1 = dr::clamp(h_m_1, 0.f, 1.f);

    Float cos_theta_i = Frame3f::cos_theta(wi_1);
    Float cos_theta_o = Frame3f::cos_theta(wo_1);

    Float sin_theta_i = dr::sqrt(1.f - cos_theta_i * cos_theta_i);
    Float sin_theta_o = dr::sqrt(1.f - cos_theta_o * cos_theta_o);

    // Check if there is a shadow for direction i and o
    Mask has_shadow_i = h_m_1 < sin_theta_i;
    Mask has_shadow_o = h_m_1 < sin_theta_o;

    cos_theta_i = dr::clamp(cos_theta_i, 0.f, 1.f);
    cos_theta_o = dr::clamp(cos_theta_o, 0.f, 1.f);

    // Orthogonal frame
    Vector3f vb = dr::normalize(wi_1 + wo_1);
    Vector3f vt = dr::normalize(dr::cross(wi_1, wo_1));
    vt          = vt * dr::sign(-vt.z());
    Vector3f vn = dr::normalize(dr::cross(vt, vb));

    // Silhouette points i
    auto [pi_p, pi_m] = silhouette_points<Float>(wi_1, wo_1, h_m_1);
    // Silhouette points o
    auto [po_p, po_m] = silhouette_points<Float>(wo_1, wi_1, h_m_1);
    Vector2f p        = shadow_point<Float>(wo_1, h_m_1, vt, vb);

    // Split line
    Vector2f p1(-vn.z() / vn.x() * h_m_1, 0.f);
    Vector2f p2 = p1 + Vector2f(-vn.y() / vn.x(), 1.f);

    // Get relative position of all 4 silhouette points
    Mask il_pi_p = isLeft<Float, Mask>(p1, p2, pi_p);
    Mask il_pi_m = isLeft<Float, Mask>(p1, p2, pi_m);
    Mask il_po_p = isLeft<Float, Mask>(p1, p2, po_p);
    Mask il_po_m = isLeft<Float, Mask>(p1, p2, po_m);

    // We should always find : same_side_pi = same_side_po
    Mask same_side_pi = ~(il_pi_p ^ il_pi_m);
    Mask same_side_po = ~(il_po_p ^ il_po_m);

    // Find where a shadow is inside the other
    Mask full = same_side_pi & same_side_po & ~(il_pi_p ^ il_po_m);
    // Find where the shadows are not overlaped
    Mask null = same_side_pi & same_side_po & (il_pi_p ^ il_po_m);

    // Shadow area in direction i and o
    Float sig_si = sigma_s<Float>(cos_theta_i, h_m_1);
    Float sig_so = sigma_s<Float>(cos_theta_o, h_m_1);

    // Intersection area between the two shadows
    Float sig_in = sigma_intersection<Float>(wi_1, wo_1, h_m_1, p, pi_p, po_p);

    Mask not_colinear = dr::dot(wi_1, wo_1) < 1.f;

    Float sig   = dr::select(has_shadow_i, sig_si, 0.f);
    sig         = dr::select(has_shadow_o & not_colinear, sig + sig_so, sig);
    sig         = dr::select(full & has_shadow_i & has_shadow_o & not_colinear,
                             sig - dr::minimum(sig_si, sig_so), sig);
    sig         = dr::select((~null) & (~full) & has_shadow_i & has_shadow_o &
                                 not_colinear,
                             sig - sig_in, sig);
    Float coeff = dr::log(1.f - tau_0) * dr::InvPi<Float>;
    Float value = dr::exp(coeff * sig);
    return value;
}

template <typename Float>
Float G2_HD_0(Float tau_0, const Vector<Float, 3> &wi_1,
              const Vector<Float, 3> &wo_1) {
    using Vector2f = Vector<Float, 2>;
    using Vector3f = Vector<Float, 3>;
    using Frame3f  = Frame<Float>;
    using Mask     = dr::mask_t<Float>;

    Float cos_theta_i = Frame3f::cos_theta(wi_1);
    Float cos_theta_o = Frame3f::cos_theta(wo_1);
    // Check if there is a shadow for direction i and o
    Mask has_shadow_i = cos_theta_i < 1.f;
    Mask has_shadow_o = cos_theta_o < 1.f;
    // Orthogonal frame
    Vector3f vb = dr::normalize(wi_1 + wo_1);
    Vector3f vt = dr::normalize(dr::cross(wi_1, wo_1));
    vt          = vt * dr::sign(-vt.z());
    Vector3f vn = dr::normalize(dr::cross(vt, vb));
    // Silhouette points i
    auto [pi_p, pi_m] = silhouette_points_0<Float>(wi_1, wo_1);
    // Silhouette points o
    auto [po_p, po_m] = silhouette_points_0<Float>(wo_1, wi_1);
    // Shadow point
    Vector2f p = shadow_point_0<Float>(wo_1, vt, vb);
    // Shadow area in direction i and o
    Float sig_si = sigma_s_0(dr::maximum(cos_theta_i, 0.f));
    Float sig_so = sigma_s_0(dr::maximum(cos_theta_o, 0.f));
    // Intersection area between the two shadows
    Float sig_in = sigma_intersection_0<Float>(wi_1, wo_1, p, pi_p, po_p);

    // Intersection area between the two shadows
    Mask colinear_same_side = dr::dot(wi_1, wo_1) > 1.f;
    Mask colinear_diff_side = dr::dot(wi_1, wo_1) < -1.f;

    Float sig   = dr::select(has_shadow_i, sig_si, 0.f);
    sig         = dr::select(has_shadow_o, sig + sig_so, sig);
    sig         = dr::select(has_shadow_i & has_shadow_o, sig - sig_in, sig);
    sig         = dr::select(colinear_diff_side & has_shadow_i & has_shadow_o,
                             sig + sig_so, sig);
    sig         = dr::select(colinear_same_side & has_shadow_i & has_shadow_o,
                             dr::maximum(sig_si, sig_so), sig);
    Float coeff = dr::log(1.f - tau_0) * dr::InvPi<Float>;
    return dr::exp(coeff * sig);
}

template <typename Float>
Float G1_HD(Float tau_0, const Vector<Float, 3> &wi_1, Float h_m_1) {

    using Mask    = dr::mask_t<Float>;
    using Frame3f = Frame<Float>;

    h_m_1 = dr::clamp(h_m_1, 0.f, 1.f);

    Float cos_theta_i = Frame3f::cos_theta(wi_1);
    Float sin_theta_i = Frame3f::sin_theta(wi_1);
    // check if there is a shadow for direction i
    Mask has_shadow_i = h_m_1 < sin_theta_i;
    Float sig_s =
        dr::select(has_shadow_i, sigma_s<Float>(cos_theta_i, h_m_1), 0.f);
    Float coeff = dr::log(1.f - tau_0) * dr::InvPi<Float>;
    Float value = dr::exp(coeff * sig_s);

    return value;
}

template <typename Float>
Float G1_HD_0(Float tau_0, const Vector<Float, 3> &wi_1) {

    using Frame3f = Frame<Float>;

    Float cos_theta_i = Frame3f::cos_theta(wi_1);
    Float sig         = sigma_s_0(cos_theta_i);
    Float coeff       = dr::log(1.f - tau_0) * dr::InvPi<Float>;
    Float value       = dr::exp(coeff * sig);

    return value;
}

template <typename Float>
Float NDF_1(Float tau_0, const Vector<Float, 3> &m_1) {
    using Frame3f      = Frame<Float>;
    Float sin2_theta_m = Frame3f::sin_theta_2(m_1);
    Float value = -dr::log(1.f - tau_0) * dr::pow(1.f - tau_0, sin2_theta_m) *
                  dr::InvPi<Float> / tau_0;
    return value;
}

template <typename Float>
Vector<Float, 3> square_to_sphere_micrograin(Float tau_0,
                                             const Point<Float, 2> &sample2) {
    using Vector3f = Vector<Float, 3>;

    Float s1 = sample2.x(); // random number in [0,1] to sample phi_m
    Float s2 = sample2.y(); // random number in [0,1] to sample theta_m

    Float phi_m = dr::TwoPi<Float> * s1;
    Float theta_m =
        dr::asin(dr::sqrt(dr::log(1.f - tau_0 * s2) / dr::log(1.f - tau_0)));

    Float cp = dr::cos(phi_m);
    Float sp = dr::sin(phi_m);
    Float ct = dr::cos(theta_m);
    Float st = dr::sin(theta_m);

    return Vector3f(cp * st, sp * st, ct);
}

template <typename Float>
Vector<Float, 3> square_to_sphere_micrograin_pdf(Float tau_0,
                                                 const Vector<Float, 3> &m_1) {
    using Frame3f     = Frame<Float>;
    Float cos_theta_m = Frame3f::cos_theta(m_1);
    Float pdf         = NDF_1<Float>(tau_0, m_1) * cos_theta_m;
    return pdf;
}

// ------------------------------------------------------------------------
// ------------------ Micrograin BSDF Definition --------------------------
//  -----------------------------------------------------------------------

template <typename Float, typename Spectrum>
class MicrograinBSDF : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

public:
    ~MicrograinBSDF() {};

    MI_INLINE Float eval_tau_0(const SurfaceInteraction3f &si,
                               Mask active = true) const {
        return dr::clamp(m_tau_0->eval_1(si, active), 0.0001f, 0.9999f);
    }
    MI_INLINE Float eval_a(const SurfaceInteraction3f &si,
                           Mask active = true) const {
        return m_a->eval_1(si, active);
    }
    MI_INLINE Float eval_b(const SurfaceInteraction3f &si,
                           Mask active = true) const {
        return m_b->eval_1(si, active);
    }
    MI_INLINE Float eval_c(const SurfaceInteraction3f &si,
                           Mask active = true) const {
        return m_c->eval_1(si, active);
    }
    MI_INLINE Float eval_d(const SurfaceInteraction3f &si,
                           Mask active = true) const {
        return m_d->eval_1(si, active);
    }

    MI_INLINE Float eval_radius(const SurfaceInteraction3f &si,
                                Mask active = true) const {
        return dr::maximum(m_radius->eval_1(si, active), 0.0001f);
    }

    MI_INLINE Matrix2f eval_stretching_matrix2f(const SurfaceInteraction3f &si,
                                                Mask active = true) const {
        Float a = eval_a(si, active);
        Float b = eval_b(si, active);
        Float c = eval_c(si, active);
        Float d = eval_d(si, active);
        return Matrix2f(a, b, c, d);
    }

    MI_INLINE Matrix3f eval_stretching_matrix3f(const SurfaceInteraction3f &si,
                                                Mask active = true) const {
        Float a = eval_a(si, active);
        Float b = eval_b(si, active);
        Float c = eval_c(si, active);
        Float d = eval_d(si, active);
        return Matrix3f(a, b, 0.f, c, d, 0.f, 0.f, 0.f, 1.f);
    }

    MI_INLINE std::pair<Matrix3f, Float>
    eval_stretching_matrix3f_and_abs_det(const SurfaceInteraction3f &si,
                                         Mask active = true) const {
        Float a = eval_a(si, active);
        Float b = eval_b(si, active);
        Float c = eval_c(si, active);
        Float d = eval_d(si, active);
        return { Matrix3f(a, b, 0.f, c, d, 0.f, 0.f, 0.f, 1.f),
                 dr::abs(a * d - b * c) };
    }

    MI_INLINE std::tuple<Float, Float, Float, Float, Float, Float>
    eval_abcd_det_and_abs_det(const SurfaceInteraction3f &si,
                              Mask active = true) const {
        Float a       = eval_a(si, active);
        Float b       = eval_b(si, active);
        Float c       = eval_c(si, active);
        Float d       = eval_d(si, active);
        Float det     = a * d - b * c;
        Float abs_det = dr::abs(det);
        return { a, b, c, d, det, abs_det };
    }

    // Lightweight: one-shot fetch tau0 + (a,b,c,d) + det + abs_det
    MI_INLINE std::tuple<Float, Float, Float, Float, Float, Float, Float>
    eval_tau0_abcd_det_absdet(const SurfaceInteraction3f &si,
                              Mask active = true) const {
        Float tau0    = eval_tau_0(si, active);
        Float a       = eval_a(si, active);
        Float b       = eval_b(si, active);
        Float c       = eval_c(si, active);
        Float d       = eval_d(si, active);
        Float det     = a * d - b * c;
        Float abs_det = dr::abs(det);
        return { tau0, a, b, c, d, det, abs_det };
    }

    MI_INLINE void eval_params_abcd_r_tau0_det(const SurfaceInteraction3f &si,
                                               Float &a, Float &b, Float &c,
                                               Float &d, Float &r, Float &tau0,
                                               Float &abs_det, Float &inv_det,
                                               Mask active = true) const {
        a    = eval_a(si, active);
        b    = eval_b(si, active);
        c    = eval_c(si, active);
        d    = eval_d(si, active);
        r    = eval_radius(si, active);
        tau0 = eval_tau_0(si, active);

        Float det = a * d - b * c;
        abs_det   = dr::abs(det);

        inv_det = dr::rcp(det);
    }

    virtual Spectrum eval_fresnel(const BSDFContext &ctx,
                                  const SurfaceInteraction3f &si,
                                  const Vector3f &wo,
                                  const Vector3f &m, Mask active = true) const {
        return 0.f;
    }

    virtual Spectrum eval_weighted_albedo(const SurfaceInteraction3f &si,
                                          const Vector3f &wo, const Vector3f &m,
                                          Mask active = true) const {
        return 0.f;
    }

    virtual Float
    specular_component_sampling_probability(const Float cos_theta_i) const {
        return 0.f;
    }

    /*Float D(const SurfaceInteraction3f &si, const Vector3f &m,
            Mask active = true) const {
        Float tau_0    = eval_tau_0(si, active);
        Matrix3f M     = eval_stretching_matrix3f(si, active);
        Matrix3f M_T   = dr::transpose(M);
        Vector3f m_1   = dr::normalize(M_T * m);
        Float norm_sqr = dr::squared_norm(M_T * m);
        Float coeff    = dr::abs(dr::det(M)) / (norm_sqr * norm_sqr);
        return coeff * NDF_1<Float>(tau_0, m_1);
    }

    Float G2(const SurfaceInteraction3f &si, const Vector3f &m,
             const Vector3f &wo, Mask active = true) const {
        Float tau_0    = eval_tau_0(si, active);
        Matrix3f M     = eval_stretching_matrix3f(si, active);
        Matrix3f M_inv = dr::inverse(M);
        Matrix3f M_T   = dr::transpose(M);
        Vector3f wi_1  = dr::normalize(M_inv * si.wi);
        Vector3f wo_1  = dr::normalize(M_inv * wo);
        Vector3f m_1   = dr::normalize(M_T * m);
        Float h_m_1    = Frame3f::cos_theta(m_1);
        Mask G2_local  = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);
        Float G2_dist  = G2_HD<Float>(tau_0, wi_1, wo_1, h_m_1);
        return dr::select(G2_local, G2_dist, 0.f);
    }*/

    /*Vector3f square_to_micrograin(const SurfaceInteraction3f &si,
                                  const Point2f &sample2,
                                  Mask active = true) const {
        Float tau_0      = eval_tau_0(si, active);
        Matrix3f M       = eval_stretching_matrix3f(si, active);
        Matrix3f M_inv_T = dr::transpose(dr::inverse(M));
        Vector3f m_1     = square_to_sphere_micrograin<Float>(tau_0, sample2);
        Vector3f m       = dr::normalize(M_inv_T * m_1);
        return m;
    }

    Float square_to_micrograin_pdf(const SurfaceInteraction3f &si,
                                   const Vector<Float, 3> &m,
                                   Mask active = true) const {
        Float cos_theta_m = Frame3f::cos_theta(m);
        Float pdf         = this->D(si, m, active) * cos_theta_m;
        return pdf;
    }*/

    MI_DECLARE_CLASS(MicrograinBSDF)

protected:
    MicrograinBSDF(const Properties &props) : Base(props) {
        this->m_tau_0  = props.get_texture<Texture>("filling_factor", 0.2f);
        this->m_radius = props.get_unbounded_texture<Texture>("radius", 1.f);
        this->m_a      = props.get_unbounded_texture<Texture>("a", 1.f);
        this->m_b      = props.get_unbounded_texture<Texture>("b", 0.f);
        this->m_c      = props.get_unbounded_texture<Texture>("c", 0.f);
        this->m_d      = props.get_unbounded_texture<Texture>("d", 1.f);
    }

protected:
    ref<Texture> m_tau_0;            // filling factor
    ref<Texture> m_a, m_b, m_c, m_d; // 2D surface stretching matrix
                                     // M = a, b
                                     //     c, d
    ref<Texture> m_radius;           // micrograin radius(size)

    MI_TRAVERSE_CB(Base, m_tau_0, m_a, m_b, m_c, m_d, m_radius)
};









template <typename Float, typename Spectrum>
class PolyMicrograin : public BSDF<Float, Spectrum> {
public:
    MI_IMPORT_BASE(BSDF, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

public:
    
    static constexpr size_t NbGrainMax = 16;

    struct PackedCache {
        // M = [a b; c d]
        Float a[NbGrainMax], b[NbGrainMax], c[NbGrainMax], d[NbGrainMax];

        Float r[NbGrainMax], r2[NbGrainMax], inv_r[NbGrainMax],
            inv_r2[NbGrainMax];
        Float tau0[NbGrainMax], log_base[NbGrainMax]; // log(1 - tau0)

        Float abs_det[NbGrainMax], inv_det[NbGrainMax];
        Vector3f wi1[NbGrainMax]; // normalize(M^{-1} * wi)
    };

protected:
    // ===== 2x2 embedded ops (faster than Matrix3f inverse/transpose) =====
    MI_INLINE Vector3f mul_Mt(const Float a, const Float b, const Float c,
                              const Float d, const Vector3f &v) const {
        // M^T * [x y]^T = [a c; b d] * [x y]
        return Vector3f(a * v.x() + c * v.y(), b * v.x() + d * v.y(), v.z());
    }

    MI_INLINE Vector3f mul_Minv(const Float a, const Float b, const Float c,
                                const Float d, const Float inv_det,
                                const Vector3f &v) const {
        // M^{-1} = 1/det [ d -b; -c a]
        return Vector3f((d * v.x() - b * v.y()) * inv_det,
                        (-c * v.x() + a * v.y()) * inv_det, v.z());
    }

    MI_INLINE Vector3f mul_MinvT(const Float a, const Float b, const Float c,
                                 const Float d, const Float inv_det,
                                 const Vector3f &v) const {
        // (M^{-1})^T = 1/det [ d -c; -b a]
        return Vector3f((d * v.x() - c * v.y()) * inv_det,
                        (-b * v.x() + a * v.y()) * inv_det, v.z());
    }

    // ===== one-shot cache (no wo) =====
    MI_INLINE void cache_packed(const SurfaceInteraction3f &si, PackedCache &pc,
                                Mask active = true) const {
        for (size_t k = 0; k < m_bsdf_count; ++k) {
            Float a, b, c, d, r, tau0, abs_det, inv_det;
            m_micrograin_bsdfs[k]->eval_params_abcd_r_tau0_det(
                si, a, b, c, d, r, tau0, abs_det, inv_det, active);

            pc.a[k] = a;
            pc.b[k] = b;
            pc.c[k] = c;
            pc.d[k] = d;

            pc.r[k]      = r;
            pc.r2[k]     = r * r;
            pc.inv_r[k]  = dr::rcp(r);
            pc.inv_r2[k] = dr::rcp(pc.r2[k]);

            pc.tau0[k]     = tau0;
            pc.log_base[k] = dr::log(1.f - tau0);

            pc.abs_det[k] = abs_det;
            pc.inv_det[k] = inv_det;

            pc.wi1[k] = dr::normalize(mul_Minv(a, b, c, d, inv_det, si.wi));
        }
    }

    // ===== Packed helpers: log-domain products to shrink DrJit graph =====
    MI_INLINE Float packed_log_prod_1_minus_tau0(const PackedCache &pc,
                                                 Mask active = true) const {
        // log Π (1 - tau0_i) = Σ log(1 - tau0_i)
        Float log_prod = 0.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            log_prod += dr::select(active, pc.log_base[i], 0.f);
        }
        return log_prod;
    }

    MI_INLINE Float packed_global_tau0(const PackedCache &pc,
                                       Mask active = true) const {
        // global_tau0 = 1 - Π(1 - tau0_i) = 1 - exp(Σ log(1 - tau0_i))
        Float log_prod = packed_log_prod_1_minus_tau0(pc, active);
        Float prod     = dr::exp(log_prod);
        return dr::clamp(1.f - prod, 0.f, 0.9999f);
    }

    MI_INLINE Float packed_log_term_lambda(const PackedCache &pc,
                                           Mask active = true) const {
        // term_lambda = Π (1 - tau0_i)^(-1/r_i^2)
        // log term_lambda = Σ [ log(1 - tau0_i) * (-1/r_i^2) ]
        Float log_lambda = 0.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float log_lambda_i =
                -pc.log_base[i] * pc.inv_r2[i]; // -(log(1-tau0))/r^2
            log_lambda += dr::select(active, log_lambda_i, 0.f);
        }
        return log_lambda;
    }

    MI_INLINE Float packed_term_lambda(const PackedCache &pc,
                                       Mask active = true) const {
        return dr::exp(packed_log_term_lambda(pc, active));
    }

    MI_INLINE Float packed_log_lambda_i(const PackedCache &pc, size_t i) const {
        // log lambda_i = log((1 - tau0_i)^(-1/r_i^2)) = -(log_base_i)/r^2
        return -pc.log_base[i] * pc.inv_r2[i];
    }

    MI_INLINE Float packed_proba_level_type_from_logs(
        Float log_lambda_i, Float log_term_lambda) const {
        // proba_level_type = log(lambda_i) / log(term_lambda)
        return dr::clamp(log_lambda_i / log_term_lambda, 0.f, 1.f);
    }

    // ===== wo1 cache (PackedCache + wo) =====
    MI_INLINE void cache_wo1_from_packed(const PackedCache &pc,
                                         const Vector3f &wo,
                                         Vector3f wo1[NbGrainMax],
                                         Mask /*active*/ = true) const {
        for (size_t k = 0; k < m_bsdf_count; ++k) {
            wo1[k] = dr::normalize(mul_Minv(pc.a[k], pc.b[k], pc.c[k], pc.d[k],
                                            pc.inv_det[k], wo));
        }
    }

public:
    ~PolyMicrograin() {}


    MI_INLINE void build_packed_cache(const SurfaceInteraction3f &si,
                                      PackedCache &pc,
                                      Mask active = true) const {
        cache_packed(si, pc, active);
    }

    MI_INLINE void build_wo1_from_cache(const PackedCache &pc,
                                        const Vector3f &wo,
                                        Vector3f wo1[NbGrainMax],
                                        Mask active = true) const {
        cache_wo1_from_packed(pc, wo, wo1, active);
    }


    MI_INLINE Float eval_global_tau_0_from_cache(const PackedCache &pc,
                                                 Mask active = true) const {
        /*Float prod = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            prod *= dr::select(active, (1.f - pc.tau0[i]), 1.f);
        }
        return dr::clamp(1.f - prod, 0.f, 0.9999f);*/
        return packed_global_tau0(pc, active);
    }

    MI_INLINE Float eval_visibility1_bulk_from_cache(const PackedCache &pc,
                                                     Mask active = true) const {
        Float v1 = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float G1 = G1_HD_0<Float>(pc.tau0[i], pc.wi1[i]);
            v1 *= dr::select(active, G1, 1.f);
        }
        return dr::clamp(v1, 0.f, 1.f);
    }

    MI_INLINE Float eval_visibility2_bulk_from_cache(
        const PackedCache &pc, const Vector3f wo1[NbGrainMax],
        Mask active = true) const {
        Float v2 = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float G2 = G2_HD_0<Float>(pc.tau0[i], pc.wi1[i], wo1[i]);
            v2 *= dr::select(active, G2, 1.f);
        }
        return dr::clamp(v2, 0.f, 1.f);
    }

    MI_INLINE std::pair<Float, Float>
    eval_visibility12_bulk_from_cache(const PackedCache &pc,
                                      const Vector3f wo1[NbGrainMax],
                                      Mask active = true) const {
        Float v1 = 1.f, v2 = 1.f;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float G1 = G1_HD_0<Float>(pc.tau0[i], pc.wi1[i]);
            v1 *= dr::select(active, G1, 1.f);

            Float G2 = G2_HD_0<Float>(pc.tau0[i], pc.wi1[i], wo1[i]);
            v2 *= dr::select(active, G2, 1.f);
        }
        return { dr::clamp(v1, 0.f, 1.f), dr::clamp(v2, 0.f, 1.f) };
    }

    MI_INLINE std::pair<Float, Float>
    eval_global_tau0_and_visibility1_bulk_from_cache(const PackedCache &pc,
                                                     Mask active = true) const {
        Float prod = 1.f;
        Float v1   = 1.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            prod *= dr::select(active, (1.f - pc.tau0[i]), 1.f);

            Float G1 = G1_HD_0<Float>(pc.tau0[i], pc.wi1[i]);
            v1 *= dr::select(active, G1, 1.f);
        }

        Float global_tau0 = dr::clamp(1.f - prod, 0.f, 0.9999f);
        return { global_tau0, dr::clamp(v1, 0.f, 1.f) };
    }

    MI_INLINE std::tuple<Float, Float, Float>
    eval_global_tau0_and_visibility12_bulk_from_cache(
        const PackedCache &pc, const Vector3f wo1[NbGrainMax],
        Mask active = true) const {
        Float prod = 1.f;
        Float v1   = 1.f;
        Float v2   = 1.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            prod *= dr::select(active, (1.f - pc.tau0[i]), 1.f);

            Float G1 = G1_HD_0<Float>(pc.tau0[i], pc.wi1[i]);
            v1 *= dr::select(active, G1, 1.f);

            Float G2 = G2_HD_0<Float>(pc.tau0[i], pc.wi1[i], wo1[i]);
            v2 *= dr::select(active, G2, 1.f);
        }

        Float global_tau0 = dr::clamp(1.f - prod, 0.f, 0.9999f);
        return { global_tau0, dr::clamp(v1, 0.f, 1.f),
                 dr::clamp(v2, 0.f, 1.f) };
    }



    //MI_INLINE Float eval_global_tau_0(const SurfaceInteraction3f &si,
    //                                  Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);
    //    return eval_global_tau_0_from_cache(pc, active);
    //}

    //MI_INLINE Float eval_visibility1_bulk(const SurfaceInteraction3f &si,
    //                                      Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);
    //    return eval_visibility1_bulk_from_cache(pc, active);
    //}

    //MI_INLINE Float eval_visibility2_bulk(const SurfaceInteraction3f &si,
    //                                      const Vector3f &wo,
    //                                      Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);

    //    Vector3f wo1[NbGrainMax];
    //    build_wo1_from_cache(pc, wo, wo1, active);

    //    return eval_visibility2_bulk_from_cache(pc, wo1, active);
    //}

    //MI_INLINE std::pair<Float, Float>
    //eval_visibility12_bulk(const SurfaceInteraction3f &si, const Vector3f &wo,
    //                       Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);

    //    Vector3f wo1[NbGrainMax];
    //    build_wo1_from_cache(pc, wo, wo1, active);

    //    return eval_visibility12_bulk_from_cache(pc, wo1, active);
    //}

    //// combo: global_tau0 + v1
    //MI_INLINE std::pair<Float, Float>
    //eval_global_tau0_and_visibility1_bulk(const SurfaceInteraction3f &si,
    //                                      Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);
    //    return eval_global_tau0_and_visibility1_bulk_from_cache(pc, active);
    //}

    //// combo: global_tau0 + v1 + v2
    //MI_INLINE std::tuple<Float, Float, Float>
    //eval_global_tau0_and_visibility12_bulk(const SurfaceInteraction3f &si,
    //                                       const Vector3f &wo,
    //                                       Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);

    //    Vector3f wo1[NbGrainMax];
    //    build_wo1_from_cache(pc, wo, wo1, active);

    //    return eval_global_tau0_and_visibility12_bulk_from_cache(pc, wo1,
    //                                                             active);
    //}

    //// v2 only after you already have wo (fast path)
    //MI_INLINE Float eval_visibility2_bulk_fast(const SurfaceInteraction3f &si,
    //                                           const Vector3f &wo,
    //                                           Mask active = true) const {
    //    PackedCache pc;
    //    build_packed_cache(si, pc, active);

    //    Vector3f wo1[NbGrainMax];
    //    build_wo1_from_cache(pc, wo, wo1, active);

    //    return eval_visibility2_bulk_from_cache(pc, wo1, active);
    //}

    MI_DECLARE_CLASS(PolyMicrograin)

protected:
    PolyMicrograin(const Properties &props) : Base(props) {
        for (size_t i = 0; i < NbGrainMax; ++i)
            m_micrograin_bsdfs[i] = nullptr;

        m_bsdf_count = 0;
        for (auto &prop : props.objects()) {
            if (auto *bsdf = prop.try_get<MicrograinBSDF<Float, Spectrum>>()) {
                if (m_bsdf_count == NbGrainMax) {
                    Throw("Too many micrograin bsdfs: {} exceeds maximum {}",
                          m_bsdf_count + 1, NbGrainMax);
                }
                m_micrograin_bsdfs[m_bsdf_count++] = bsdf;
            }
        }

        if (m_bsdf_count == 0)
            Throw("PolyMicrograin: At least one MicrograinBSDF must be "
                  "specified.");

        this->m_components.clear();
        this->m_flags = 0;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            this->m_flags |= m_micrograin_bsdfs[i]->flags();
            for (size_t j = 0; j < m_micrograin_bsdfs[i]->component_count();
                 ++j)
                this->m_components.push_back(m_micrograin_bsdfs[i]->flags(j));
        }
    }

protected:
    ref<MicrograinBSDF<Float, Spectrum>> m_micrograin_bsdfs[NbGrainMax];
    size_t m_bsdf_count;

    MI_TRAVERSE_CB(Base, m_bsdf_count, m_micrograin_bsdfs[0],
                   m_micrograin_bsdfs[1], m_micrograin_bsdfs[2],
                   m_micrograin_bsdfs[3], m_micrograin_bsdfs[4],
                   m_micrograin_bsdfs[5], m_micrograin_bsdfs[6],
                   m_micrograin_bsdfs[7], m_micrograin_bsdfs[8],
                   m_micrograin_bsdfs[9], m_micrograin_bsdfs[10],
                   m_micrograin_bsdfs[11], m_micrograin_bsdfs[12],
                   m_micrograin_bsdfs[13], m_micrograin_bsdfs[14],
                   m_micrograin_bsdfs[15])
};


NAMESPACE_END(mitsuba)