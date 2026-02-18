#include "ReportExporter.h"

#include "ChartRenderer.h"

namespace aifr3d::plugin {

namespace {

bool writeJson(const juce::File& file, const juce::var& v, juce::String& err) {
  if (!file.replaceWithText(juce::JSON::toString(v, true))) {
    err = "Failed writing JSON file: " + file.getFullPathName();
    return false;
  }
  return true;
}

juce::var makeAnalysisJson(const AnalysisSnapshot& s) {
  auto* rootObj = new juce::DynamicObject();
  rootObj->setProperty("schema_version", s.analysis.schema_version);
  rootObj->setProperty("analysis_id", s.analysis.analysis_id.has_value() ? juce::var(*s.analysis.analysis_id) : juce::var());
  rootObj->setProperty("frame_count", static_cast<int64_t>(s.analysis.frame_count));
  rootObj->setProperty("sample_rate_hz", s.analysis.sample_rate_hz);
  rootObj->setProperty("generated_at_utc", s.analysis.generated_at_utc);

  auto writeBasic = [&](const char* name, const std::optional<double>& value) {
    rootObj->setProperty(name, ReportExporter::optionalNumber(value));
  };

  auto* basic = new juce::DynamicObject();
  basic->setProperty("peak_dbfs", ReportExporter::optionalNumber(s.analysis.basic.peak_dbfs));
  basic->setProperty("rms_dbfs", ReportExporter::optionalNumber(s.analysis.basic.rms_dbfs));
  basic->setProperty("crest_db", ReportExporter::optionalNumber(s.analysis.basic.crest_db));
  rootObj->setProperty("basic", juce::var(basic));

  auto* loud = new juce::DynamicObject();
  loud->setProperty("integrated_lufs", ReportExporter::optionalNumber(s.analysis.loudness.integrated_lufs));
  loud->setProperty("short_term_lufs", ReportExporter::optionalNumber(s.analysis.loudness.short_term_lufs));
  loud->setProperty("loudness_range_lu", ReportExporter::optionalNumber(s.analysis.loudness.loudness_range_lu));
  rootObj->setProperty("loudness", juce::var(loud));

  auto* tp = new juce::DynamicObject();
  tp->setProperty("true_peak_dbfs", ReportExporter::optionalNumber(s.analysis.true_peak.true_peak_dbfs));
  tp->setProperty("oversample_factor", s.analysis.true_peak.oversample_factor);
  rootObj->setProperty("true_peak", juce::var(tp));

  auto* spectral = new juce::DynamicObject();
  spectral->setProperty("sub", ReportExporter::optionalNumber(s.analysis.spectral.sub));
  spectral->setProperty("low", ReportExporter::optionalNumber(s.analysis.spectral.low));
  spectral->setProperty("lowmid", ReportExporter::optionalNumber(s.analysis.spectral.lowmid));
  spectral->setProperty("mid", ReportExporter::optionalNumber(s.analysis.spectral.mid));
  spectral->setProperty("highmid", ReportExporter::optionalNumber(s.analysis.spectral.highmid));
  spectral->setProperty("high", ReportExporter::optionalNumber(s.analysis.spectral.high));
  spectral->setProperty("air", ReportExporter::optionalNumber(s.analysis.spectral.air));
  rootObj->setProperty("spectral", juce::var(spectral));

  auto* stereo = new juce::DynamicObject();
  stereo->setProperty("correlation", ReportExporter::optionalNumber(s.analysis.stereo.correlation));
  stereo->setProperty("lr_balance_db", ReportExporter::optionalNumber(s.analysis.stereo.lr_balance_db));
  stereo->setProperty("width_proxy", ReportExporter::optionalNumber(s.analysis.stereo.width_proxy));
  rootObj->setProperty("stereo", juce::var(stereo));

  auto* dynamics = new juce::DynamicObject();
  dynamics->setProperty("peak_dbfs", ReportExporter::optionalNumber(s.analysis.dynamics.peak_dbfs));
  dynamics->setProperty("rms_dbfs", ReportExporter::optionalNumber(s.analysis.dynamics.rms_dbfs));
  dynamics->setProperty("crest_db", ReportExporter::optionalNumber(s.analysis.dynamics.crest_db));
  dynamics->setProperty("dr_proxy_db", ReportExporter::optionalNumber(s.analysis.dynamics.dr_proxy_db));
  rootObj->setProperty("dynamics", juce::var(dynamics));

  return juce::var(rootObj);
}

juce::var makeBenchmarkCompareJson(const AnalysisSnapshot& s) {
  if (!s.benchmarkCompare.has_value()) {
    return juce::var();
  }
  auto* root = new juce::DynamicObject();
  root->setProperty("profile_id", s.benchmarkCompare->profile_id);
  root->setProperty("genre", s.benchmarkCompare->genre);
  auto* summary = new juce::DynamicObject();
  summary->setProperty("in_range_count", s.benchmarkCompare->summary.in_range_count);
  summary->setProperty("slightly_off_count", s.benchmarkCompare->summary.slightly_off_count);
  summary->setProperty("needs_attention_count", s.benchmarkCompare->summary.needs_attention_count);
  summary->setProperty("unknown_count", s.benchmarkCompare->summary.unknown_count);
  root->setProperty("summary", juce::var(summary));
  return juce::var(root);
}

juce::var makeReferenceJson(const AnalysisSnapshot& s) {
  if (!s.referenceCompare.has_value()) {
    return juce::var();
  }
  auto* root = new juce::DynamicObject();
  root->setProperty("schema_version", s.referenceCompare->schema_version);
  root->setProperty("reference_count", static_cast<int64_t>(s.referenceCompare->reference_count));
  auto* dist = new juce::DynamicObject();
  dist->setProperty("basic_closeness_0_100", s.referenceCompare->distance.basic_closeness_0_100);
  dist->setProperty("loudness_closeness_0_100", s.referenceCompare->distance.loudness_closeness_0_100);
  dist->setProperty("spectral_closeness_0_100", s.referenceCompare->distance.spectral_closeness_0_100);
  dist->setProperty("stereo_closeness_0_100", s.referenceCompare->distance.stereo_closeness_0_100);
  dist->setProperty("dynamics_closeness_0_100", s.referenceCompare->distance.dynamics_closeness_0_100);
  dist->setProperty("overall_closeness_0_100", s.referenceCompare->distance.overall_closeness_0_100);
  root->setProperty("distance", juce::var(dist));
  return juce::var(root);
}

juce::var makeIssuesJson(const AnalysisSnapshot& s) {
  if (!s.issues.has_value()) {
    return juce::var();
  }
  auto* root = new juce::DynamicObject();
  root->setProperty("schema_version", s.issues->schema_version);
  root->setProperty("generated_at_utc", s.issues->generated_at_utc);
  juce::Array<juce::var> arr;
  for (const auto& issue : s.issues->top_issues) {
    auto* o = new juce::DynamicObject();
    o->setProperty("id", issue.id);
    o->setProperty("title", issue.title);
    o->setProperty("severity", ReportExporter::severityToString(issue.severity));
    o->setProperty("confidence_0_100", issue.confidence_0_100);
    o->setProperty("summary", issue.summary);
    juce::Array<juce::var> fixes;
    for (const auto& step : issue.fix_steps) {
      auto* sObj = new juce::DynamicObject();
      sObj->setProperty("title", step.title);
      sObj->setProperty("action", step.action);
      sObj->setProperty("when", step.when);
      sObj->setProperty("how", step.how);
      sObj->setProperty("recheck", step.recheck);
      fixes.add(juce::var(sObj));
    }
    o->setProperty("fix_steps", fixes);
    arr.add(juce::var(o));
  }
  root->setProperty("top_issues", arr);
  return juce::var(root);
}

bool writePng(const juce::Image& image, const juce::File& file, juce::String& err) {
  juce::PNGImageFormat png;
  std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
  if (stream == nullptr) {
    err = "Cannot open PNG output stream: " + file.getFullPathName();
    return false;
  }
  if (!png.writeImageToStream(image, *stream)) {
    err = "Failed writing PNG: " + file.getFullPathName();
    return false;
  }
  return true;
}

}  // namespace

ReportExporter::ExportResult ReportExporter::exportSnapshot(const AnalysisSnapshot& snapshot,
                                                            const juce::File& baseDocumentsDir) {
  ExportResult out;
  if (!snapshot.valid) {
    out.error = "Cannot export: snapshot is invalid.";
    return out;
  }

  const auto sessionId = makeSessionId();
  const auto root = baseDocumentsDir.getChildFile("AIFR3D").getChildFile("sessions").getChildFile(sessionId);
  const auto chartsDir = root.getChildFile("charts");
  if (!chartsDir.createDirectory()) {
    out.error = "Could not create session directory: " + chartsDir.getFullPathName();
    return out;
  }

  juce::String err;
  if (!writeJson(root.getChildFile("analysis.json"), makeAnalysisJson(snapshot), err)) {
    out.error = err;
    return out;
  }
  if (snapshot.benchmarkCompare.has_value() &&
      !writeJson(root.getChildFile("benchmark_compare.json"), makeBenchmarkCompareJson(snapshot), err)) {
    out.error = err;
    return out;
  }
  if (snapshot.referenceCompare.has_value() &&
      !writeJson(root.getChildFile("reference_compare.json"), makeReferenceJson(snapshot), err)) {
    out.error = err;
    return out;
  }
  if (snapshot.issues.has_value() && !writeJson(root.getChildFile("issues.json"), makeIssuesJson(snapshot), err)) {
    out.error = err;
    return out;
  }

  if (!writePng(ChartRenderer::renderSpectralBands(snapshot, 960, 360),
                chartsDir.getChildFile("spectral_bands.png"),
                err)) {
    out.error = err;
    return out;
  }
  if (!writePng(ChartRenderer::renderLoudnessTruePeak(snapshot, 720, 320),
                chartsDir.getChildFile("loudness_true_peak.png"),
                err)) {
    out.error = err;
    return out;
  }
  if (!writePng(ChartRenderer::renderStereoCorrelation(snapshot, 720, 220),
                chartsDir.getChildFile("stereo_correlation.png"),
                err)) {
    out.error = err;
    return out;
  }

  out.ok = true;
  out.sessionId = sessionId;
  out.sessionDir = root;
  return out;
}

juce::String ReportExporter::makeSessionId() {
  const auto now = juce::Time::getCurrentTime();
  return now.formatted("%Y%m%d_%H%M%S") + "_" + juce::Uuid().toDashedString();
}

juce::var ReportExporter::optionalNumber(const std::optional<double>& value) {
  return value.has_value() ? juce::var(*value) : juce::var();
}

juce::String ReportExporter::severityToString(aifr3d::Severity sev) {
  switch (sev) {
    case aifr3d::Severity::Info:
      return "Info";
    case aifr3d::Severity::Minor:
      return "Minor";
    case aifr3d::Severity::Moderate:
      return "Moderate";
    case aifr3d::Severity::Severe:
      return "Severe";
    default:
      return "Unknown";
  }
}

}  // namespace aifr3d::plugin
