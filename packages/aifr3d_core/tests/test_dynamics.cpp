#include "aifr3d/dynamics.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool cond, const std::string& msg) {
  if (!cond) {
    throw std::runtime_error(msg);
  }
}

}  // namespace

int main() {
  try {
    constexpr std::size_t frames = 4096;

    std::vector<float> silence(frames * 2U, 0.0f);
    const auto ds = aifr3d::compute_dynamics_interleaved_stereo(silence.data(), frames);
    require(!ds.peak_dbfs.has_value(), "silence peak must be nullopt");
    require(!ds.rms_dbfs.has_value(), "silence rms must be nullopt");
    require(!ds.crest_db.has_value(), "silence crest must be nullopt");

    std::vector<float> tone(frames * 2U, 0.25f);
    const auto dt = aifr3d::compute_dynamics_interleaved_stereo(tone.data(), frames);
    require(dt.peak_dbfs.has_value(), "tone peak missing");
    require(dt.rms_dbfs.has_value(), "tone rms missing");
    require(dt.crest_db.has_value(), "tone crest missing");
    require(std::fabs(*dt.crest_db) < 1e-9, "constant tone should have near-zero crest");
  } catch (const std::exception& ex) {
    std::cerr << "[FAIL] " << ex.what() << '\n';
    return 1;
  }

  std::cout << "[PASS] test_dynamics\n";
  return 0;
}
