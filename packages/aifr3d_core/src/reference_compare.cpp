#include "aifr3d/reference_compare.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

namespace aifr3d {

namespace {

std::optional<double> delta(const std::optional<double>& mix, const std::optional<double>& ref) {
  if (!mix.has_value() || !ref.has_value()) {
    return std::nullopt;
  }
  return *mix - *ref;
}

std::optional<double> meanOptionals(const std::vector<std::optional<double>>& vals) {
  double sum = 0.0;
  std::size_t n = 0;
  for (const auto& v : vals) {
    if (v.has_value()) {
      sum += *v;
      ++n;
    }
  }
  if (n == 0U) {
    return std::nullopt;
  }
  return sum / static_cast<double>(n);
}

double clamp0to100(double x) {
  return std::max(0.0, std::min(100.0, x));
}

double closenessForDeltas(const std::vector<std::optional<double>>& deltas,
                         double scale) {
  double sum = 0.0;
  std::size_t n = 0;
  for (const auto& d : deltas) {
    if (!d.has_value()) {
      continue;
    }
    const double normalized = std::fabs(*d) / std::max(1e-9, scale);
    const double score = 100.0 / (1.0 + normalized);
    sum += score;
    ++n;
  }
  if (n == 0U) {
    return 0.0;
  }
  return clamp0to100(sum / static_cast<double>(n));
}

ReferenceMetricDelta makeMetricDelta(const std::optional<double>& mix,
                                     const std::optional<double>& ref) {
  ReferenceMetricDelta out;
  out.mix = mix;
  out.reference = ref;
  out.delta = delta(mix, ref);
  return out;
}

}  // namespace

ReferenceCompareResult compareToReferences(const AnalysisResult& mix,
                                           const std::vector<AnalysisResult>& refs) {
  if (refs.empty()) {
    throw std::invalid_argument("compareToReferences requires at least one reference result");
  }
  for (std::size_t i = 0; i < refs.size(); ++i) {
    if (refs[i].schema_version != mix.schema_version) {
      throw std::invalid_argument("schema_version mismatch between mix and reference index " +
                                  std::to_string(i));
    }
  }

  ReferenceCompareResult out;
  out.schema_version = mix.schema_version;
  out.reference_count = refs.size();

  out.reference_average = mix;
  out.reference_average.analysis_id = std::nullopt;
  out.reference_average.generated_at_utc = "1970-01-01T00:00:00Z";

  std::vector<std::optional<double>> avg_basic_peak;
  std::vector<std::optional<double>> avg_basic_rms;
  std::vector<std::optional<double>> avg_basic_crest;
  std::vector<std::optional<double>> avg_loudness_integrated;
  std::vector<std::optional<double>> avg_loudness_short;
  std::vector<std::optional<double>> avg_loudness_range;
  std::vector<std::optional<double>> avg_spectral_sub;
  std::vector<std::optional<double>> avg_spectral_low;
  std::vector<std::optional<double>> avg_spectral_lowmid;
  std::vector<std::optional<double>> avg_spectral_mid;
  std::vector<std::optional<double>> avg_spectral_highmid;
  std::vector<std::optional<double>> avg_spectral_high;
  std::vector<std::optional<double>> avg_spectral_air;
  std::vector<std::optional<double>> avg_stereo_corr;
  std::vector<std::optional<double>> avg_stereo_balance;
  std::vector<std::optional<double>> avg_stereo_width;
  std::vector<std::optional<double>> avg_dyn_peak;
  std::vector<std::optional<double>> avg_dyn_rms;
  std::vector<std::optional<double>> avg_dyn_crest;
  std::vector<std::optional<double>> avg_dyn_dr;

  for (std::size_t i = 0; i < refs.size(); ++i) {
    const AnalysisResult& ref = refs[i];

    avg_basic_peak.push_back(ref.basic.peak_dbfs);
    avg_basic_rms.push_back(ref.basic.rms_dbfs);
    avg_basic_crest.push_back(ref.basic.crest_db);
    avg_loudness_integrated.push_back(ref.loudness.integrated_lufs);
    avg_loudness_short.push_back(ref.loudness.short_term_lufs);
    avg_loudness_range.push_back(ref.loudness.loudness_range_lu);
    avg_spectral_sub.push_back(ref.spectral.sub);
    avg_spectral_low.push_back(ref.spectral.low);
    avg_spectral_lowmid.push_back(ref.spectral.lowmid);
    avg_spectral_mid.push_back(ref.spectral.mid);
    avg_spectral_highmid.push_back(ref.spectral.highmid);
    avg_spectral_high.push_back(ref.spectral.high);
    avg_spectral_air.push_back(ref.spectral.air);
    avg_stereo_corr.push_back(ref.stereo.correlation);
    avg_stereo_balance.push_back(ref.stereo.lr_balance_db);
    avg_stereo_width.push_back(ref.stereo.width_proxy);
    avg_dyn_peak.push_back(ref.dynamics.peak_dbfs);
    avg_dyn_rms.push_back(ref.dynamics.rms_dbfs);
    avg_dyn_crest.push_back(ref.dynamics.crest_db);
    avg_dyn_dr.push_back(ref.dynamics.dr_proxy_db);

    ReferenceDeltaPerTrack per;
    per.reference_index = i;
    per.basic.peak_dbfs = makeMetricDelta(mix.basic.peak_dbfs, ref.basic.peak_dbfs);
    per.basic.rms_dbfs = makeMetricDelta(mix.basic.rms_dbfs, ref.basic.rms_dbfs);
    per.basic.crest_db = makeMetricDelta(mix.basic.crest_db, ref.basic.crest_db);

    per.loudness.integrated_lufs =
        makeMetricDelta(mix.loudness.integrated_lufs, ref.loudness.integrated_lufs);
    per.loudness.short_term_lufs =
        makeMetricDelta(mix.loudness.short_term_lufs, ref.loudness.short_term_lufs);
    per.loudness.loudness_range_lu =
        makeMetricDelta(mix.loudness.loudness_range_lu, ref.loudness.loudness_range_lu);

    per.spectral.sub = makeMetricDelta(mix.spectral.sub, ref.spectral.sub);
    per.spectral.low = makeMetricDelta(mix.spectral.low, ref.spectral.low);
    per.spectral.lowmid = makeMetricDelta(mix.spectral.lowmid, ref.spectral.lowmid);
    per.spectral.mid = makeMetricDelta(mix.spectral.mid, ref.spectral.mid);
    per.spectral.highmid = makeMetricDelta(mix.spectral.highmid, ref.spectral.highmid);
    per.spectral.high = makeMetricDelta(mix.spectral.high, ref.spectral.high);
    per.spectral.air = makeMetricDelta(mix.spectral.air, ref.spectral.air);

    per.stereo.correlation = makeMetricDelta(mix.stereo.correlation, ref.stereo.correlation);
    per.stereo.lr_balance_db = makeMetricDelta(mix.stereo.lr_balance_db, ref.stereo.lr_balance_db);
    per.stereo.width_proxy = makeMetricDelta(mix.stereo.width_proxy, ref.stereo.width_proxy);

    per.dynamics.peak_dbfs = makeMetricDelta(mix.dynamics.peak_dbfs, ref.dynamics.peak_dbfs);
    per.dynamics.rms_dbfs = makeMetricDelta(mix.dynamics.rms_dbfs, ref.dynamics.rms_dbfs);
    per.dynamics.crest_db = makeMetricDelta(mix.dynamics.crest_db, ref.dynamics.crest_db);
    per.dynamics.dr_proxy_db = makeMetricDelta(mix.dynamics.dr_proxy_db, ref.dynamics.dr_proxy_db);

    out.per_reference.push_back(per);
  }

  out.reference_average.basic.peak_dbfs = meanOptionals(avg_basic_peak);
  out.reference_average.basic.rms_dbfs = meanOptionals(avg_basic_rms);
  out.reference_average.basic.crest_db = meanOptionals(avg_basic_crest);
  out.reference_average.loudness.integrated_lufs = meanOptionals(avg_loudness_integrated);
  out.reference_average.loudness.short_term_lufs = meanOptionals(avg_loudness_short);
  out.reference_average.loudness.loudness_range_lu = meanOptionals(avg_loudness_range);
  out.reference_average.spectral.sub = meanOptionals(avg_spectral_sub);
  out.reference_average.spectral.low = meanOptionals(avg_spectral_low);
  out.reference_average.spectral.lowmid = meanOptionals(avg_spectral_lowmid);
  out.reference_average.spectral.mid = meanOptionals(avg_spectral_mid);
  out.reference_average.spectral.highmid = meanOptionals(avg_spectral_highmid);
  out.reference_average.spectral.high = meanOptionals(avg_spectral_high);
  out.reference_average.spectral.air = meanOptionals(avg_spectral_air);
  out.reference_average.stereo.correlation = meanOptionals(avg_stereo_corr);
  out.reference_average.stereo.lr_balance_db = meanOptionals(avg_stereo_balance);
  out.reference_average.stereo.width_proxy = meanOptionals(avg_stereo_width);
  out.reference_average.dynamics.peak_dbfs = meanOptionals(avg_dyn_peak);
  out.reference_average.dynamics.rms_dbfs = meanOptionals(avg_dyn_rms);
  out.reference_average.dynamics.crest_db = meanOptionals(avg_dyn_crest);
  out.reference_average.dynamics.dr_proxy_db = meanOptionals(avg_dyn_dr);

  const auto aggregate_delta = [&](const std::optional<double>& mix_v,
                                   const std::optional<double>& ref_mean) {
    return delta(mix_v, ref_mean);
  };

  const std::vector<std::optional<double>> basic_deltas{
      aggregate_delta(mix.basic.peak_dbfs, out.reference_average.basic.peak_dbfs),
      aggregate_delta(mix.basic.rms_dbfs, out.reference_average.basic.rms_dbfs),
      aggregate_delta(mix.basic.crest_db, out.reference_average.basic.crest_db),
  };
  const std::vector<std::optional<double>> loudness_deltas{
      aggregate_delta(mix.loudness.integrated_lufs, out.reference_average.loudness.integrated_lufs),
      aggregate_delta(mix.loudness.short_term_lufs, out.reference_average.loudness.short_term_lufs),
      aggregate_delta(mix.loudness.loudness_range_lu, out.reference_average.loudness.loudness_range_lu),
  };
  const std::vector<std::optional<double>> spectral_deltas{
      aggregate_delta(mix.spectral.sub, out.reference_average.spectral.sub),
      aggregate_delta(mix.spectral.low, out.reference_average.spectral.low),
      aggregate_delta(mix.spectral.lowmid, out.reference_average.spectral.lowmid),
      aggregate_delta(mix.spectral.mid, out.reference_average.spectral.mid),
      aggregate_delta(mix.spectral.highmid, out.reference_average.spectral.highmid),
      aggregate_delta(mix.spectral.high, out.reference_average.spectral.high),
      aggregate_delta(mix.spectral.air, out.reference_average.spectral.air),
  };
  const std::vector<std::optional<double>> stereo_deltas{
      aggregate_delta(mix.stereo.correlation, out.reference_average.stereo.correlation),
      aggregate_delta(mix.stereo.lr_balance_db, out.reference_average.stereo.lr_balance_db),
      aggregate_delta(mix.stereo.width_proxy, out.reference_average.stereo.width_proxy),
  };
  const std::vector<std::optional<double>> dynamics_deltas{
      aggregate_delta(mix.dynamics.peak_dbfs, out.reference_average.dynamics.peak_dbfs),
      aggregate_delta(mix.dynamics.rms_dbfs, out.reference_average.dynamics.rms_dbfs),
      aggregate_delta(mix.dynamics.crest_db, out.reference_average.dynamics.crest_db),
      aggregate_delta(mix.dynamics.dr_proxy_db, out.reference_average.dynamics.dr_proxy_db),
  };

  out.distance.basic_closeness_0_100 = closenessForDeltas(basic_deltas, 3.0);
  out.distance.loudness_closeness_0_100 = closenessForDeltas(loudness_deltas, 3.0);
  out.distance.spectral_closeness_0_100 = closenessForDeltas(spectral_deltas, 6.0);
  out.distance.stereo_closeness_0_100 = closenessForDeltas(stereo_deltas, 0.5);
  out.distance.dynamics_closeness_0_100 = closenessForDeltas(dynamics_deltas, 3.0);
  out.distance.overall_closeness_0_100 =
      (out.distance.basic_closeness_0_100 + out.distance.loudness_closeness_0_100 +
       out.distance.spectral_closeness_0_100 + out.distance.stereo_closeness_0_100 +
       out.distance.dynamics_closeness_0_100) /
      5.0;

  return out;
}

}  // namespace aifr3d
