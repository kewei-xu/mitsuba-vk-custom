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
    MI_IMPORT_BASE(PolyMicrograin, m_flags, m_components, m_micrograin_bsdfs, m_bsdf_count)
    MI_IMPORT_TYPES(Texture)

    //using Base                         = PolyMicrograin<Float, Spectrum>;
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


    //MI_INLINE Vector3f wo_trans_via_normal_m(const Vector3f& wo, 
    //                                         const Vector3f& wm,
    //                                         Mask active=true) const {
    //    Vector3f wx(0.f); 
    //    Vector3f wy(0.f);
    //    Mask valid = wm.z() >= -0.999999f;
    //    if (dr::any_or<true>(valid)) {
    //        Float a = 1.f / (1.f + wm.z());
    //        Float b = -wm.x() * wm.y() * a;
    //        dr::masked(wx, valid) = Vector3f(1.f - wm.x() * wm.x() * a, b, -wm.x());
    //        dr::masked(wy, valid) = Vector3f(b, 1.f - wm.y() * wm.y() * a, -wm.y());
    //    } 
    //    if (dr::any_or<true>(~valid)){
    //        dr::masked(wx, ~valid) = Vector3f(0.f, -1.f, 0.f);
    //        dr::masked(wy, ~valid) = Vector3f(-1.f, 0.f, 0.f);
    //    }
    //    
    //    return dr::select(active, wx * wo.x() + wy * wo.y() + wm * wo.z(), wo);  
    //}

    MI_INLINE Float term_lambda_from_packed(const PackedCache &pc,
                                            Mask active = true) const {
        return this->packed_term_lambda(pc, active);
    }

    MI_INLINE Float log_term_lambda_from_packed(const PackedCache &pc,
                                                Mask active = true) const {
        return this->packed_log_term_lambda(pc, active);
    }

    //  proba_level 
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

    // logs version of proba_level_type(tau0,r,term_lambda)
    MI_INLINE Float proba_level_type_packed(const PackedCache &pc, size_t i,
                                            Float log_term_lambda) const {
        Float log_lambda_i = this->packed_log_lambda_i(pc, i);
        return this->packed_proba_level_type_from_logs(log_lambda_i,
                                                       log_term_lambda);
    }

    // ---- sorting helpers (index-only) ----
    template <typename T, typename MaskT>
    MI_INLINE void swap_if(T &x, T &y, const MaskT &mask) const {
        T x0 = x, y0 = y;
        x = dr::select(mask, y0, x0);
        y = dr::select(mask, x0, y0);
    }

    // Sort network: only swaps r_key[] and idx[] (NOT swapping all packed
    // arrays)
    MI_INLINE void sort16_index_by_radius(Float (&r_key)[NbGrainMax],
                                          UInt32 (&idx)[NbGrainMax],
                                          size_t count, Mask active) const {
        auto pair_valid = [&](size_t i, size_t j) -> Mask {
            return active & (i < count) & (j < count);
        };

        auto CS = [&](size_t i, size_t j) {
            Mask valid = pair_valid(i, j);
            Mask m     = (r_key[i] > r_key[j]) & valid;
            swap_if(r_key[i], r_key[j], m);
            swap_if(idx[i], idx[j], m);
        };

        // 16-input sorting network 
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


    // ---- gather helpers (idx -> value), Scheme: tree gather (4-stage) ----

    // Use 4 bit-masks (b0..b3) to avoid recomputing comparisons per field.
    // b0 = (id & 1)!=0, b1 = (id & 2)!=0, b2 = (id & 4)!=0, b3 = (id & 8)!=0
    template <typename T>
    MI_INLINE T gather16_by_bits(const T (&arr)[NbGrainMax], const UInt32 &id,
                                 const Mask &valid_k, const Mask &b0,
                                 const Mask &b1, const Mask &b2,
                                 const Mask &b3) const {
        // Stage 0: 8 pairs
        T p0 = dr::select(b0, arr[1], arr[0]);
        T p1 = dr::select(b0, arr[3], arr[2]);
        T p2 = dr::select(b0, arr[5], arr[4]);
        T p3 = dr::select(b0, arr[7], arr[6]);
        T p4 = dr::select(b0, arr[9], arr[8]);
        T p5 = dr::select(b0, arr[11], arr[10]);
        T p6 = dr::select(b0, arr[13], arr[12]);
        T p7 = dr::select(b0, arr[15], arr[14]);

        // Stage 1: 4 quads
        T q0 = dr::select(b1, p1, p0);
        T q1 = dr::select(b1, p3, p2);
        T q2 = dr::select(b1, p5, p4);
        T q3 = dr::select(b1, p7, p6);

        // Stage 2: 2 octets
        T o0 = dr::select(b2, q1, q0);
        T o1 = dr::select(b2, q3, q2);

        // Stage 3: final
        T out = dr::select(b3, o1, o0);

        // Gate invalid lanes to zero (important!)
        return dr::select(valid_k, out, dr::zeros<T>());
    }

    // Backward-compatible wrapper (computes bit masks internally)
    template <typename T>
    MI_INLINE T gather16_by_idx(const T (&arr)[NbGrainMax], const UInt32 &id,
                                Mask active) const {
        Mask valid_k = active;
        Mask b0      = dr::neq(id & (UInt32) 1u, (UInt32) 0u);
        Mask b1      = dr::neq(id & (UInt32) 2u, (UInt32) 0u);
        Mask b2      = dr::neq(id & (UInt32) 4u, (UInt32) 0u);
        Mask b3      = dr::neq(id & (UInt32) 8u, (UInt32) 0u);
        return gather16_by_bits(arr, id, valid_k, b0, b1, b2, b3);
    }

    // Materialize sorted PackedCache + sorted p_spec from (pc_local,
    // p_spec_local, idx[])
    MI_INLINE void materialize_sorted_from_index(
        const PackedCache &pc_local, const Float (&p_spec_local)[NbGrainMax],
        const UInt32 (&idx)[NbGrainMax], PackedCache &pc_sorted,
        Float (&p_spec_sorted)[NbGrainMax], size_t count, Mask active) const {

        // Optional: zero-init outputs once (keeps behavior deterministic)
        for (size_t k = 0; k < NbGrainMax; ++k) {
            pc_sorted.a[k] = pc_sorted.b[k] = pc_sorted.c[k] = pc_sorted.d[k] =
                0.f;
            pc_sorted.r[k] = pc_sorted.r2[k] = pc_sorted.inv_r[k] =
                pc_sorted.inv_r2[k]          = 0.f;
            pc_sorted.tau0[k] = pc_sorted.log_base[k] = 0.f;
            pc_sorted.abs_det[k] = pc_sorted.inv_det[k] = 0.f;
            pc_sorted.wi1[k]                            = Vector3f(0.f);
            p_spec_sorted[k]                            = 0.f;
        }

        for (size_t k = 0; k < NbGrainMax; ++k) {
            UInt32 id = idx[k];

            // Only lanes with id < count are meaningful
            Mask valid_k = active & (id < (UInt32) count);

            // Compute bit masks ONCE per k (reused for all fields)
            Mask b0 = dr::neq(id & (UInt32) 1u, (UInt32) 0u);
            Mask b1 = dr::neq(id & (UInt32) 2u, (UInt32) 0u);
            Mask b2 = dr::neq(id & (UInt32) 4u, (UInt32) 0u);
            Mask b3 = dr::neq(id & (UInt32) 8u, (UInt32) 0u);

            pc_sorted.a[k] =
                gather16_by_bits(pc_local.a, id, valid_k, b0, b1, b2, b3);
            pc_sorted.b[k] =
                gather16_by_bits(pc_local.b, id, valid_k, b0, b1, b2, b3);
            pc_sorted.c[k] =
                gather16_by_bits(pc_local.c, id, valid_k, b0, b1, b2, b3);
            pc_sorted.d[k] =
                gather16_by_bits(pc_local.d, id, valid_k, b0, b1, b2, b3);

            pc_sorted.r[k] =
                gather16_by_bits(pc_local.r, id, valid_k, b0, b1, b2, b3);
            pc_sorted.r2[k] =
                gather16_by_bits(pc_local.r2, id, valid_k, b0, b1, b2, b3);
            pc_sorted.inv_r[k] =
                gather16_by_bits(pc_local.inv_r, id, valid_k, b0, b1, b2, b3);
            pc_sorted.inv_r2[k] =
                gather16_by_bits(pc_local.inv_r2, id, valid_k, b0, b1, b2, b3);

            pc_sorted.tau0[k] =
                gather16_by_bits(pc_local.tau0, id, valid_k, b0, b1, b2, b3);
            pc_sorted.log_base[k] = gather16_by_bits(pc_local.log_base, id,
                                                     valid_k, b0, b1, b2, b3);

            pc_sorted.abs_det[k] =
                gather16_by_bits(pc_local.abs_det, id, valid_k, b0, b1, b2, b3);
            pc_sorted.inv_det[k] =
                gather16_by_bits(pc_local.inv_det, id, valid_k, b0, b1, b2, b3);

            pc_sorted.wi1[k] =
                gather16_by_bits(pc_local.wi1, id, valid_k, b0, b1, b2, b3);

            p_spec_sorted[k] =
                gather16_by_bits(p_spec_local, id, valid_k, b0, b1, b2, b3);
        }
    }



    // ---- Scheme D: suffix tables for log_lambda / log_term_lambda /
    // term_lambda ----
    // log_lambda[j] = log((1 - tau0_j)^(-1/r_j^2)) = -(log_base_j)/r_j^2
    // log_term_lambda[i] = Σ_{k=i..n-1} log_lambda[k]
    // term_lambda[i] = exp(log_term_lambda[i])
    MI_INLINE void build_suffix_tables_lambda(
        const PackedCache &pc, Float (&log_lambda)[NbGrainMax],
        Float (&log_term_lambda)[NbGrainMax], Float (&term_lambda)[NbGrainMax],
        size_t count, Mask active) const {

        for (size_t k = 0; k < NbGrainMax; ++k) {
            log_lambda[k]      = 0.f;
            log_term_lambda[k] = 0.f;
            term_lambda[k]     = 1.f;
        }

        // per-type log_lambda
        for (size_t j = 0; j < count; ++j) {
            // exactly same as your packed_log_lambda_i(pc, j)
            log_lambda[j] = -pc.log_base[j] * pc.inv_r2[j];
            (void) active; // keep signature consistent; active used by caller
                           // masks
        }

        // suffix sum
        Float acc = 0.f;
        for (int jj = (int) count - 1; jj >= 0; --jj) {
            acc += log_lambda[(size_t) jj];
            log_term_lambda[(size_t) jj] = acc;
            term_lambda[(size_t) jj]     = dr::exp(acc);
        }
    }


    std::pair<BSDFSample3f, Spectrum>
    sample_ex(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              Float sample1, const Point2f &sample2,
              const Point2f &sample2_extra, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        active &= cos_theta_i > 0.f;
        if (!dr::any_or<true>(active))
            return { dr::zeros<BSDFSample3f>(), 0.f };

        // ---- build local packed arrays (UNSORTED) ----
        PackedCache pc_local;
        Float p_spec_local[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            pc_local.a[k] = pc_local.b[k] = pc_local.c[k] = pc_local.d[k] = 0.f;
            pc_local.r[k] = pc_local.r2[k] = pc_local.inv_r[k] =
                pc_local.inv_r2[k]         = 0.f;
            pc_local.tau0[k] = pc_local.log_base[k] = 0.f;
            pc_local.abs_det[k] = pc_local.inv_det[k] = 0.f;
            pc_local.wi1[k]                           = Vector3f(0.f);
            p_spec_local[k]                           = 0.f;
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

            p_spec_local[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        // ---- index-only sort by radius ----
        UInt32 idx[NbGrainMax];
        Float r_key[NbGrainMax];
        for (size_t k = 0; k < NbGrainMax; ++k) {
            idx[k]   = (UInt32) k;
            r_key[k] = pc_local.r[k];
        }

        this->sort16_index_by_radius(r_key, idx, m_bsdf_count, active);

        // ---- materialize sorted cache once ----
        PackedCache pc;
        Float p_spec[NbGrainMax];
        materialize_sorted_from_index(pc_local, p_spec_local, idx, pc, p_spec,
                                      m_bsdf_count, active);

        // global_tau0
        Float global_tau_0 = this->packed_global_tau0(pc, active);

        // ===== Scheme D: suffix tables for lambda =====
        Float log_lambda[NbGrainMax];
        Float log_term_lambda_tab[NbGrainMax];
        Float term_lambda_tab[NbGrainMax];

        build_suffix_tables_lambda(pc, log_lambda, log_term_lambda_tab,
                                   term_lambda_tab, m_bsdf_count, active);

        Float term_kappa = 1.f - global_tau_0;

        Float h_lower = 0.f;
        Float h_upper = 0.f;

        BSDFSample3f bs = dr::zeros<BSDFSample3f>();
        Spectrum result(0.f);

        // chosen per-lane
        Float ch_tau0 = 0.f;
        Float ch_a = 0.f, ch_b = 0.f, ch_c = 0.f, ch_d = 0.f;
        Float ch_r = 0.f, ch_p_spec = 0.f;
        Float ch_inv_det = 0.f;
        Float ch_h_lower = 0.f, ch_h_upper = 0.f;
        Float ch_term_lambda  = 1.f;
        Float ch_sample1_2    = 0.f;
        Mask ch_selected      = false;
        Mask ch_spec_selected = false;

        Float p_level_cum = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            h_upper = pc.r[i];

            // use precomputed term_lambda(i)
            Float term_lambda_i     = term_lambda_tab[i];
            Float log_term_lambda_i = log_term_lambda_tab[i];

            Float p_level = this->proba_level(global_tau_0, term_kappa,
                                              term_lambda_i, h_upper, h_lower);

            Float p_level_cum_pre = p_level_cum;
            p_level_cum += p_level;

            Mask level_selected =
                (sample1 >= p_level_cum_pre) & (sample1 < p_level_cum);

            Float sample1_1 = (sample1 - p_level_cum_pre) / p_level;

            Float p_type_cum = 0.f;

            // safe denom only affects degenerate log_term_lambda==0 case
            Float denom = dr::maximum(log_term_lambda_i, 1e-8f);

            for (size_t j = i; j < m_bsdf_count; ++j) {
                // p_type == log_lambda[j] / log_term_lambda(i)  (exactly
                // equivalent)
                Float p_type = dr::clamp(log_lambda[j] / denom, 0.f, 1.f);

                Float p_type_cum_pre = p_type_cum;
                p_type_cum += p_type;

                Mask type_selected = level_selected &
                                     (sample1_1 >= p_type_cum_pre) &
                                     (sample1_1 < p_type_cum);

                Float sample1_2 = (sample1_1 - p_type_cum_pre) / p_type;

                Mask spec_selected = type_selected & (sample1_2 < p_spec[j]);

                dr::masked(ch_tau0, type_selected) = pc.tau0[j];
                dr::masked(ch_a, type_selected) = pc.a[j];
                dr::masked(ch_b, type_selected) = pc.b[j];
                dr::masked(ch_c, type_selected) = pc.c[j];
                dr::masked(ch_d, type_selected) = pc.d[j];

                dr::masked(ch_r, type_selected)       = pc.r[j];
                dr::masked(ch_inv_det, type_selected) = pc.inv_det[j];
                dr::masked(ch_p_spec, type_selected)  = p_spec[j];

                dr::masked(ch_h_lower, type_selected) = h_lower;
                dr::masked(ch_h_upper, type_selected) = h_upper;

                dr::masked(ch_term_lambda, type_selected)   = term_lambda_i;
                dr::masked(ch_sample1_2, type_selected)     = sample1_2;
                dr::masked(ch_spec_selected, type_selected) = spec_selected;

                ch_selected |= type_selected;
            }

            h_lower = h_upper;

            // only kappa recurses (same as your original)
            Float cur_term_kappa = 1.f - pc.tau0[i];
            term_kappa /= cur_term_kappa;
        }

        Mask no_issue = active & ch_selected;
        if (!dr::any_or<true>(no_issue))
            return { bs, 0.f };

        Vector3f wh_1 = square_to_sphere_micrograin_conditional_level_and_type(
            ch_term_lambda, ch_r, ch_h_upper, ch_h_lower, sample2);

        Normal3f wh = dr::normalize(
            this->mul_MinvT(ch_a, ch_b, ch_c, ch_d, ch_inv_det, wh_1));

        //wo reframe
        // sample normal 
        /*Vector3f m_1 = square_to_sphere_micrograin<Float>(ch_tau0, sample2_extra);
        Vector3f m = dr::normalize(this->mul_MinvT(ch_a, ch_b, ch_c, ch_d, ch_inv_det, m_1));

        Vector3f wo = dr::select(ch_spec_selected, reflect(si.wi, wh),
                                 wo_trans_via_normal_m(warp::square_to_cosine_hemisphere(sample2), m));*/
        // wo without reframed (cause base on NDF not VNDF)
        Vector3f wo =
            dr::select(ch_spec_selected, reflect(si.wi, wh),
                           warp::square_to_cosine_hemisphere(sample2));


        Mask active_sel = no_issue & (Frame3f::cos_theta(wo) > 0.f);
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

        /*in case!*/
        Mask accident_NaN = dr::any(dr::isnan(result));
        return { bs, result & !accident_NaN };
    }


    // ====================== pdf (Packed + manual 2x2) ======================
    Float pdf(const BSDFContext &ctx, const SurfaceInteraction3f &si,
              const Vector3f &wo, Mask active) const override {

        Float cos_theta_i = Frame3f::cos_theta(si.wi);
        Float cos_theta_o = Frame3f::cos_theta(wo);

        active &= (cos_theta_i > 0.f) & (cos_theta_o > 0.f);
        if (!dr::any_or<true>(active))
            return 0.f;

        // ---- build local packed arrays (UNSORTED) ----
        PackedCache pc_local;
        Float p_spec_local[NbGrainMax];

        for (size_t k = 0; k < NbGrainMax; ++k) {
            pc_local.a[k] = pc_local.b[k] = pc_local.c[k] = pc_local.d[k] = 0.f;
            pc_local.r[k] = pc_local.r2[k] = pc_local.inv_r[k] =
                pc_local.inv_r2[k]         = 0.f;
            pc_local.tau0[k] = pc_local.log_base[k] = 0.f;
            pc_local.abs_det[k] = pc_local.inv_det[k] = 0.f;
            pc_local.wi1[k]                           = Vector3f(0.f);
            p_spec_local[k]                           = 0.f;
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

            p_spec_local[k] =
                m_micrograin_bsdfs[k]->specular_component_sampling_probability(
                    cos_theta_i);
        }

        // ---- index-only sort by radius ----
        UInt32 idx[NbGrainMax];
        Float r_key[NbGrainMax];
        for (size_t k = 0; k < NbGrainMax; ++k) {
            idx[k]   = (UInt32) k;
            r_key[k] = pc_local.r[k];
        }

        this->sort16_index_by_radius(r_key, idx, m_bsdf_count, active);

        // ---- materialize sorted cache once ----
        PackedCache pc;
        Float p_spec[NbGrainMax];
        materialize_sorted_from_index(pc_local, p_spec_local, idx, pc, p_spec,
                                      m_bsdf_count, active);

        Float global_tau_0 = this->packed_global_tau0(pc, active);

        // ===== Scheme D: suffix tables for lambda =====
        Float log_lambda[NbGrainMax];
        Float log_term_lambda_tab[NbGrainMax];
        Float term_lambda_tab[NbGrainMax];

        build_suffix_tables_lambda(pc, log_lambda, log_term_lambda_tab,
                                   term_lambda_tab, m_bsdf_count, active);

        // additionally build suffix sums for diffuse factorization:
        // S_i = Σ_{j=i..n-1} log_lambda[j] * (1 - p_spec[j])
        Float suffix_mix[NbGrainMax];
        for (size_t k = 0; k < NbGrainMax; ++k)
            suffix_mix[k] = 0.f;

        Float acc_mix = 0.f;
        for (int jj = (int) m_bsdf_count - 1; jj >= 0; --jj) {
            acc_mix += log_lambda[(size_t) jj] * (1.f - p_spec[(size_t) jj]);
            suffix_mix[(size_t) jj] = acc_mix;
        }

        Vector3f wh     = dr::normalize(si.wi + wo);
        Mask valid_spec = (dr::dot(si.wi, wh) > 0.f) & (dr::dot(wo, wh) > 0.f);

        // per-j precompute: tmp = M^T * wh
        Vector3f tmp[NbGrainMax];
        Float norm_sqr[NbGrainMax];
        Float hv_height[NbGrainMax];

        for (size_t j = 0; j < m_bsdf_count; ++j) {
            tmp[j]      = this->mul_Mt(pc.a[j], pc.b[j], pc.c[j], pc.d[j], wh);
            norm_sqr[j] = dr::squared_norm(tmp[j]);
            Vector3f wh_1 = dr::normalize(tmp[j]);
            hv_height[j]  = Frame3f::cos_theta(wh_1) * pc.r[j];
        }

        Float pdf_ = 0.f;

        // ---- diffuse part: O(n) (exact algebraic reduction) ----
        // pdf_diff * Σ_i p_level(i) * [ S_i / L_i ]
        Float pdf_diff = warp::square_to_cosine_hemisphere_pdf(wo);

        Float term_kappa = 1.f - global_tau_0;
        Float h_lower    = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float h_upper = pc.r[i];

            Float term_lambda_i     = term_lambda_tab[i];
            Float log_term_lambda_i = log_term_lambda_tab[i];

            Float p_level = this->proba_level(global_tau_0, term_kappa,
                                              term_lambda_i, h_upper, h_lower);

            // exact: Σ_{j>=i} p_type(i,j)*(1-p_spec[j]) = suffix_mix[i] /
            // log_term_lambda_i
            Float denom = dr::maximum(log_term_lambda_i, 1e-8f);
            Float mix_i = suffix_mix[i] / denom;

            pdf_ += dr::select(active, p_level * mix_i * pdf_diff, 0.f);

            // update kappa (same as original)
            term_kappa /= (1.f - pc.tau0[i]);
            h_lower = h_upper;
        }

        // ---- specular part: keep (i,j) but cheaper p_type / term_lambda
        // lookup ----
        term_kappa = 1.f - global_tau_0;
        h_lower    = 0.f;

        for (size_t i = 0; i < m_bsdf_count; ++i) {
            Float h_upper = pc.r[i];

            Float term_lambda_i     = term_lambda_tab[i];
            Float log_term_lambda_i = log_term_lambda_tab[i];
            Float denom             = dr::maximum(log_term_lambda_i, 1e-8f);

            Float p_level    = this->proba_level(global_tau_0, term_kappa,
                                                 term_lambda_i, h_upper, h_lower);
            Mask valid_level = dr::neq(h_upper - h_lower, 0.f);

            for (size_t j = i; j < m_bsdf_count; ++j) {
                Float p_type = dr::clamp(log_lambda[j] / denom, 0.f, 1.f);

                Mask inRange =
                    (hv_height[j] > h_lower) & (hv_height[j] <= h_upper);

                Float pdf_spec =
                    this->square_to_micrograin_conditional_level_and_type_pdf(
                        term_lambda_i, pc.r[j], h_upper, h_lower, pc.abs_det[j],
                        norm_sqr[j], wh) /
                    (4.f * dr::dot(wo, wh));

                pdf_ +=
                    dr::select(active & valid_level & inRange & valid_spec,
                               p_level * p_type * p_spec[j] * pdf_spec, 0.f);
            }

            term_kappa /= (1.f - pc.tau0[i]);
            h_lower = h_upper;
        }

        Mask accident_NaN = dr::isnan(pdf_);
        return dr::select(accident_NaN, 0.f, pdf_);
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

        // packed once (original order；eval_ex don't need to sort by radius)
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
                ctx, si, wo, h, Mask(active & G_local_h));

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
                si, wo, m, Mask(active & G_local_m & valid_normal_sample));

            Spectrum brdf_diff =
                refl * D_type_normal_joint_m * G_dist_m / pdf_m / cos_theta_i;

            value += dr::select(active & G_local_m & valid_normal_sample,
                                brdf_diff, 0.f);
        }

        Mask accident_NaN = dr::any(dr::isnan(value));
        return dr::select(accident_NaN, 0.f, value);
    }

    // ================== original spherical sampling & pdf helper (don't need to modifier the matrix)
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
