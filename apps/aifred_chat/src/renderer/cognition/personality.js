(function attachPersonalityModule(globalScope) {
  const VERSION = 1;
  const DEFAULT_VECTOR = {
    verbosity: 0.5,
    technicality: 0.6,
    warmth: 0.45,
    directness: 0.7,
    humor: 0.15,
    risk_aversion: 0.65
  };

  function clamp01(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
      return 0;
    }
    return Math.min(1, Math.max(0, number));
  }

  function normalizeVector(input) {
    const source = input && typeof input === "object" ? input : {};
    return {
      verbosity: clamp01(source.verbosity ?? DEFAULT_VECTOR.verbosity),
      technicality: clamp01(source.technicality ?? DEFAULT_VECTOR.technicality),
      warmth: clamp01(source.warmth ?? DEFAULT_VECTOR.warmth),
      directness: clamp01(source.directness ?? DEFAULT_VECTOR.directness),
      humor: clamp01(source.humor ?? DEFAULT_VECTOR.humor),
      risk_aversion: clamp01(source.risk_aversion ?? DEFAULT_VECTOR.risk_aversion)
    };
  }

  function applyDelta(vector, key, delta) {
    if (!Object.prototype.hasOwnProperty.call(vector, key)) {
      return;
    }
    vector[key] = clamp01(vector[key] + delta);
  }

  function updateVector(currentVector, options) {
    const next = normalizeVector(currentVector);
    const payload = options && typeof options === "object" ? options : {};
    const userText = String(payload.userText || "").toLowerCase();
    const messageLength = String(payload.userText || "").trim().length;
    const followupCount = Number.isFinite(payload.followupCount) ? Number(payload.followupCount) : 0;
    const feedback = payload.feedback === "up" || payload.feedback === "down" ? payload.feedback : null;

    if (/\b(short|brief|concise|tldr|tl;dr|one[- ]liner)\b/.test(userText)) {
      applyDelta(next, "verbosity", -0.08);
      applyDelta(next, "directness", 0.05);
    }

    if (/\b(detailed|more detail|thorough|step[- ]by[- ]step|in depth|deep dive)\b/.test(userText)) {
      applyDelta(next, "verbosity", 0.09);
      applyDelta(next, "technicality", 0.04);
    }

    if (/\b(stop being technical|less technical|simpler|plain english)\b/.test(userText)) {
      applyDelta(next, "technicality", -0.08);
      applyDelta(next, "warmth", 0.03);
    }

    if (/\b(be brutal|be direct|straight to the point|no fluff|blunt)\b/.test(userText)) {
      applyDelta(next, "directness", 0.1);
      applyDelta(next, "warmth", -0.04);
    }

    if (/\b(friendly|supportive|encouraging|warm)\b/.test(userText)) {
      applyDelta(next, "warmth", 0.08);
      applyDelta(next, "directness", -0.02);
    }

    if (/\b(joke|funny|humor|lighten up)\b/.test(userText)) {
      applyDelta(next, "humor", 0.06);
    }

    if (/\b(careful|safe|risk|double[- ]check|conservative)\b/.test(userText)) {
      applyDelta(next, "risk_aversion", 0.06);
    }

    if (messageLength > 600) {
      applyDelta(next, "technicality", 0.03);
      applyDelta(next, "verbosity", 0.02);
    } else if (messageLength > 0 && messageLength < 60) {
      applyDelta(next, "verbosity", -0.02);
      applyDelta(next, "directness", 0.01);
    }

    if (followupCount > 0) {
      applyDelta(next, "technicality", Math.min(0.05, followupCount * 0.01));
    }

    if (feedback === "up") {
      applyDelta(next, "warmth", 0.02);
      applyDelta(next, "risk_aversion", 0.01);
    }

    if (feedback === "down") {
      applyDelta(next, "directness", 0.02);
      applyDelta(next, "verbosity", -0.01);
    }

    return next;
  }

  function buildPrompt(vector) {
    const v = normalizeVector(vector);
    return (
      "Personality profile: " +
      JSON.stringify(v) +
      ". Instruction: match these traits in response style while preserving factual accuracy."
    );
  }

  function resetVector() {
    return normalizeVector(DEFAULT_VECTOR);
  }

  globalScope.AIFREDPersonality = {
    VERSION,
    DEFAULT_VECTOR,
    normalizeVector,
    updateVector,
    buildPrompt,
    resetVector
  };
})(window);
