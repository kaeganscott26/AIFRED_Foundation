#pragma once

#include <string>

namespace aifr3d::ai {

struct PromptContext {
  std::string user_prompt;
  std::string session_id;
};

struct CompletionResult {
  std::string response_text;
  bool deterministic{true};
};

class ICompletionProvider {
 public:
  virtual ~ICompletionProvider() = default;
  virtual CompletionResult complete(const PromptContext& context) = 0;
};

}  // namespace aifr3d::ai
