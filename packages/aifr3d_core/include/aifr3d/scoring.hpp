#pragma once

#include "aifr3d/analyzer.hpp"
#include "aifr3d/compare.hpp"

#include <map>
#include <string>
#include <vector>

namespace aifr3d {

struct ScoreConfig {
  std::string version{"v1"};
  std::map<std::string, double> weights{
      {"loudness", 0.30},
      {"dynamics", 0.25},
      {"tonal_balance", 0.25},
      {"stereo", 0.20},
  };
};

struct ScoreBreakdown {
  std::string version{"v1"};
  double overall_0_100{0.0};
  std::map<std::string, double> subscores;
  std::map<std::string, double> weights;
  std::vector<std::string> notes;
};

struct AnalysisReport {
  AnalysisResult analysis;
  BenchmarkCompareResult benchmark_compare;
  ScoreBreakdown score;
};

ScoreBreakdown computeScore(const BenchmarkCompareResult& compare,
                            const ScoreConfig& config = {});

}  // namespace aifr3d
