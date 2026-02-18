#include "aifr3d/benchmark_profile.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace aifr3d {

namespace {

struct JsonValue;
using JsonObject = std::map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

struct JsonValue {
  using Variant = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
  Variant value;

  bool isObject() const { return std::holds_alternative<JsonObject>(value); }
  bool isString() const { return std::holds_alternative<std::string>(value); }
  bool isNumber() const { return std::holds_alternative<double>(value); }

  const JsonObject& asObject() const { return std::get<JsonObject>(value); }
  const std::string& asString() const { return std::get<std::string>(value); }
  double asNumber() const { return std::get<double>(value); }
};

class Parser {
 public:
  explicit Parser(std::string text) : text_(std::move(text)) {}

  JsonValue parse() {
    skipWs();
    JsonValue v = parseValue();
    skipWs();
    if (pos_ != text_.size()) {
      throw std::invalid_argument("Unexpected trailing JSON content");
    }
    return v;
  }

 private:
  JsonValue parseValue() {
    skipWs();
    if (pos_ >= text_.size()) {
      throw std::invalid_argument("Unexpected end of JSON input");
    }
    const char c = text_[pos_];
    if (c == '{') {
      return JsonValue{parseObject()};
    }
    if (c == '[') {
      return JsonValue{parseArray()};
    }
    if (c == '"') {
      return JsonValue{parseString()};
    }
    if (c == 't') {
      parseLiteral("true");
      return JsonValue{true};
    }
    if (c == 'f') {
      parseLiteral("false");
      return JsonValue{false};
    }
    if (c == 'n') {
      parseLiteral("null");
      return JsonValue{nullptr};
    }
    if (c == '-' || std::isdigit(static_cast<unsigned char>(c)) != 0) {
      return JsonValue{parseNumber()};
    }
    throw std::invalid_argument("Unsupported JSON token");
  }

  JsonObject parseObject() {
    expect('{');
    skipWs();
    JsonObject out;
    if (consume('}')) {
      return out;
    }
    while (true) {
      skipWs();
      std::string key = parseString();
      skipWs();
      expect(':');
      skipWs();
      out.emplace(std::move(key), parseValue());
      skipWs();
      if (consume('}')) {
        break;
      }
      expect(',');
    }
    return out;
  }

  JsonArray parseArray() {
    expect('[');
    skipWs();
    JsonArray out;
    if (consume(']')) {
      return out;
    }
    while (true) {
      out.push_back(parseValue());
      skipWs();
      if (consume(']')) {
        break;
      }
      expect(',');
    }
    return out;
  }

  std::string parseString() {
    expect('"');
    std::string out;
    while (pos_ < text_.size()) {
      const char c = text_[pos_++];
      if (c == '"') {
        return out;
      }
      if (c == '\\') {
        if (pos_ >= text_.size()) {
          throw std::invalid_argument("Invalid JSON string escape");
        }
        const char esc = text_[pos_++];
        switch (esc) {
          case '"':
          case '\\':
          case '/':
            out.push_back(esc);
            break;
          case 'b':
            out.push_back('\b');
            break;
          case 'f':
            out.push_back('\f');
            break;
          case 'n':
            out.push_back('\n');
            break;
          case 'r':
            out.push_back('\r');
            break;
          case 't':
            out.push_back('\t');
            break;
          default:
            throw std::invalid_argument("Unsupported JSON escape sequence");
        }
      } else {
        out.push_back(c);
      }
    }
    throw std::invalid_argument("Unterminated JSON string");
  }

  double parseNumber() {
    const std::size_t start = pos_;
    if (text_[pos_] == '-') {
      ++pos_;
    }
    while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
      ++pos_;
    }
    if (pos_ < text_.size() && text_[pos_] == '.') {
      ++pos_;
      while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
        ++pos_;
      }
    }
    if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
      ++pos_;
      if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) {
        ++pos_;
      }
      while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0) {
        ++pos_;
      }
    }
    return std::stod(text_.substr(start, pos_ - start));
  }

  void parseLiteral(const char* literal) {
    for (std::size_t i = 0; literal[i] != '\0'; ++i) {
      if (pos_ + i >= text_.size() || text_[pos_ + i] != literal[i]) {
        throw std::invalid_argument("Invalid JSON literal");
      }
    }
    pos_ += std::char_traits<char>::length(literal);
  }

  void expect(char c) {
    skipWs();
    if (pos_ >= text_.size() || text_[pos_] != c) {
      throw std::invalid_argument("Unexpected JSON token");
    }
    ++pos_;
  }

  bool consume(char c) {
    skipWs();
    if (pos_ < text_.size() && text_[pos_] == c) {
      ++pos_;
      return true;
    }
    return false;
  }

  void skipWs() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
      ++pos_;
    }
  }

  std::string text_;
  std::size_t pos_{0};
};

const JsonValue& requireField(const JsonObject& object, std::string_view key) {
  const auto it = object.find(std::string(key));
  if (it == object.end()) {
    throw std::invalid_argument("Missing required field: " + std::string(key));
  }
  return it->second;
}

const JsonObject& requireObject(const JsonValue& value, std::string_view key) {
  if (!value.isObject()) {
    throw std::invalid_argument("Field must be an object: " + std::string(key));
  }
  return value.asObject();
}

std::string requireStringField(const JsonObject& object, std::string_view key) {
  const JsonValue& value = requireField(object, key);
  if (!value.isString()) {
    throw std::invalid_argument("Field must be string: " + std::string(key));
  }
  return value.asString();
}

int requireIntField(const JsonObject& object, std::string_view key) {
  const JsonValue& value = requireField(object, key);
  if (!value.isNumber()) {
    throw std::invalid_argument("Field must be number: " + std::string(key));
  }
  const double v = value.asNumber();
  if (std::floor(v) != v) {
    throw std::invalid_argument("Field must be integer-valued: " + std::string(key));
  }
  return static_cast<int>(v);
}

std::optional<double> optionalNumberField(const JsonObject& object, std::string_view key) {
  const auto it = object.find(std::string(key));
  if (it == object.end()) {
    return std::nullopt;
  }
  if (!it->second.isNumber()) {
    throw std::invalid_argument("Field must be number: " + std::string(key));
  }
  return it->second.asNumber();
}

std::optional<std::string> optionalStringField(const JsonObject& object, std::string_view key) {
  const auto it = object.find(std::string(key));
  if (it == object.end()) {
    return std::nullopt;
  }
  if (!it->second.isString()) {
    throw std::invalid_argument("Field must be string: " + std::string(key));
  }
  return it->second.asString();
}

BenchmarkMetricTarget parseMetricTarget(const JsonObject& obj) {
  BenchmarkMetricTarget out;
  out.mean = requireField(obj, "mean").asNumber();
  out.stddev = optionalNumberField(obj, "stddev");
  out.target_min = optionalNumberField(obj, "target_min");
  out.target_max = optionalNumberField(obj, "target_max");
  return out;
}

std::optional<BenchmarkMetricTarget> parseOptionalMetric(const JsonObject& object,
                                                         std::string_view key) {
  const auto it = object.find(std::string(key));
  if (it == object.end()) {
    return std::nullopt;
  }
  if (!it->second.isObject()) {
    throw std::invalid_argument("Metric target must be object: " + std::string(key));
  }
  return parseMetricTarget(it->second.asObject());
}

}  // namespace

BenchmarkProfile loadBenchmarkProfileFromJson(const std::string& json_path) {
  std::ifstream in(json_path);
  if (!in) {
    throw std::invalid_argument("Cannot open benchmark profile JSON: " + json_path);
  }
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

  Parser parser(text);
  const JsonValue root = parser.parse();
  const JsonObject& obj = requireObject(root, "root");

  BenchmarkProfile profile;
  profile.schema_version = requireStringField(obj, "schema_version");
  profile.genre = requireStringField(obj, "genre");
  profile.profile_id = requireStringField(obj, "profile_id");
  profile.created_at_utc = requireStringField(obj, "created_at_utc");
  profile.track_count = requireIntField(obj, "track_count");
  profile.source_notes = optionalStringField(obj, "source_notes");

  const JsonObject& metrics = requireObject(requireField(obj, "metrics"), "metrics");

  if (const auto it = metrics.find("basic"); it != metrics.end()) {
    const JsonObject& g = requireObject(it->second, "metrics.basic");
    profile.metrics.basic.peak_dbfs = parseOptionalMetric(g, "peak_dbfs");
    profile.metrics.basic.rms_dbfs = parseOptionalMetric(g, "rms_dbfs");
    profile.metrics.basic.crest_db = parseOptionalMetric(g, "crest_db");
  }
  if (const auto it = metrics.find("loudness"); it != metrics.end()) {
    const JsonObject& g = requireObject(it->second, "metrics.loudness");
    profile.metrics.loudness.integrated_lufs = parseOptionalMetric(g, "integrated_lufs");
    profile.metrics.loudness.short_term_lufs = parseOptionalMetric(g, "short_term_lufs");
    profile.metrics.loudness.loudness_range_lu = parseOptionalMetric(g, "loudness_range_lu");
  }
  if (const auto it = metrics.find("spectral"); it != metrics.end()) {
    const JsonObject& g = requireObject(it->second, "metrics.spectral");
    profile.metrics.spectral.sub = parseOptionalMetric(g, "sub");
    profile.metrics.spectral.low = parseOptionalMetric(g, "low");
    profile.metrics.spectral.lowmid = parseOptionalMetric(g, "lowmid");
    profile.metrics.spectral.mid = parseOptionalMetric(g, "mid");
    profile.metrics.spectral.highmid = parseOptionalMetric(g, "highmid");
    profile.metrics.spectral.high = parseOptionalMetric(g, "high");
    profile.metrics.spectral.air = parseOptionalMetric(g, "air");
  }
  if (const auto it = metrics.find("stereo"); it != metrics.end()) {
    const JsonObject& g = requireObject(it->second, "metrics.stereo");
    profile.metrics.stereo.correlation = parseOptionalMetric(g, "correlation");
    profile.metrics.stereo.lr_balance_db = parseOptionalMetric(g, "lr_balance_db");
    profile.metrics.stereo.width_proxy = parseOptionalMetric(g, "width_proxy");
  }
  if (const auto it = metrics.find("dynamics"); it != metrics.end()) {
    const JsonObject& g = requireObject(it->second, "metrics.dynamics");
    profile.metrics.dynamics.peak_dbfs = parseOptionalMetric(g, "peak_dbfs");
    profile.metrics.dynamics.rms_dbfs = parseOptionalMetric(g, "rms_dbfs");
    profile.metrics.dynamics.crest_db = parseOptionalMetric(g, "crest_db");
    profile.metrics.dynamics.dr_proxy_db = parseOptionalMetric(g, "dr_proxy_db");
  }

  return profile;
}

}  // namespace aifr3d
