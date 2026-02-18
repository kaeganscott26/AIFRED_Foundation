#include "aifr3d/analyzer.hpp"
#include "aifr3d/benchmark_profile.hpp"
#include "aifr3d/compare.hpp"
#include "aifr3d/issues.hpp"
#include "aifr3d/reference_compare.hpp"
#include "aifr3d/scoring.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct WavData {
  double sample_rate_hz{0.0};
  std::size_t frame_count{0};
  std::vector<float> interleaved_stereo;
};

std::uint16_t read_u16_le(const std::vector<std::uint8_t>& d, std::size_t off) {
  return static_cast<std::uint16_t>(d[off] | (static_cast<std::uint16_t>(d[off + 1]) << 8));
}

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& d, std::size_t off) {
  return static_cast<std::uint32_t>(d[off] | (static_cast<std::uint32_t>(d[off + 1]) << 8) |
                                    (static_cast<std::uint32_t>(d[off + 2]) << 16) |
                                    (static_cast<std::uint32_t>(d[off + 3]) << 24));
}

float decode_sample_pcm(const std::uint8_t* p, int bits_per_sample) {
  if (bits_per_sample == 16) {
    const std::int16_t v = static_cast<std::int16_t>(p[0] | (static_cast<std::int16_t>(p[1]) << 8));
    return static_cast<float>(static_cast<double>(v) / 32768.0);
  }
  if (bits_per_sample == 24) {
    std::int32_t v = (static_cast<std::int32_t>(p[2]) << 24) | (static_cast<std::int32_t>(p[1]) << 16) |
                     (static_cast<std::int32_t>(p[0]) << 8);
    v >>= 8;
    return static_cast<float>(static_cast<double>(v) / 8388608.0);
  }
  if (bits_per_sample == 32) {
    const std::int32_t v = static_cast<std::int32_t>(p[0] | (static_cast<std::int32_t>(p[1]) << 8) |
                                                     (static_cast<std::int32_t>(p[2]) << 16) |
                                                     (static_cast<std::int32_t>(p[3]) << 24));
    return static_cast<float>(static_cast<double>(v) / 2147483648.0);
  }
  throw std::invalid_argument("Unsupported PCM bit depth");
}

float decode_sample_float32(const std::uint8_t* p) {
  std::uint32_t bits = static_cast<std::uint32_t>(p[0] | (static_cast<std::uint32_t>(p[1]) << 8) |
                                                  (static_cast<std::uint32_t>(p[2]) << 16) |
                                                  (static_cast<std::uint32_t>(p[3]) << 24));
  float out = 0.0F;
  std::memcpy(&out, &bits, sizeof(float));
  return out;
}

WavData load_wav_stereo(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::invalid_argument("cannot open wav: " + path);
  }
  std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (bytes.size() < 44U) {
    throw std::invalid_argument("wav too small");
  }
  if (std::string(reinterpret_cast<const char*>(bytes.data()), 4) != "RIFF" ||
      std::string(reinterpret_cast<const char*>(bytes.data() + 8), 4) != "WAVE") {
    throw std::invalid_argument("not a RIFF/WAVE file");
  }

  std::uint16_t audio_format = 0;
  std::uint16_t channels = 0;
  std::uint32_t sample_rate = 0;
  std::uint16_t bits_per_sample = 0;
  std::size_t data_off = 0;
  std::size_t data_size = 0;

  std::size_t off = 12;
  while (off + 8 <= bytes.size()) {
    const auto chunk_id = std::string(reinterpret_cast<const char*>(bytes.data() + off), 4);
    const auto chunk_size = static_cast<std::size_t>(read_u32_le(bytes, off + 4));
    const std::size_t chunk_data = off + 8;
    if (chunk_data + chunk_size > bytes.size()) {
      throw std::invalid_argument("invalid wav chunk bounds");
    }
    if (chunk_id == "fmt ") {
      if (chunk_size < 16) {
        throw std::invalid_argument("invalid fmt chunk");
      }
      audio_format = read_u16_le(bytes, chunk_data);
      channels = read_u16_le(bytes, chunk_data + 2);
      sample_rate = read_u32_le(bytes, chunk_data + 4);
      bits_per_sample = read_u16_le(bytes, chunk_data + 14);
    } else if (chunk_id == "data") {
      data_off = chunk_data;
      data_size = chunk_size;
      break;
    }
    off = chunk_data + chunk_size + (chunk_size % 2U);
  }

  if (data_off == 0 || sample_rate == 0 || channels == 0 || bits_per_sample == 0) {
    throw std::invalid_argument("wav missing required fmt/data fields");
  }

  const int bytes_per_sample = bits_per_sample / 8;
  const int bytes_per_frame = bytes_per_sample * static_cast<int>(channels);
  if (bytes_per_sample <= 0 || bytes_per_frame <= 0) {
    throw std::invalid_argument("invalid wav format geometry");
  }

  const std::size_t frame_count = data_size / static_cast<std::size_t>(bytes_per_frame);
  WavData out;
  out.sample_rate_hz = static_cast<double>(sample_rate);
  out.frame_count = frame_count;
  out.interleaved_stereo.resize(frame_count * 2U, 0.0F);

  for (std::size_t f = 0; f < frame_count; ++f) {
    const std::size_t frame_off = data_off + f * static_cast<std::size_t>(bytes_per_frame);
    auto read_channel = [&](std::size_t ch) {
      const std::size_t sample_off = frame_off + ch * static_cast<std::size_t>(bytes_per_sample);
      const auto* p = bytes.data() + sample_off;
      if (audio_format == 1) {
        return decode_sample_pcm(p, bits_per_sample);
      }
      if (audio_format == 3 && bits_per_sample == 32) {
        return decode_sample_float32(p);
      }
      throw std::invalid_argument("unsupported wav encoding (only PCM/float32)");
    };

    const float l = read_channel(0);
    const float r = read_channel(std::min<std::size_t>(1, static_cast<std::size_t>(channels - 1)));
    out.interleaved_stereo[f * 2U] = l;
    out.interleaved_stereo[f * 2U + 1U] = r;
  }

  return out;
}

std::string json_num_or_null(const std::optional<double>& v) {
  if (!v.has_value()) {
    return "null";
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(8) << *v;
  return oss.str();
}

std::string to_json(const aifr3d::AnalysisResult& a,
                    const std::optional<aifr3d::BenchmarkCompareResult>& b,
                    const std::optional<aifr3d::ReferenceCompareResult>& r,
                    const std::optional<aifr3d::ScoreBreakdown>& s,
                    const std::optional<aifr3d::IssueReport>& issues) {
  std::ostringstream o;
  o << "{\n";
  o << "  \"schema_version\": " << a.schema_version << ",\n";
  o << "  \"frame_count\": " << a.frame_count << ",\n";
  o << "  \"sample_rate_hz\": " << std::fixed << std::setprecision(2) << a.sample_rate_hz << ",\n";

  o << "  \"basic\": {\n";
  o << "    \"peak_dbfs\": " << json_num_or_null(a.basic.peak_dbfs) << ",\n";
  o << "    \"rms_dbfs\": " << json_num_or_null(a.basic.rms_dbfs) << ",\n";
  o << "    \"crest_db\": " << json_num_or_null(a.basic.crest_db) << "\n";
  o << "  },\n";

  o << "  \"loudness\": {\n";
  o << "    \"integrated_lufs\": " << json_num_or_null(a.loudness.integrated_lufs) << ",\n";
  o << "    \"short_term_lufs\": " << json_num_or_null(a.loudness.short_term_lufs) << ",\n";
  o << "    \"loudness_range_lu\": " << json_num_or_null(a.loudness.loudness_range_lu) << "\n";
  o << "  },\n";

  o << "  \"true_peak\": {\n";
  o << "    \"true_peak_dbfs\": " << json_num_or_null(a.true_peak.true_peak_dbfs) << ",\n";
  o << "    \"oversample_factor\": " << a.true_peak.oversample_factor << "\n";
  o << "  },\n";

  o << "  \"spectral\": {\n";
  o << "    \"sub\": " << json_num_or_null(a.spectral.sub) << ",\n";
  o << "    \"low\": " << json_num_or_null(a.spectral.low) << ",\n";
  o << "    \"lowmid\": " << json_num_or_null(a.spectral.lowmid) << ",\n";
  o << "    \"mid\": " << json_num_or_null(a.spectral.mid) << ",\n";
  o << "    \"highmid\": " << json_num_or_null(a.spectral.highmid) << ",\n";
  o << "    \"high\": " << json_num_or_null(a.spectral.high) << ",\n";
  o << "    \"air\": " << json_num_or_null(a.spectral.air) << "\n";
  o << "  },\n";

  o << "  \"stereo\": {\n";
  o << "    \"correlation\": " << json_num_or_null(a.stereo.correlation) << ",\n";
  o << "    \"lr_balance_db\": " << json_num_or_null(a.stereo.lr_balance_db) << ",\n";
  o << "    \"width_proxy\": " << json_num_or_null(a.stereo.width_proxy) << "\n";
  o << "  },\n";

  o << "  \"dynamics\": {\n";
  o << "    \"peak_dbfs\": " << json_num_or_null(a.dynamics.peak_dbfs) << ",\n";
  o << "    \"rms_dbfs\": " << json_num_or_null(a.dynamics.rms_dbfs) << ",\n";
  o << "    \"crest_db\": " << json_num_or_null(a.dynamics.crest_db) << ",\n";
  o << "    \"dr_proxy_db\": " << json_num_or_null(a.dynamics.dr_proxy_db) << "\n";
  o << "  },\n";

  if (s.has_value()) {
    o << "  \"score\": {\n";
    o << "    \"overall_0_100\": " << std::fixed << std::setprecision(4) << s->overall_0_100 << "\n";
    o << "  },\n";
  }
  if (b.has_value()) {
    o << "  \"benchmark_compare\": {\n";
    o << "    \"profile_id\": \"" << b->profile_id << "\",\n";
    o << "    \"in_range_count\": " << b->summary.in_range_count << "\n";
    o << "  },\n";
  }
  if (r.has_value()) {
    o << "  \"reference_compare\": {\n";
    o << "    \"reference_count\": " << r->reference_count << ",\n";
    o << "    \"overall_closeness_0_100\": " << std::fixed << std::setprecision(4)
      << r->distance.overall_closeness_0_100 << "\n";
    o << "  },\n";
  }
  if (issues.has_value()) {
    o << "  \"issues\": {\n";
    o << "    \"top_issues_count\": " << issues->top_issues.size() << "\n";
    o << "  }\n";
  } else {
    o << "  \"issues\": null\n";
  }

  o << "}\n";
  return o.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: aifr3d_core_cli <input.wav> <output.json> [benchmark.json] [reference.wav]\n";
    return 2;
  }

  try {
    const std::string input_wav = argv[1];
    const std::string out_json = argv[2];
    const std::optional<std::string> benchmark_path = (argc >= 4) ? std::optional<std::string>(argv[3]) : std::nullopt;
    const std::optional<std::string> reference_path = (argc >= 5) ? std::optional<std::string>(argv[4]) : std::nullopt;

    const auto wav = load_wav_stereo(input_wav);

    aifr3d::Analyzer analyzer;
    auto analysis = analyzer.analyzeInterleavedStereo(wav.interleaved_stereo.data(), wav.frame_count, wav.sample_rate_hz);

    std::optional<aifr3d::BenchmarkCompareResult> bench;
    std::optional<aifr3d::ReferenceCompareResult> refs;
    std::optional<aifr3d::ScoreBreakdown> score;

    if (benchmark_path.has_value()) {
      const auto profile = aifr3d::loadBenchmarkProfileFromJson(*benchmark_path);
      bench = aifr3d::compareAgainstBenchmark(analysis, profile);
      score = aifr3d::computeScore(*bench);
    }

    if (reference_path.has_value()) {
      const auto ref_wav = load_wav_stereo(*reference_path);
      auto ref_analysis = analyzer.analyzeInterleavedStereo(ref_wav.interleaved_stereo.data(),
                                                            ref_wav.frame_count,
                                                            ref_wav.sample_rate_hz);
      ref_analysis.schema_version = analysis.schema_version;
      refs = aifr3d::compareToReferences(analysis, {ref_analysis});
    }

    const aifr3d::BenchmarkCompareResult* bench_ptr = bench.has_value() ? &(*bench) : nullptr;
    const aifr3d::ReferenceCompareResult* refs_ptr = refs.has_value() ? &(*refs) : nullptr;
    auto issues = aifr3d::generateIssues(analysis, bench_ptr, refs_ptr);

    std::ofstream out(out_json);
    if (!out) {
      throw std::invalid_argument("cannot open output json: " + out_json);
    }
    out << to_json(analysis, bench, refs, score, issues);
    out.close();

    std::cout << "Wrote analysis JSON: " << out_json << "\n";
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "aifr3d_core_cli error: " << e.what() << "\n";
    return 1;
  }
}
