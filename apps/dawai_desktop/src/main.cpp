#include "aifr3d/analyzer.hpp"

#include <cmath>
#include <cctype>
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
  double sampleRateHz{0.0};
  std::size_t frameCount{0};
  std::vector<float> interleavedStereo;
};

std::string readTextFile(const std::string& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::invalid_argument("Cannot open file: " + path);
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

std::optional<double> findNumberByKey(const std::string& text, const std::string& key) {
  const auto k = "\"" + key + "\"";
  auto pos = text.find(k);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  pos = text.find(':', pos + k.size());
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  ++pos;
  while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
    ++pos;
  }
  if (pos >= text.size() || text.compare(pos, 4, "null") == 0) {
    return std::nullopt;
  }
  std::size_t end = pos;
  while (end < text.size()) {
    const char c = text[end];
    if (!(std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')) {
      break;
    }
    ++end;
  }
  if (end == pos) {
    return std::nullopt;
  }
  return std::stod(text.substr(pos, end - pos));
}

std::optional<std::string> findStringByKey(const std::string& text, const std::string& key) {
  const auto k = "\"" + key + "\"";
  auto pos = text.find(k);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  pos = text.find(':', pos + k.size());
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  pos = text.find('"', pos);
  if (pos == std::string::npos) {
    return std::nullopt;
  }
  ++pos;
  auto end = text.find('"', pos);
  if (end == std::string::npos) {
    return std::nullopt;
  }
  return text.substr(pos, end - pos);
}

std::uint16_t readU16Le(const std::vector<std::uint8_t>& d, std::size_t off) {
  return static_cast<std::uint16_t>(d[off] | (static_cast<std::uint16_t>(d[off + 1]) << 8));
}

std::uint32_t readU32Le(const std::vector<std::uint8_t>& d, std::size_t off) {
  return static_cast<std::uint32_t>(d[off] | (static_cast<std::uint32_t>(d[off + 1]) << 8) |
                                    (static_cast<std::uint32_t>(d[off + 2]) << 16) |
                                    (static_cast<std::uint32_t>(d[off + 3]) << 24));
}

float decodePcm(const std::uint8_t* p, int bps) {
  if (bps == 16) {
    const std::int16_t v = static_cast<std::int16_t>(p[0] | (static_cast<std::int16_t>(p[1]) << 8));
    return static_cast<float>(static_cast<double>(v) / 32768.0);
  }
  if (bps == 24) {
    std::int32_t v = (static_cast<std::int32_t>(p[2]) << 24) | (static_cast<std::int32_t>(p[1]) << 16) |
                     (static_cast<std::int32_t>(p[0]) << 8);
    v >>= 8;
    return static_cast<float>(static_cast<double>(v) / 8388608.0);
  }
  if (bps == 32) {
    const std::int32_t v = static_cast<std::int32_t>(p[0] | (static_cast<std::int32_t>(p[1]) << 8) |
                                                     (static_cast<std::int32_t>(p[2]) << 16) |
                                                     (static_cast<std::int32_t>(p[3]) << 24));
    return static_cast<float>(static_cast<double>(v) / 2147483648.0);
  }
  throw std::invalid_argument("Unsupported PCM depth in WAV");
}

WavData loadWavStereo(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::invalid_argument("Cannot open wav: " + path);
  }
  std::vector<std::uint8_t> d((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (d.size() < 44 || std::string(reinterpret_cast<const char*>(d.data()), 4) != "RIFF" ||
      std::string(reinterpret_cast<const char*>(d.data() + 8), 4) != "WAVE") {
    throw std::invalid_argument("Invalid WAV file");
  }

  std::uint16_t format = 0;
  std::uint16_t channels = 0;
  std::uint32_t sampleRate = 0;
  std::uint16_t bps = 0;
  std::size_t dataOff = 0;
  std::size_t dataSize = 0;

  std::size_t off = 12;
  while (off + 8 <= d.size()) {
    const auto id = std::string(reinterpret_cast<const char*>(d.data() + off), 4);
    const auto size = static_cast<std::size_t>(readU32Le(d, off + 4));
    const auto chunkData = off + 8;
    if (chunkData + size > d.size()) {
      throw std::invalid_argument("Corrupt WAV chunk");
    }
    if (id == "fmt ") {
      format = readU16Le(d, chunkData);
      channels = readU16Le(d, chunkData + 2);
      sampleRate = readU32Le(d, chunkData + 4);
      bps = readU16Le(d, chunkData + 14);
    } else if (id == "data") {
      dataOff = chunkData;
      dataSize = size;
      break;
    }
    off = chunkData + size + (size % 2U);
  }

  if (format != 1 || channels == 0 || sampleRate == 0 || bps == 0 || dataOff == 0) {
    throw std::invalid_argument("Unsupported WAV format for stub loader");
  }

  const int bytesPerSample = bps / 8;
  const int bytesPerFrame = bytesPerSample * static_cast<int>(channels);
  const std::size_t frameCount = dataSize / static_cast<std::size_t>(bytesPerFrame);

  WavData out;
  out.sampleRateHz = static_cast<double>(sampleRate);
  out.frameCount = frameCount;
  out.interleavedStereo.resize(frameCount * 2U, 0.0f);

  for (std::size_t i = 0; i < frameCount; ++i) {
    const std::size_t foff = dataOff + i * static_cast<std::size_t>(bytesPerFrame);
    const float l = decodePcm(d.data() + foff, bps);
    const std::size_t roff = foff + static_cast<std::size_t>(bytesPerSample * (channels > 1 ? 1 : 0));
    const float r = decodePcm(d.data() + roff, bps);
    out.interleavedStereo[i * 2U] = l;
    out.interleavedStereo[i * 2U + 1U] = r;
  }

  return out;
}

int runSessionDashboard(const std::string& sessionDir) {
  const auto analysisText = readTextFile(sessionDir + "/analysis.json");
  const auto issuesText = readTextFile(sessionDir + "/issues.json");

  if (!findNumberByKey(analysisText, "schema_version").has_value()) {
    throw std::invalid_argument("analysis.json missing schema_version");
  }
  if (!findStringByKey(issuesText, "schema_version").has_value()) {
    throw std::invalid_argument("issues.json missing schema_version");
  }

  std::cout << "=== DawAI Desktop Stub Dashboard ===\n";
  if (auto score = findNumberByKey(analysisText, "overall_0_100"); score.has_value()) {
    std::cout << "Overall score: " << std::fixed << std::setprecision(2) << *score << "\n";
  } else {
    std::cout << "Overall score: n/a\n";
  }

  auto intLufs = findNumberByKey(analysisText, "integrated_lufs");
  auto tp = findNumberByKey(analysisText, "true_peak_dbfs");
  auto corr = findNumberByKey(analysisText, "correlation");

  std::cout << "Integrated LUFS: " << (intLufs.has_value() ? std::to_string(*intLufs) : std::string("n/a")) << "\n";
  std::cout << "True Peak dBFS: " << (tp.has_value() ? std::to_string(*tp) : std::string("n/a")) << "\n";
  std::cout << "Stereo Corr: " << (corr.has_value() ? std::to_string(*corr) : std::string("n/a")) << "\n";

  std::cout << "Top issue preview:\n";
  if (auto title = findStringByKey(issuesText, "title"); title.has_value()) {
    std::cout << "- " << *title << "\n";
  } else {
    std::cout << "- none\n";
  }

  std::size_t titleCount = 0;
  std::size_t pos = 0;
  const std::string titleToken = "\"title\"";
  while ((pos = issuesText.find(titleToken, pos)) != std::string::npos) {
    ++titleCount;
    pos += titleToken.size();
  }
  std::cout << "Issue count (approx): " << titleCount << "\n";

  return 0;
}

int runOfflineAnalysis(const std::string& wavPath) {
  const auto wav = loadWavStereo(wavPath);
  aifr3d::Analyzer analyzer;
  const auto analysis = analyzer.analyzeInterleavedStereo(wav.interleavedStereo.data(), wav.frameCount, wav.sampleRateHz);

  std::cout << "=== DawAI Offline Analysis (Stub) ===\n";
  std::cout << "Frames: " << analysis.frame_count << "\n";
  std::cout << "Sample rate: " << analysis.sample_rate_hz << "\n";
  std::cout << "Peak dBFS: " << (analysis.basic.peak_dbfs.has_value() ? std::to_string(*analysis.basic.peak_dbfs) : "n/a")
            << "\n";
  std::cout << "RMS dBFS: " << (analysis.basic.rms_dbfs.has_value() ? std::to_string(*analysis.basic.rms_dbfs) : "n/a")
            << "\n";
  std::cout << "Integrated LUFS: "
            << (analysis.loudness.integrated_lufs.has_value() ? std::to_string(*analysis.loudness.integrated_lufs)
                                                               : "n/a")
            << "\n";

  return 0;
}

void printUsage() {
  std::cout << "DawAI Desktop Stub (Phase 8)\n"
            << "Usage:\n"
            << "  dawai_desktop_stub --session <session_folder>\n"
            << "  dawai_desktop_stub --analyze <wav_file>\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      printUsage();
      return 2;
    }

    const std::string mode = argv[1];
    const std::string input = argv[2];

    if (mode == "--session") {
      return runSessionDashboard(input);
    }
    if (mode == "--analyze") {
      return runOfflineAnalysis(input);
    }

    printUsage();
    return 2;
  } catch (const std::exception& e) {
    std::cerr << "dawai_desktop_stub error: " << e.what() << "\n";
    return 1;
  }
}
