(function attachRouterModule(globalScope) {
  function chooseRoute(options) {
    const payload = options && typeof options === "object" ? options : {};
    const localMode = Boolean(payload.localMode);
    const localReachable = Boolean(payload.localReachable);
    const legacyMode = Boolean(payload.legacyMode);
    const preferLocalPrivate = Boolean(payload.preferLocalPrivate);
    const allowCloudPrivate = Boolean(payload.allowCloudPrivate);
    const hasOpenAIKey = Boolean(payload.hasOpenAIKey);
    const isPrivate = Boolean(payload.isPrivate);
    const wantsWebSearch = Boolean(payload.wantsWebSearch);

    if (legacyMode && !hasOpenAIKey) {
      return {
        chosen: "legacy",
        reason: "legacy mode enabled and OpenAI key missing"
      };
    }

    if (localMode && localReachable) {
      if (isPrivate && preferLocalPrivate) {
        return {
          chosen: "local",
          reason: "private request with prefer-local-private enabled"
        };
      }

      if (!wantsWebSearch) {
        return {
          chosen: "local",
          reason: "local mode enabled and local endpoint reachable"
        };
      }
    }

    if (isPrivate && !allowCloudPrivate && localMode && !localReachable) {
      if (legacyMode) {
        return {
          chosen: "legacy",
          reason: "private request blocked from cloud and local unavailable"
        };
      }

      return {
        chosen: "blocked",
        reason: "private request blocked from cloud and local unavailable"
      };
    }

    if (hasOpenAIKey) {
      return {
        chosen: "cloud",
        reason: wantsWebSearch ? "web search intent prefers cloud" : "default cloud routing"
      };
    }

    if (legacyMode) {
      return {
        chosen: "legacy",
        reason: "cloud key unavailable, legacy enabled"
      };
    }

    return {
      chosen: "blocked",
      reason: "no available route"
    };
  }

  globalScope.AIFREDRouter = {
    chooseRoute
  };
})(window);
