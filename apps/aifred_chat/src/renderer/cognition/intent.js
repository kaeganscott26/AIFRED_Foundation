(function attachIntentModule(globalScope) {
  const INTENT_KEYS = [
    "general",
    "coding/dev",
    "troubleshooting",
    "planning",
    "music/audio",
    "legal/business",
    "file_ops",
    "memory_recall"
  ];

  const RULES = {
    "coding/dev": [
      /```/,
      /\b(function|class|typescript|javascript|python|java|kotlin|golang|rust|sql|regex)\b/i,
      /\b(src\/|package\.json|npm|gradle|build|compile|refactor|api|backend|frontend)\b/i,
      /\b(stack trace|traceback|exception|error:|failed|failing|broken)\b/i
    ],
    troubleshooting: [
      /\bdebug|troubleshoot|investigate|root cause|fix build|not working|fails\b/i,
      /\b422\b|\b502\b|\b404\b|\b500\b/i,
      /\bcrash|hang|timeout|deadlock|regression\b/i
    ],
    planning: [/\bplan|roadmap|milestone|phase|timeline|priorit|scope|estimate\b/i],
    "music/audio": [/\bmix|master|lufs|eq|compressor|reverb|track|stem|bpm|audio\b/i],
    "legal/business": [/\bcontract|invoice|nda|llc|tax|liability|terms|policy|compliance\b/i],
    file_ops: [/\bfile|folder|directory|scan|import|export|upload|download|vault\b/i],
    memory_recall: [/\bremember|recall|memory|vault|earlier|last time|what did i say\b/i],
    general: []
  };

  function emptyWeights() {
    const weights = {};
    for (const key of INTENT_KEYS) {
      weights[key] = key === "general" ? 0.2 : 0;
    }
    return weights;
  }

  function normalizeWeights(rawWeights) {
    const weights = emptyWeights();
    let sum = 0;

    for (const key of INTENT_KEYS) {
      const value = Number(rawWeights[key]);
      weights[key] = Number.isFinite(value) && value > 0 ? value : 0;
      sum += weights[key];
    }

    if (sum <= 0) {
      weights.general = 1;
      return weights;
    }

    for (const key of INTENT_KEYS) {
      weights[key] = weights[key] / sum;
    }

    return weights;
  }

  function detect(text) {
    const source = String(text || "");
    const weights = emptyWeights();

    for (const [intent, checks] of Object.entries(RULES)) {
      for (const rule of checks) {
        if (rule.test(source)) {
          weights[intent] += 1;
        }
      }
    }

    if (/\b(step|steps|first|then|after)\b/i.test(source)) {
      weights.planning += 0.4;
    }

    if (/\b(scan repo|fix build|stack trace|error log)\b/i.test(source)) {
      weights["coding/dev"] += 0.8;
      weights.troubleshooting += 0.8;
    }

    if (/\bmix|lufs|master|reference tracks\b/i.test(source)) {
      weights["music/audio"] += 0.9;
    }

    if (/\bsearch web|latest|today|news\b/i.test(source)) {
      weights.general += 0.2;
    }

    const normalized = normalizeWeights(weights);

    const sorted = Object.entries(normalized)
      .sort((a, b) => b[1] - a[1])
      .map(([name, score]) => ({ name, score }));

    const primary = sorted[0] || { name: "general", score: 1 };
    const secondary = sorted[1] || null;

    const isPrivate = /\b(private|confidential|secret|password|api key|personal|my docs|my files)\b/i.test(source);
    const wantsWebSearch = /\b(web search|search web|browse|look up online|latest news)\b/i.test(source);

    return {
      primary,
      secondary,
      weights: normalized,
      isPrivate,
      wantsWebSearch,
      wantsTools: normalized.file_ops > 0.2 || normalized["coding/dev"] > 0.4 || normalized.troubleshooting > 0.35
    };
  }

  globalScope.AIFREDIntent = {
    INTENT_KEYS,
    detect,
    normalizeWeights
  };
})(window);
