(function attachProfileStore(globalScope) {
  const VERSION = 1;

  function defaultProfile() {
    const now = new Date().toISOString();
    return {
      version: VERSION,
      createdAt: now,
      updatedAt: now,
      preferences: {
        tone: {
          verbosity: 0.5,
          technicality: 0.6,
          warmth: 0.45,
          directness: 0.7,
          humor: 0.15,
          risk_aversion: 0.65
        },
        units: "metric",
        timezone: Intl.DateTimeFormat().resolvedOptions().timeZone || "UTC",
        music_feedback_mode: "pro"
      },
      topics: {
        audio: 0,
        coding: 0,
        planning: 0,
        legal_business: 0,
        file_ops: 0,
        memory: 0
      },
      entities: {
        projects: [],
        tools: []
      }
    };
  }

  function clamp01(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
      return 0;
    }
    return Math.max(0, Math.min(1, number));
  }

  function uniquePush(list, value, maxItems) {
    if (typeof value !== "string" || !value.trim()) {
      return list;
    }
    const trimmed = value.trim();
    if (!list.includes(trimmed)) {
      list.push(trimmed);
    }
    if (list.length > maxItems) {
      list.splice(0, list.length - maxItems);
    }
    return list;
  }

  function sanitizeProfile(input) {
    const base = defaultProfile();
    if (!input || typeof input !== "object") {
      return base;
    }

    const profile = {
      ...base,
      ...input,
      preferences: {
        ...base.preferences,
        ...(input.preferences || {}),
        tone: {
          ...base.preferences.tone,
          ...((input.preferences && input.preferences.tone) || {})
        }
      },
      topics: {
        ...base.topics,
        ...(input.topics || {})
      },
      entities: {
        projects: Array.isArray(input.entities?.projects)
          ? input.entities.projects.filter((item) => typeof item === "string" && item.trim())
          : [],
        tools: Array.isArray(input.entities?.tools)
          ? input.entities.tools.filter((item) => typeof item === "string" && item.trim())
          : []
      }
    };

    for (const key of Object.keys(profile.topics)) {
      profile.topics[key] = clamp01(profile.topics[key]);
    }

    return profile;
  }

  function decayTopicScore(value) {
    return clamp01(value * 0.985);
  }

  function updateFromInteraction(profileInput, options) {
    const profile = sanitizeProfile(profileInput);
    const payload = options && typeof options === "object" ? options : {};
    const intentWeights = payload.intentWeights && typeof payload.intentWeights === "object" ? payload.intentWeights : {};
    const personality = payload.personality && typeof payload.personality === "object" ? payload.personality : profile.preferences.tone;
    const userText = String(payload.userText || "");

    profile.updatedAt = new Date().toISOString();
    profile.preferences.tone = {
      ...profile.preferences.tone,
      ...personality
    };

    profile.topics.audio = decayTopicScore(profile.topics.audio) + clamp01(intentWeights["music/audio"] || 0) * 0.2;
    profile.topics.coding = decayTopicScore(profile.topics.coding) + clamp01(intentWeights["coding/dev"] || 0) * 0.2;
    profile.topics.planning = decayTopicScore(profile.topics.planning) + clamp01(intentWeights.planning || 0) * 0.2;
    profile.topics.legal_business = decayTopicScore(profile.topics.legal_business) + clamp01(intentWeights["legal/business"] || 0) * 0.2;
    profile.topics.file_ops = decayTopicScore(profile.topics.file_ops) + clamp01(intentWeights.file_ops || 0) * 0.2;
    profile.topics.memory = decayTopicScore(profile.topics.memory) + clamp01(intentWeights.memory_recall || 0) * 0.2;

    if (/\b(imperial|fahrenheit|miles|feet|lbs)\b/i.test(userText)) {
      profile.preferences.units = "imperial";
    }

    if (/\b(metric|celsius|kilometers|meters|kg)\b/i.test(userText)) {
      profile.preferences.units = "metric";
    }

    if (/\bcasual feedback|chill feedback\b/i.test(userText)) {
      profile.preferences.music_feedback_mode = "casual";
    }

    if (/\bpro feedback|technical feedback\b/i.test(userText)) {
      profile.preferences.music_feedback_mode = "pro";
    }

    const projectMatch = userText.match(/\b(project|repo|app)\s*[:=-]\s*([A-Za-z0-9._-]{2,})/i);
    if (projectMatch) {
      uniquePush(profile.entities.projects, projectMatch[2], 20);
    }

    const toolMatch = userText.match(/\b(tool|using|with)\s*[:=-]\s*([A-Za-z0-9._ -]{2,})/i);
    if (toolMatch) {
      uniquePush(profile.entities.tools, toolMatch[2], 30);
    }

    for (const key of Object.keys(profile.topics)) {
      profile.topics[key] = clamp01(profile.topics[key]);
    }

    return profile;
  }

  globalScope.AIFREDProfileStore = {
    VERSION,
    defaultProfile,
    sanitizeProfile,
    updateFromInteraction
  };
})(window);
