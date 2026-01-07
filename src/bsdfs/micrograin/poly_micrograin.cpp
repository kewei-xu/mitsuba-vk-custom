#include <mitsuba/core/fwd.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/core/properties.h>
#include <mitsuba/core/string.h>
#include <mitsuba/core/warp.h>
#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/texture.h>

#include "micrograin.h"

NAMESPACE_BEGIN(mitsuba)

template <typename Float, typename Spectrum>
class PolyMicrograinBSDF final : public PolyMicrograin<Float, Spectrum> {
public:
    MI_IMPORT_BASE(PolyMicrograin, m_flags, m_components)
    MI_IMPORT_TYPES(Texture)

    using Base                         = PolyMicrograin<Float, Spectrum>;
    static constexpr size_t NbGrainMax = Base::NbGrainMax;
    using PackedCache                  = typename Base::PackedCache;

    PolyMicrograinBSDF(const Properties &props) : Base(props) {
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
                std::string name = "micrograin_bsdf_" + std::to_string(i + 1);
                callback->put(name, m_micrograin_bsdfs[i],
                              ParamFlags::NonDifferentiable);
            }
        }
    }

    std::string to_string() const override {
        std::ostringstream oss;
        oss << "PolyMicrograinBSDF[" << std::endl;
        for (size_t i = 0; i < m_bsdf_count; ++i) {
            oss << "  micrograin_bsdf_" << (i + 1) << " = "
                << string::indent(m_micrograin_bsdfs[i]->to_string()) << ","
                << std::endl;
        }
        oss << "]";
        return oss.str();
    }

    Spectrum eval(const BSDFContext &, const SurfaceInteraction3f &,
                  const Vector3f &, Mask) const override {
        if (!m_has_diffuse_component)
            Throw("eval of pure specular for <pathv1> is under development.");
        Throw(
            "eval not supported for anisotropic rough diffuse; use <pathv2>.");
        return 0.f;
    }

    std::pair<BSDFSample3f, Spectrum> sample(const BSDFContext &,
                                             const SurfaceInteraction3f &,
                                             Float, const Point2f &,
                                             Mask) const override {
        if (!m_has_diffuse_component)
            Throw("sample of pure specular for <pathv1> is under development.");
        Throw("sample not supported for anisotropic rough diffuse; use "
              "<pathv2>.");
        return { dr::zeros<BSDFSample3f>(), 0.f };
    }

    // ================= helper (Packed) =================

    MI_INLINE Float term_lambda_from_packed(const PackedCache &pc,
                                            Mask active = true) const {
        return this->packed_term_lambda(pc, active);
    }

    MI_INLINE Float log_term_lambda_from_packed(const PackedCache &pc,
                                                Mask active = true) const {
        return this->packed_log_term_lambda(pc, active);
    }

    // 你的原 proba_level 保留（无需改）
    MI_INLINE Float proba_level(const Float global_tau_0,
                                const Float term_kappa, const Float term_lambda,
                                const Float h_upper,
                                const Float h_lower) const {
        Float hu2 = h_upper * h_upper;
        Float hl2 = h_lower * h_lower;
        Float v   = term_kappa *
                  (dr::pow(term_lambda, hu2) - dr::pow(term_lambda, hl2)) /
                  global_tau_0;
        return dr::clamp(v, 0.f, 1.f);
    }

    // 用 logs 的 Packed 版本代替你原 proba_level_type(tau0,r,term_lambda)
    MI_INLINE Float proba_level_type_packed(const PackedCache &pc, size_t i,
                                            Float log_term_lambda) const {
        Float log_lambda_i = this->packed_log_lambda_i(pc, i);
        return this->packed_proba_level_type_from_logs(log_lambda_i,
                                                       log_term_lambda);
    }

    // ---- sorting (保持你原版，支持 variadic swap) ----
    template <typename T, typename MaskT>
    MI_INLINE void swap_if(T &x, T &y, const MaskT &mask) const {
        T x0 = x, y0 = y;
        x = dr::select(mask, y0, x0);
        y = dr::select(mask, x0, y0);
    }

    template <typename T, typename... Ts>
    MI_INLINE void sort16_bsdf_by_radius(T (&rr)[NbGrainMax], size_t count,
                                         Mask active,
                                         Ts (&...arrays)[NbGrainMax]) const {
        auto pair_valid = [&](size_t i, size_t j) -> Mask {
            return active & (i < count) & (j < count);
        };
        auto CS = [&](size_t i, size_t j) {
            Mask valid = pair_valid(i, j);
            Mask m     = (rr[i] > rr[j]) & valid;
            swap_if(rr[i], rr[j], m);
            (swap_if(arrays[i], arrays[j], m), ...);
        };

        CS(0, 1);
        CS(2, 3);
        CS(4, 5);
        CS(6, 7);
        CS(8, 9);
        CS(10, 11);
        CS(12, 13);
        CS(14, 15);
        CS(0, 2);
        CS(1, 3);
        CS(4, 6);
        CS(5, 7);
        CS(8, 10);
        CS(9, 11);
        CS(12, 14);
        CS(13, 15);
        CS(1, 2);
        CS(5, 6);
        CS(9, 10);
        CS(13, 14);
        CS(0, 4);
        CS(1, 5);
        CS(2, 6);
        CS(3, 7);
        CS(8, 12);
        CS(9, 13);
        CS(10, 14);
        CS(11, 15);
        CS(0, 2);
        CS(1, 3);
        CS(4, 6);
        CS(5, 7);
        CS(8, 10);
        CS(9, 11);
        CS(12, 14);
        CS(13, 15);
        CS(1, 2);
        CS(3, 4);
        CS(5, 6);
        CS(9, 10);
        CS(11, 12);
        CS(13, 14);
        CS(0, 8);
        CS(1, 9);
        CS(2, 10);
        CS(3, 11);
        CS(4, 12);
        CS(5, 13);
        CS(6, 14);
        CS(7, 15);
        CS(0, 4);
        CS(1, 5);
        CS(2, 6);
        CS(3, 7);
        CS(8, 12);
        CS(9, 13);
        CS(10, 14);
        CS(11, 15);
        CS(0, 2);
        CS(1, 3);
        CS(4, 6);
        CS(5, 7);
        CS(8, 10);
        CS(9, 11);
        CS(12, 14);
        CS(13, 15);
        CS(1, 2);
        CS(3, 4);
        CS(5, 6);
        CS(7, 8);
        CS(9, 10);
        CS(11, 12);
        CS(13, 14);
    }

    // ====================== sample_ex (Packed + manual 2x2)
    // ======================
    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              Float sample1, const Point2f &sample2,
              const Point2f &sample2_extra, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;
        if (!dr::any_or<true>(active))
            return { dr::zeros<BSDFSample3f>(), 0.f };

        // ---- build local packed arrays (then sort once) ----
        PackedCache pc_local;
        Float p_spec[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            pc_local.a[k] = pc_local.b[k] = pc_local.c[k] = pc_local.d[k] = 0.f;
            pc_local.r[k] = pc_local.r2[k] = pc_local.inv_r[k] =
                pc_local.inv_r2[k]         = 0.f;
            pc_local.tau0[k] = pc_local.log_base[k] = 0.f;
            pc_local.abs_det[k] = pc_local.inv_det[k] = 0.f;
            pc_local.wi1[k]                           = Vector3f(0.f);
            p_spec[k]                                 = 0.f;
        }

        for (size_t k = 0; k < m_bsdf_count; ++k) {
            Float a    = m_micrograin_bsdfs[k]->eval_a(si, active);
            Float b    = m_micrograin_bsdfs[k]->eval_b(si, active);
            Float c    = m_micrograin_bsdfs[k]->eval_c(si, active);
            Float d    = m_micrograin_bsdfs[k]->eval_d(si, active);
            Float r    = m_micrograin_bsdfs[k]->eval_radius(si, active);
            Float tau0 = m_micrograin_bsdfs[k]->eval_tau_0(si, active);

            Float det     = a * d - b * c;
            Float inv_det = dr::rcp(det);

            pc_local.a[k] = a;
            pc_local.b[k] = b;
            pc_local.c[k] = c;
            pc_local.d[k] = d;

            pc_local.r[k]      = r;
            pc_local.r2[k]     = r * r;
            pc_local.inv_r[k]  = dr::rcp(r);
            pc_local.inv_r2[k] = dr::rcp(pc_local.r2[k]);

            pc_local.tau0[k]     = tau0;
            pc_local.log_base[k] = dr::log(1.f - tau0);

            pc_local.abs_det[k] = dr::abs(det);
            pc_local.inv_det[k] = inv_det;

            pc_local.wi1[k] =
                dr::normalize(this->mul_Minv(a, b, c, d, inv_det, si.wi));

            p_spec[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        // sort by radius, swap everything derived too
        this->sort16_bsdf_by_radius(
            pc_local.r, m_bsdf_count, active, pc_local.a, pc_local.b,
            pc_local.c, pc_local.d, pc_local.r2, pc_local.inv_r,
            pc_local.inv_r2, pc_local.tau0, pc_local.log_base, pc_local.abs_det,
            pc_local.inv_det, pc_local.wi1, p_spec);

        // global_tau0 (log-domain)
        Float global_tau_0 = this->packed_global_tau0(pc_local, active);

        // --- init per-level term_lambda (log-domain) ---
        Float log_term_lambda_cur =
            this->packed_log_term_lambda(pc_local, active);
        Float term_lambda_cur = dr::exp(log_term_lambda_cur);

        Float term_kappa = 1.f - global_tau_0;

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        // outputs
        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result(0.f);

        // chosen per-lane
        Float ch_a = 0.f, ch_b = 0.f, ch_c = 0.f, ch_d = 0.f;
        Float ch_r = 0.f, ch_tau0 = 0.f, ch_p_spec = 0.f;
        Float ch_abs_det = 0.f, ch_inv_det = 0.f;
        Float ch_h_lower = 0.f, ch_h_upper = 0.f;
        Float ch_term_lambda  = 1.f;
        Float ch_sample1_2    = 0.f;
        Mask ch_selected      = false;
        Mask ch_spec_selected = false;

        Float p_level_cum = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            h_upper = pc_local.r[i];

            // IMPORTANT: p_level uses CURRENT term_lambda (same as original
            // logic)
            Float p_level = this->proba_level(
                global_tau_0, term_kappa, term_lambda_cur, h_upper, h_lower);
            Float p_level_cum_pre = p_level_cum;
            p_level_cum += p_level;

            Mask level_selected =
                (sample1 >= p_level_cum_pre) & (sample1 < p_level_cum);

            Float sample1_1 = (sample1 - p_level_cum_pre) / p_level;

            Float p_type_cum = 0.f;

            for (size_t j = i; j < m_bsdf_count; ++j) {
                // IMPORTANT: p_type must use CURRENT log_term_lambda (per
                // level)
                Float p_type = this->proba_level_type_packed(
                    pc_local, j, log_term_lambda_cur);

                Float p_type_cum_pre = p_type_cum;
                p_type_cum += p_type;

                Mask type_selected = level_selected &
                                     (sample1_1 >= p_type_cum_pre) &
                                     (sample1_1 < p_type_cum);

                Float sample1_2 = (sample1_1 - p_type_cum_pre) / p_type;

                Mask spec_selected = type_selected & (sample1_2 < p_spec[j]);

                dr::masked(ch_a, type_selected) = pc_local.a[j];
                dr::masked(ch_b, type_selected) = pc_local.b[j];
                dr::masked(ch_c, type_selected) = pc_local.c[j];
                dr::masked(ch_d, type_selected) = pc_local.d[j];

                dr::masked(ch_r, type_selected)       = pc_local.r[j];
                dr::masked(ch_tau0, type_selected)    = pc_local.tau0[j];
                dr::masked(ch_abs_det, type_selected) = pc_local.abs_det[j];
                dr::masked(ch_inv_det, type_selected) = pc_local.inv_det[j];
                dr::masked(ch_p_spec, type_selected)  = p_spec[j];

                dr::masked(ch_h_lower, type_selected) = h_lower;
                dr::masked(ch_h_upper, type_selected) = h_upper;

                // IMPORTANT: store CURRENT level's term_lambda for conditional
                // sampling
                dr::masked(ch_term_lambda, type_selected)   = term_lambda_cur;
                dr::masked(ch_sample1_2, type_selected)     = sample1_2;
                dr::masked(ch_spec_selected, type_selected) = spec_selected;

                ch_selected |= type_selected;
            }

            h_lower = h_upper;

            // --- update for next level (log recursion) ---
            Float cur_term_kappa = 1.f - pc_local.tau0[i];
            term_kappa /= cur_term_kappa;

            // log_term_lambda_cur -= log_lambda_i  (instead of term_lambda /=
            // exp(log_lambda_i))
            Float log_lambda_i = this->packed_log_lambda_i(pc_local, i);
            log_term_lambda_cur -= log_lambda_i;
            term_lambda_cur = dr::exp(log_term_lambda_cur);
        }

        Mask ok = active & ch_selected;
        if (!dr::any_or<true>(ok))
            return { bs, 0.f };

        // sample wh_1
        Vector3f wh_1 = square_to_sphere_micrograin_conditional_level_and_type(
            ch_term_lambda, ch_r, ch_h_upper, ch_h_lower, sample2);

        // wh = normalize((M^{-1})^T * wh_1)  --- manual 2x2
        Normal3f wh = dr::normalize(
            this->mul_MinvT(ch_a, ch_b, ch_c, ch_d, ch_inv_det, wh_1));

        Vector3f wo = dr::select(ch_spec_selected, reflect(si.wi, wh),
                                 warp::square_to_cosine_hemisphere(sample2));

        Mask active_sel = ok & (Frame3f::cos_theta(wo) > 0.f);
        if (!dr::any_or<true>(active_sel))
            return { bs, 0.f };

        Float pdf_ = this->pdf(ctx, si, wo, active_sel);

        BSDFSample3f bs_tmp(wo);
        bs_tmp.pdf = pdf_;
        bs_tmp.sampled_type =
            dr::select(ch_spec_selected, +BSDFFlags::GlossyReflection,
                       +BSDFFlags::DiffuseReflection);

        dr::masked(bs, ch_selected) = bs_tmp;

        Mask pdf_valid = dr::neq(pdf_, 0.f);
        Spectrum val   = this->eval_ex(ctx, si, wo, sample2_extra, active_sel);
        Spectrum contrib = dr::select(active_sel & pdf_valid, val / pdf_, 0.f);
        dr::masked(result, ch_selected) = contrib;

        return { bs, result };
    }

    // ====================== pdf (Packed + manual 2x2) ======================
    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        active &= (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        if (!dr::any_or<true>(active))
            return 0.f;

        // local packed + p_spec then sort
        PackedCache pc_local;
        Float p_spec[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            pc_local.a[k] = pc_local.b[k] = pc_local.c[k] = pc_local.d[k] = 0.f;
            pc_local.r[k] = pc_local.r2[k] = pc_local.inv_r[k] =
                pc_local.inv_r2[k]         = 0.f;
            pc_local.tau0[k] = pc_local.log_base[k] = 0.f;
            pc_local.abs_det[k] = pc_local.inv_det[k] = 0.f;
            pc_local.wi1[k]                           = Vector3f(0.f);
            p_spec[k]                                 = 0.f;
        }

        for (size_t k = 0; k < m_bsdf_count; ++k) {
            Float a    = m_micrograin_bsdfs[k]->eval_a(si, active);
            Float b    = m_micrograin_bsdfs[k]->eval_b(si, active);
            Float c    = m_micrograin_bsdfs[k]->eval_c(si, active);
            Float d    = m_micrograin_bsdfs[k]->eval_d(si, active);
            Float r    = m_micrograin_bsdfs[k]->eval_radius(si, active);
            Float tau0 = m_micrograin_bsdfs[k]->eval_tau_0(si, active);

            Float det     = a * d - b * c;
            Float inv_det = dr::rcp(det);

            pc_local.a[k] = a;
            pc_local.b[k] = b;
            pc_local.c[k] = c;
            pc_local.d[k] = d;

            pc_local.r[k]      = r;
            pc_local.r2[k]     = r * r;
            pc_local.inv_r[k]  = dr::rcp(r);
            pc_local.inv_r2[k] = dr::rcp(pc_local.r2[k]);

            pc_local.tau0[k]     = tau0;
            pc_local.log_base[k] = dr::log(1.f - tau0);

            pc_local.abs_det[k] = dr::abs(det);
            pc_local.inv_det[k] = inv_det;

            pc_local.wi1[k] =
                dr::normalize(this->mul_Minv(a, b, c, d, inv_det, si.wi));

            p_spec[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        this->sort16_bsdf_by_radius(
            pc_local.r, m_bsdf_count, active, pc_local.a, pc_local.b,
            pc_local.c, pc_local.d, pc_local.r2, pc_local.inv_r,
            pc_local.inv_r2, pc_local.tau0, pc_local.log_base, pc_local.abs_det,
            pc_local.inv_det, pc_local.wi1, p_spec);

        Float global_tau_0 = this->packed_global_tau0(pc_local, active);

        // --- init per-level term_lambda (log-domain) ---
        Float log_term_lambda_cur =
            this->packed_log_term_lambda(pc_local, active);
        Float term_lambda_cur = dr::exp(log_term_lambda_cur);

        Float term_kappa = 1.f - global_tau_0;

        Vector3f wh     = dr::normalize(si.wi + wo);
        Mask valid_spec = (dr::dot(si.wi, wh) > 0.f) & (dr::dot(wo, wh) > 0.f);

        // per-j precompute using manual 2x2: tmp = M^T * wh
        Vector3f tmp[NbGrainMax];
        Float norm_sqr[NbGrainMax];
        Float hv_height[NbGrainMax];

        for (size_t j = 0; j < m_bsdf_count; ++j) {
            tmp[j] = this->mul_Mt(pc_local.a[j], pc_local.b[j], pc_local.c[j],
                                  pc_local.d[j], wh);
            norm_sqr[j]   = dr::squared_norm(tmp[j]);
            Vector3f wh_1 = dr::normalize(tmp[j]);
            hv_height[j]  = Frame3f::cos_theta(wh_1) * pc_local.r[j];
        }

        Float pdf_ = 0.f;

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            h_upper = pc_local.r[i];

            // IMPORTANT: p_level uses CURRENT term_lambda
            Float p_level = this->proba_level(
                global_tau_0, term_kappa, term_lambda_cur, h_upper, h_lower);
            Mask valid_level = dr::neq(h_upper - h_lower, 0.f);

            for (size_t j = i; j < m_bsdf_count; ++j) {
                // IMPORTANT: p_type uses CURRENT log_term_lambda
                Float p_type = this->proba_level_type_packed(
                    pc_local, j, log_term_lambda_cur);

                Mask inRange =
                    (hv_height[j] > h_lower) & (hv_height[j] <= h_upper);

                Float pdf_spec =
                    this->square_to_micrograin_conditional_level_and_type_pdf(
                        term_lambda_cur, pc_local.r[j], h_upper, h_lower,
                        pc_local.abs_det[j], norm_sqr[j], wh) /
                    (4.f * dr::dot(wo, wh));

                pdf_ +=
                    dr::select(active & valid_level & inRange & valid_spec,
                               p_level * p_type * p_spec[j] * pdf_spec, 0.f);

                Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);
                pdf_ += dr::select(
                    active & valid_level,
                    p_level * p_type * (1.f - p_spec[j]) * pdf_diff, 0.f);
            }

            h_lower = h_upper;

            // --- update recursion ---
            Float cur_term_kappa = 1.f - pc_local.tau0[i];
            term_kappa /= cur_term_kappa;

            Float log_lambda_i = this->packed_log_lambda_i(pc_local, i);
            log_term_lambda_cur -= log_lambda_i;
            term_lambda_cur = dr::exp(log_term_lambda_cur);
        }

        return pdf_;
    }

    // ====================== eval_ex (Packed + log shared_product + manual 2x2)
    // ======================
    Spectrum eval_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
                     const Vector3f &wo, const Point2f &sample2_extra,
                     Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        active &= (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        if (!dr::any_or<true>(active))
            return 0.f;

        // packed once (原始顺序即可；eval_ex 不需要按 r 排序)
        PackedCache pc;
        this->cache_packed(si, pc, active);

        Float global_tau_0 = this->packed_global_tau0(pc, active);

        Vector3f h     = dr::normalize(si.wi + wo);
        Mask G_local_h = (dr::dot(si.wi, h) > 0.f) & (dr::dot(wo, h) > 0.f);

        Vector3f wo1[NbGrainMax];
        this->cache_wo1_from_packed(pc, wo, wo1, active);

        // shared_product: Σ log(1-tau0_k) * exponent_k, then exp
        auto shared_product_log = [&](const Float expo_numer,
                                      Mask act) -> Float {
            Float logv = 0.f;
            for (size_t k = 0; k < m_bsdf_count; ++k) {
                Float expo_coeff = expo_numer * pc.inv_r2[k];
                Float exponent   = dr::maximum(1.f - expo_coeff, 0.f);
                logv += dr::select(act, pc.log_base[k] * exponent, 0.f);
            }
            return dr::exp(logv);
        };

        auto g_dist_from_height = [&](const Float height, Mask act) -> Float {
            Float value = 1.f;
            for (size_t k = 0; k < m_bsdf_count; ++k) {
                Float h_m_1 = height * pc.inv_r[k];
                Float g2_dist =
                    G2_HD<Float>(pc.tau0[k], pc.wi1[k], wo1[k], h_m_1);
                value *= dr::select(act, g2_dist, 1.f);
            }
            return value;
        };

        Spectrum value(0.f);

        for (size_t i = 0; i < m_bsdf_count; ++i) {

            // ===== specular =====
            Vector3f h_tmp =
                this->mul_Mt(pc.a[i], pc.b[i], pc.c[i], pc.d[i], h);
            Vector3f h_1        = dr::normalize(h_tmp);
            Float norm_sqr      = dr::squared_norm(h_tmp);
            Float cos_theta_h_1 = Frame3f::cos_theta(h_1);

            Float coeff      = pc.abs_det[i] / (norm_sqr * norm_sqr);
            Float expo_numer = pc.r2[i] * cos_theta_h_1 * cos_theta_h_1;

            Float shared_prod =
                shared_product_log(expo_numer, active & G_local_h);

            Float D_type_normal_joint = coeff * pc.log_base[i] * shared_prod *
                                        -dr::InvPi<Float> / global_tau_0;

            Float height = cos_theta_h_1 * pc.r[i];
            Float G_dist = g_dist_from_height(height, active & G_local_h);

            Spectrum F = m_micrograin_bsdfs[i]->eval_fresnel(
                ctx, si, h, active & G_local_h);

            Spectrum brdf_spec =
                D_type_normal_joint * F * G_dist / (4.f * cos_theta_i);
            value += dr::select(active & G_local_h, brdf_spec, 0.f);

            // ===== diffuse =====
            Vector3f m_1 =
                square_to_sphere_micrograin<Float>(pc.tau0[i], sample2_extra);
            Vector3f m = dr::normalize(this->mul_MinvT(
                pc.a[i], pc.b[i], pc.c[i], pc.d[i], pc.inv_det[i], m_1));

            Float cos_theta_m_1 = Frame3f::cos_theta(m_1);
            Mask G_local_m = (dr::dot(si.wi, m) > 0.f) & (dr::dot(wo, m) > 0.f);

            Vector3f mtm = this->mul_Mt(pc.a[i], pc.b[i], pc.c[i], pc.d[i], m);
            Float norm_sqr_m = dr::squared_norm(mtm);
            Float coeff_m    = pc.abs_det[i] / (norm_sqr_m * norm_sqr_m);
            Float D_         = coeff_m * NDF_1<Float>(pc.tau0[i], m_1);
            Float pdf_m      = D_ * Frame3f::cos_theta(m);
            Mask valid_normal_sample = dr::neq(pdf_m, 0.f);

            Float expo_numer_m = pc.r2[i] * cos_theta_m_1 * cos_theta_m_1;
            Float shared_prod_m =
                shared_product_log(expo_numer_m, active & G_local_m);

            Float D_type_normal_joint_m = coeff_m * pc.log_base[i] *
                                          shared_prod_m * -dr::InvPi<Float> /
                                          global_tau_0;

            Float height_m = Frame3f::cos_theta(m_1) * pc.r[i];
            Float G_dist_m = g_dist_from_height(height_m, active & G_local_m);

            Spectrum refl = m_micrograin_bsdfs[i]->eval_weighted_albedo(
                si, wo, m, active & G_local_m & valid_normal_sample);

            Spectrum brdf_diff =
                refl * D_type_normal_joint_m * G_dist_m / pdf_m / cos_theta_i;

            value += dr::select(active & G_local_m & valid_normal_sample,
                                brdf_diff, 0.f);
        }

        return value;
    }

    // ================== 原有 spherical sampling & pdf helper (不用动矩阵)
    // ==================

    MI_INLINE Vector3f square_to_sphere_micrograin_conditional_level_and_type(
        const Float term_lambda, const Float r, const Float h_upper,
        const Float h_lower, const Point2f &sample2) const {

        Float s1 = sample2.x();
        Float s2 = sample2.y();

        Float phi   = dr::TwoPi<Float> * s1;
        Float theta = dr::acos(dr::sqrt(
            dr::log((1.f - s2) * dr::pow(term_lambda, h_upper * h_upper) +
                    s2 * dr::pow(term_lambda, h_lower * h_lower)) /
            (r * r) / dr::log(term_lambda)));

        return Vector3f(dr::sin(theta) * dr::cos(phi),
                        dr::sin(theta) * dr::sin(phi), dr::cos(theta));
    }

    MI_INLINE Float square_to_micrograin_conditional_level_and_type_pdf(
        const Float term_lambda, const Float r, const Float h_upper,
        const Float h_lower, const Float abs_det_M, const Float norm_sqr,
        const Vector3f &m) const {

        Float cos_theta_m = Frame3f::cos_theta(m);
        Float coeff       = abs_det_M / (norm_sqr * norm_sqr);
        Float r2          = r * r;

        Float numer =
            r2 * dr::log(term_lambda) *
            dr::pow(term_lambda, r2 * cos_theta_m * cos_theta_m / norm_sqr);

        Float denom = dr::pow(term_lambda, h_upper * h_upper) -
                      dr::pow(term_lambda, h_lower * h_lower);

        return dr::maximum(
            coeff * dr::InvPi<Float> * cos_theta_m * numer / denom, 0.f);
    }

    MI_DECLARE_CLASS(PolyMicrograinBSDF)

private:
    bool m_has_diffuse_component;
    MI_TRAVERSE_CB(Base, m_has_diffuse_component)
};

MI_EXPORT_PLUGIN(PolyMicrograinBSDF)
NAMESPACE_END(mitsuba)
