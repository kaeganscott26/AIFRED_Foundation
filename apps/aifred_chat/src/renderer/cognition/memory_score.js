(function attachMemoryScoreModule(globalScope) {
  function clampScore(value) {
    const number = Number(value);
    if (!Number.isFinite(number)) {
      return 0;
    }
    return Math.max(0, Math.min(100, number));
  }

  function daysSince(isoString) {
    if (typeof isoString !== "string" || !isoString) {
      return 365;
    }

    const then = new Date(isoString).getTime();
    if (!Number.isFinite(then)) {
      return 365;
    }

    const deltaMs = Date.now() - then;
    return Math.max(0, deltaMs / (24 * 60 * 60 * 1000));
  }

  function computeIntentMatchBoost(item, intentName) {
    if (!intentName) {
      return 0;
    }

    const normalizedIntent = String(intentName).toLowerCase();
    const summary = String(item.summaryText || "").toLowerCase();
    const tags = Array.isArray(item.tags) ? item.tags.join(" ").toLowerCase() : "";
    const type = String(item.type || "other").toLowerCase();

    if (normalizedIntent === "music/audio") {
      if (type === "audio" || type === "video" || /mix|master|lufs|audio|song|track/.test(summary + " " + tags)) {
        return 14;
      }
    }

    if (normalizedIntent === "coding/dev") {
      if (type === "text" && /code|error|build|bug|repo|stack/.test(summary + " " + tags)) {
        return 12;
      }
    }

    if (normalizedIntent === "memory_recall") {
      return 10;
    }

    return 4;
  }

  function computeScore(item, options) {
    const payload = options && typeof options === "object" ? options : {};
    const base = 10;

    const recencyDays = daysSince(item.updatedAt || item.createdAt);
    const recencyBoost = Math.max(0, 34 * Math.exp(-recencyDays / 20));

    const frequency = Number.isFinite(item.referenceCount) ? item.referenceCount : 0;
    const frequencyBoost = Math.min(22, frequency * 3);

    const pinnedBoost = item.pinned ? 18 : 0;
    const likedBoost = item.userSignal === "liked" ? 9 : 0;
    const dislikedPenalty = item.userSignal === "disliked" ? -18 : 0;
    const hiddenPenalty = item.hidden ? -30 : 0;
    const penalty = item.forget ? -100 : 0;

    const intentMatchBoost = computeIntentMatchBoost(item, payload.intentPrimary);

    return clampScore(base + recencyBoost + frequencyBoost + pinnedBoost + likedBoost + intentMatchBoost + dislikedPenalty + hiddenPenalty + penalty);
  }

  function applyReference(item) {
    const next = { ...item };
    next.referenceCount = (Number.isFinite(next.referenceCount) ? next.referenceCount : 0) + 1;
    next.updatedAt = new Date().toISOString();
    next.score = computeScore(next, {});
    return next;
  }

  function applyFeedback(item, kind) {
    const next = { ...item };
    if (kind === "positive") {
      next.userSignal = "liked";
    } else if (kind === "negative") {
      next.userSignal = "disliked";
    }

    next.updatedAt = new Date().toISOString();
    next.score = computeScore(next, {});
    return next;
  }

  function rank(items, options) {
    const payload = options && typeof options === "object" ? options : {};
    const list = Array.isArray(items) ? items : [];
    return list
      .map((item) => {
        const score = computeScore(item, payload);
        return {
          ...item,
          score
        };
      })
      .sort((a, b) => b.score - a.score);
  }

  globalScope.AIFREDMemoryScore = {
    computeScore,
    applyReference,
    applyFeedback,
    rank
  };
})(window);
