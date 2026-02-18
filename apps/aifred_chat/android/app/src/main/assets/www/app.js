/* AIFRED app.js (unified desktop + android webview)
 * Features:
 * - Live model verification so ONLY working models appear.
 * - Context memory (localStorage) with rolling window.
 * - Intent parsing + response templating + adaptive phrasing.
 * - Android TTS/STT bridge supported (AIFRED_ANDROID).
 * - Desktop local ops supported (window.desktopApp.ops).
 */

(() => {
  "use strict";

  // ============================================================
  // CONFIG
  // ============================================================

  const CFG = {
    CHAT_TIMEOUT_MS: 90000,
    IMAGE_TIMEOUT_MS: 180000,
    TTS_TIMEOUT_MS: 45000,

    // Pollinations legacy OpenAI-compatible endpoint
    LEGACY_CHAT_ENDPOINT: "https://text.pollinations.ai/openai",
    LEGACY_MODELS_ENDPOINT: "https://text.pollinations.ai/models",

    // Optional: newer authenticated endpoint (ONLY if you add a token in localStorage)
    // NOTE: do NOT commit tokens to GitHub.
    AUTH_CHAT_ENDPOINT: "https://enter.pollinations.ai/openai",
    AUTH_MODELS_ENDPOINT: "https://enter.pollinations.ai/models",

    TEXT_ENDPOINT: "https://text.pollinations.ai",
    IMAGE_ENDPOINT: "https://image.pollinations.ai/prompt",

    DEFAULT_CHAT_MODEL: "openai",
    DEFAULT_IMAGE_MODEL: "flux",
    DEFAULT_TTS_VOICE: "nova",

    IMAGE_MODELS: ["flux", "turbo"],
    TTS_VOICES: ["alloy", "echo", "fable", "onyx", "nova", "shimmer"],

    // Memory
    MEMORY_STORAGE_KEY: "aifred_memory_v1",
    SETTINGS_STORAGE_KEY: "aifred_settings_v1",
    MAX_TURNS: 16, // how many user+assistant turns to keep

    // Model verification
    VERIFY_CONCURRENCY: 3,
    VERIFY_TIMEOUT_MS: 12000,
    VERIFY_TEST_PROMPT: "Reply with: OK",
  };

  // ============================================================
  // DOM
  // ============================================================

  const el = {
    messages: document.getElementById("messages"),
    composer: document.getElementById("composer"),
    prompt: document.getElementById("prompt"),
    send: document.getElementById("send"),
    status: document.getElementById("status"),

    model: document.getElementById("chat-model"),
    modelHint: document.getElementById("model-hint"),
    refreshModels: document.getElementById("refresh-models"),

    stream: document.getElementById("stream"),
    toolCalling: document.getElementById("tool-calling"),
    webSearch: document.getElementById("web-search"),
    temperature: document.getElementById("temperature"),
    maxTokens: document.getElementById("max-tokens"),

    trace: document.getElementById("trace"),
    platformChip: document.getElementById("platform-chip"),

    imageModel: document.getElementById("image-model"),
    imagePrompt: document.getElementById("image-prompt"),
    imageBtn: document.getElementById("image-btn"),

    ttsModel: document.getElementById("tts-model"),
    ttsInput: document.getElementById("tts-input"),
    ttsBtn: document.getElementById("tts-btn"),
    ttsOutput: document.getElementById("tts-output"),

    analyzeUrl: document.getElementById("analyze-url"),
    analyzePrompt: document.getElementById("analyze-prompt"),
    analyzeBtn: document.getElementById("analyze-btn"),

    localOps: document.getElementById("local-ops"),
    fileReadPath: document.getElementById("file-read-path"),
    fileReadBtn: document.getElementById("file-read-btn"),
    fileWritePath: document.getElementById("file-write-path"),
    fileWriteContent: document.getElementById("file-write-content"),
    fileWriteAppend: document.getElementById("file-write-append"),
    fileWriteBtn: document.getElementById("file-write-btn"),
    commandInput: document.getElementById("command-input"),
    commandCwd: document.getElementById("command-cwd"),
    commandBtn: document.getElementById("command-btn"),
  };

  // ============================================================
  // STATE
  // ============================================================

  let busy = false;
  let lastUserPrompt = "";

  const runtime = {
    platform: detectPlatform(),
    provider: "legacy", // "legacy" or "auth"
    token: "", // optional token for auth endpoint
    models: [],
  };

  const memory = {
    // conversation turns: [{role, content}]
    messages: [],
  };

  const speechState = {
    ttsReady: false,
    sttReady: false,
    micOk: false,
  };

  // ============================================================
  // INIT
  // ============================================================

  init();

  function init() {
    loadSettings();
    loadMemory();

    // Fill basic selects
    populateSelect(el.imageModel, CFG.IMAGE_MODELS, CFG.DEFAULT_IMAGE_MODEL);
    populateSelect(el.ttsModel, CFG.TTS_VOICES, CFG.DEFAULT_TTS_VOICE);

    // Platform chip
    if (el.platformChip) {
      el.platformChip.textContent = `platform: ${runtime.platform}`;
    }

    // Events
    if (el.composer) el.composer.addEventListener("submit", onSubmit);
    if (el.refreshModels) el.refreshModels.addEventListener("click", () => void refreshModels(true));
    if (el.model) el.model.addEventListener("change", onModelChanged);

    if (el.imageBtn) el.imageBtn.addEventListener("click", () => void onGenerateImage());
    if (el.ttsBtn) el.ttsBtn.addEventListener("click", () => void onGenerateSpeech());
    if (el.analyzeBtn) el.analyzeBtn.addEventListener("click", () => void onAnalyzeImage());

    if (el.fileReadBtn) el.fileReadBtn.addEventListener("click", () => void onManualReadFile());
    if (el.fileWriteBtn) el.fileWriteBtn.addEventListener("click", () => void onManualWriteFile());
    if (el.commandBtn) el.commandBtn.addEventListener("click", () => void onManualRunCommand());

    // Enter-to-send
    if (el.prompt) {
      el.prompt.addEventListener("keydown", (event) => {
        if (event.key === "Enter" && !event.shiftKey) {
          event.preventDefault();
          el.composer?.dispatchEvent(new Event("submit", { bubbles: true, cancelable: true }));
        }
      });
    }

    // Global error handlers
    window.addEventListener("error", (event) => {
      appendMessage("system", `Error: ${event.message || "unknown"}`);
      setBusy(false);
    });

    window.addEventListener("unhandledrejection", (event) => {
      event.preventDefault();
      appendMessage("system", `Async error: ${describeError(event.reason)}`);
      setBusy(false);
    });

    // Android callbacks (from MainActivity)
    installAndroidCallbacks();

    // Boot messages
    appendMessage("system", "AIFRED online.");
    appendMessage("system", "Tip: Refresh models. Only verified-working models will be shown.");

    setStatus("Initializing...");
    setBusy(false);

    // Load models
    void refreshModels(false).then(() => {
      setStatus("Ready");
      renderMemoryIntoChat();
    });
  }

  // ============================================================
  // PLATFORM + SETTINGS
  // ============================================================

  function detectPlatform() {
    // desktop runtime exposes window.desktopApp.platform
    if (window.desktopApp?.platform) return String(window.desktopApp.platform);
    // android runtime exposes AIFRED_ANDROID object
    if (globalThis.AIFRED_ANDROID) return "android";
    return "web";
  }

  function loadSettings() {
    const raw = safeParseJSON(localStorage.getItem(CFG.SETTINGS_STORAGE_KEY), {});
    runtime.token = typeof raw?.pollinationsToken === "string" ? raw.pollinationsToken.trim() : "";
    runtime.provider = runtime.token ? "auth" : "legacy";
  }

  function saveSettings() {
    const payload = {
      pollinationsToken: runtime.token || "",
    };
    localStorage.setItem(CFG.SETTINGS_STORAGE_KEY, JSON.stringify(payload));
  }

  // ============================================================
  // MEMORY
  // ============================================================

  function loadMemory() {
    const raw = safeParseJSON(localStorage.getItem(CFG.MEMORY_STORAGE_KEY), null);
    if (raw && Array.isArray(raw.messages)) {
      memory.messages = raw.messages.filter(Boolean).slice(-CFG.MAX_TURNS * 2);
    } else {
      memory.messages = [];
    }
  }

  function saveMemory() {
    const payload = {
      messages: memory.messages.slice(-CFG.MAX_TURNS * 2),
    };
    localStorage.setItem(CFG.MEMORY_STORAGE_KEY, JSON.stringify(payload));
  }

  function pushMemory(role, content) {
    memory.messages.push({ role, content: String(content || "") });
    memory.messages = memory.messages.slice(-CFG.MAX_TURNS * 2);
    saveMemory();
  }

  function renderMemoryIntoChat() {
    // If the UI already has messages in DOM, don’t duplicate
    const existing = el.messages?.querySelectorAll(".msg")?.length || 0;
    if (existing > 0) return;

    if (memory.messages.length === 0) return;

    appendMessage("system", `Restored memory (${Math.floor(memory.messages.length / 2)} turns).`);
    for (const m of memory.messages) {
      if (m.role === "user" || m.role === "assistant") {
        appendMessage(m.role, m.content);
      }
    }
  }

  // ============================================================
  // ANDROID BRIDGE (TTS/STT)
  // ============================================================

  function androidBridge() {
    const native = globalThis.AIFRED_ANDROID;
    return {
      native,
      available: !!native,
      hasSpeak: !!native && typeof native.ttsSpeak === "function",
      hasListen: !!native && typeof native.sttStart === "function",
      hasPerm: !!native && typeof native.ensureMicPermission === "function",
    };
  }

  function installAndroidCallbacks() {
    // MainActivity calls window.__AIFRED_* callbacks (not attached to the bridge object)
    window.__AIFRED_onTtsReady = (ok) => {
      speechState.ttsReady = !!ok;
      appendTrace(`[android] TTS ready: ${speechState.ttsReady}`);
    };
    window.__AIFRED_onSttReady = (ok) => {
      speechState.sttReady = !!ok;
      appendTrace(`[android] STT ready: ${speechState.sttReady}`);
    };
    window.__AIFRED_onMicPermission = (ok) => {
      speechState.micOk = !!ok;
      appendTrace(`[android] Mic permission: ${speechState.micOk}`);
    };
    window.__AIFRED_onTtsEvent = (evt) => appendTrace(`[android] TTS event: ${String(evt || "")}`);
    window.__AIFRED_onSttEvent = (evt) => appendTrace(`[android] STT event: ${String(evt || "")}`);
    window.__AIFRED_onSttError = (code) => appendTrace(`[android] STT error: ${code}`);
    window.__AIFRED_onSttResult = (text) => {
      const t = String(text || "");
      appendTrace(`[android] STT result: ${t.slice(0, 120)}`);
      if (el.prompt) {
        el.prompt.value = t;
        el.prompt.focus();
      }
    };
  }

  function ensureMicPermission() {
    const b = androidBridge();
    if (!b.hasPerm) return false;
    try {
      b.native.ensureMicPermission();
      return true;
    } catch (e) {
      appendTrace(`[warn] ensureMicPermission failed: ${describeError(e)}`);
      return false;
    }
  }

  function startListening(langTag = "en-US") {
    const b = androidBridge();
    if (!b.hasListen) {
      appendTrace("[info] STT not available.");
      return;
    }
    try {
      ensureMicPermission();
      b.native.sttStart(String(langTag || "en-US"));
    } catch (e) {
      appendTrace(`[warn] STT start failed: ${describeError(e)}`);
    }
  }

  // ============================================================
  // UI HELPERS
  // ============================================================

  function setBusy(value) {
    busy = !!value;

    if (el.send) {
      el.send.disabled = busy;
      el.send.textContent = busy ? "Sending..." : "Send";
    }
    if (el.refreshModels) el.refreshModels.disabled = busy;

    if (el.imageBtn) el.imageBtn.disabled = busy;
    if (el.ttsBtn) el.ttsBtn.disabled = busy;
    if (el.analyzeBtn) el.analyzeBtn.disabled = busy;

    if (el.fileReadBtn) el.fileReadBtn.disabled = busy;
    if (el.fileWriteBtn) el.fileWriteBtn.disabled = busy;
    if (el.commandBtn) el.commandBtn.disabled = busy;
  }

  function setStatus(text) {
    if (el.status) el.status.textContent = String(text || "");
  }

  function appendTrace(line) {
    if (!el.trace) return;
    const stamp = new Date().toLocaleTimeString();
    const current = el.trace.textContent.trim();
    el.trace.textContent = current ? `${current}\n${stamp} ${line}` : `${stamp} ${line}`;
    el.trace.scrollTop = el.trace.scrollHeight;
  }

  function appendMessage(role, text) {
    if (!el.messages) return null;

    const box = document.createElement("article");
    box.className = `msg ${role}`;

    const roleNode = document.createElement("p");
    roleNode.className = "role";
    roleNode.textContent = role;

    const textNode = document.createElement("p");
    textNode.className = "text";
    textNode.textContent = typeof text === "string" ? text : safeJSONStringify(text);

    box.append(roleNode, textNode);

    if (role === "assistant") {
      const speakBtn = document.createElement("button");
      speakBtn.type = "button";
      speakBtn.className = "secondary";
      speakBtn.style.marginTop = "8px";
      speakBtn.textContent = "Speak";
      speakBtn.addEventListener("click", async () => {
        speakBtn.disabled = true;
        await speakText(textNode.textContent || "");
        speakBtn.disabled = false;
      });
      box.appendChild(speakBtn);

      const b = androidBridge();
      if (b.hasListen) {
        const micBtn = document.createElement("button");
        micBtn.type = "button";
        micBtn.className = "secondary";
        micBtn.style.marginTop = "8px";
        micBtn.style.marginLeft = "8px";
        micBtn.textContent = "Mic";
        micBtn.addEventListener("click", () => startListening("en-US"));
        box.appendChild(micBtn);
      }
    }

    el.messages.appendChild(box);
    el.messages.scrollTop = el.messages.scrollHeight;
    return box;
  }

  function populateSelect(select, options, preferred) {
    if (!select) return;
    const current = select.value;
    select.innerHTML = "";

    const unique = Array.from(new Set((options || []).filter(Boolean))).sort((a, b) => a.localeCompare(b));
    for (const item of unique) {
      const opt = document.createElement("option");
      opt.value = item;
      opt.textContent = item;
      select.appendChild(opt);
    }

    if (unique.includes(current)) select.value = current;
    else if (unique.includes(preferred)) select.value = preferred;
    else if (unique.length > 0) select.selectedIndex = 0;
  }

  // ============================================================
  // INTENT PARSING + RESPONSE TEMPLATING
  // ============================================================

  function parseIntent(userText) {
    const t = String(userText || "").trim().toLowerCase();

    const isQuestion = /\?\s*$/.test(t) || t.startsWith("what") || t.startsWith("how") || t.startsWith("why");
    const isCode = t.includes("```") || t.includes("stack trace") || t.includes("error") || t.includes("exception");
    const isBuild = t.includes("build") || t.includes("make") || t.includes("create") || t.includes("vst3");
    const isPlan = t.includes("plan") || t.includes("steps") || t.includes("roadmap") || t.includes("today");

    if (isCode) return "debug";
    if (isBuild) return "build";
    if (isPlan) return "plan";
    if (isQuestion) return "qa";
    return "chat";
  }

  function buildAssistantStyle(intent) {
    // This guides tone and structure without forcing a rigid template.
    switch (intent) {
      case "debug":
        return "Be surgical. Ask for exact logs if needed. Provide minimal steps + exact code edits.";
      case "build":
        return "Be execution-focused. Give a short plan, then concrete next actions.";
      case "plan":
        return "Be structured. Provide numbered steps and a tiny checklist at the end.";
      case "qa":
        return "Answer clearly, then add one helpful follow-up question.";
      default:
        return "Be natural, helpful, and brief unless user asks for detail.";
    }
  }

  function postProcessAssistantText(rawText, intent) {
    const text = String(rawText || "").trim();
    if (!text) return "";

    // Light templating: if the model outputs a wall, add spacing.
    let out = text.replace(/\n{3,}/g, "\n\n").trim();

    // If debug/build/plan: prefer bulletable structure
    if ((intent === "debug" || intent === "build" || intent === "plan") && out.length > 600) {
      // Add a small separator if none exists
      if (!out.includes("\n\n")) out = out.replace(/\. /g, ".\n\n");
    }

    return out;
  }

  // ============================================================
  // SYSTEM PROMPT (adaptive)
  // ============================================================

  function buildSystemPrompt(intent) {
    const base =
      "You are AIFRED. You are a practical assistant. Be accurate and action-oriented. " +
      "If you are unsure, say so briefly and give the safest next step. " +
      "Do not claim to have done actions you did not do.";

    const style = buildAssistantStyle(intent);

    const memoryRule =
      "You have conversation memory provided. Use it to stay consistent. " +
      "Do not repeat the entire memory. Only use what matters.";

    return `${base}\n\nStyle:\n${style}\n\nMemory:\n${memoryRule}`;
  }

  // ============================================================
  // CHAT OPTIONS
  // ============================================================

  function buildChatOptions() {
    const options = {
      model: el.model?.value || CFG.DEFAULT_CHAT_MODEL,
      stream: Boolean(el.stream?.checked),
      searchMode: Boolean(el.webSearch?.checked),
    };

    const temperature = Number.parseFloat(el.temperature?.value || "");
    if (Number.isFinite(temperature) && temperature >= 0 && temperature <= 1) options.temperature = temperature;

    const maxTokens = Number.parseInt(el.maxTokens?.value || "", 10);
    if (Number.isFinite(maxTokens) && maxTokens > 0) options.max_tokens = maxTokens;

    // Tool calling: keep your calculate tool only (local ops remain desktop-only manual)
    if (el.toolCalling?.checked) {
      options.tools = [
        {
          type: "function",
          function: {
            name: "calculate",
            description: "Perform basic math operations",
            parameters: {
              type: "object",
              properties: {
                operation: { type: "string", enum: ["add", "subtract", "multiply", "divide"] },
                a: { type: "number" },
                b: { type: "number" },
              },
              required: ["operation", "a", "b"],
            },
          },
        },
      ];
      // Most providers behave better non-streaming when tools enabled
      options.stream = false;
    }

    return options;
  }

  function cloneOptionsWithoutTools(options) {
    const copy = { ...(options || {}) };
    delete copy.tools;
    return copy;
  }

  // ============================================================
  // CHAT FLOW
  // ============================================================

  async function onSubmit(event) {
    event.preventDefault();
    if (busy) return;

    const prompt = String(el.prompt?.value || "").trim();
    if (!prompt) return;

    appendMessage("user", prompt);
    pushMemory("user", prompt);
    lastUserPrompt = prompt;
    if (el.prompt) el.prompt.value = "";

    setBusy(true);
    setStatus("Thinking...");

    const intent = parseIntent(prompt);

    try {
      const options = buildChatOptions();

      // 1) main attempt
      let parsed = await withTimeout(callSingleTurn(prompt, options, intent), CFG.CHAT_TIMEOUT_MS, "Chat timed out.");

      // 2) tool calls (calculate only)
      if (parsed.toolCalls.length > 0) {
        const toolLines = [];
        for (const tc of parsed.toolCalls) {
          toolLines.push(await runTool(tc));
        }
        const followup =
          `${prompt}\n\nTool results:\n${toolLines.join("\n")}\n\n` +
          "Now answer the user clearly based on tool results.";
        parsed = await withTimeout(
          callSingleTurn(followup, cloneOptionsWithoutTools(options), intent),
          CFG.CHAT_TIMEOUT_MS,
          "Follow-up timed out."
        );
      }

      // 3) empty safeguard (your screenshot issue)
      if (!String(parsed.text || "").trim()) {
        appendTrace("[warn] empty assistant text; retrying fallback model");
        const fallbackOptions = { ...cloneOptionsWithoutTools(options), model: CFG.DEFAULT_CHAT_MODEL };
        parsed = await withTimeout(callSingleTurn(prompt, fallbackOptions, intent), CFG.CHAT_TIMEOUT_MS, "Retry timed out.");
      }

      const finalText = postProcessAssistantText(parsed.text || "", intent);
      appendMessage("assistant", finalText || "I didn’t get a response from the provider. Try another model.");
      pushMemory("assistant", finalText || "");

      setStatus("Ready");
    } catch (error) {
      appendMessage("system", `Chat failed: ${describeError(error)}`);
      setStatus("Chat failed");
    } finally {
      setBusy(false);
    }
  }

  async function callSingleTurn(userPrompt, options, intent) {
    const systemPrompt = buildSystemPrompt(intent);

    const messages = [
      { role: "system", content: systemPrompt },
      // Memory injection
      ...buildMemoryMessages(),
    ];

    if (options.searchMode) {
      messages.push({
        role: "system",
        content:
          "Web-search response mode is enabled. You do NOT have a live browser tool. " +
          "Answer in a best-effort way and clearly say what might be uncertain.",
      });
    }

    messages.push({ role: "user", content: userPrompt });

    const resp = await callChat(messages, options);
    return parseChatResponse(resp);
  }

  function buildMemoryMessages() {
    // Keep it light: last N messages only
    const slice = memory.messages.slice(-CFG.MAX_TURNS * 2);
    return slice.map((m) => ({ role: m.role, content: m.content }));
  }

  // ============================================================
  // PROVIDER ROUTING
  // ============================================================

  function getChatEndpoint() {
    return runtime.provider === "auth" ? CFG.AUTH_CHAT_ENDPOINT : CFG.LEGACY_CHAT_ENDPOINT;
  }

  function getModelsEndpoint() {
    return runtime.provider === "auth" ? CFG.AUTH_MODELS_ENDPOINT : CFG.LEGACY_MODELS_ENDPOINT;
  }

  function buildHeaders() {
    const h = { "Content-Type": "application/json" };
    // Optional token for enter.pollinations.ai
    if (runtime.provider === "auth" && runtime.token) {
      h["Authorization"] = `Bearer ${runtime.token}`;
    }
    return h;
  }

  // ============================================================
  // MODEL LIST + VERIFICATION
  // ============================================================

  async function refreshModels(userClicked) {
    setStatus("Refreshing models...");
    const endpoint = getModelsEndpoint();

    let rawModels = [];
    try {
      const res = await fetch(endpoint, { method: "GET", headers: { Accept: "application/json", ...(runtime.provider === "auth" ? buildHeaders() : {}) } });
      if (!res.ok) {
        const txt = await res.text();
        throw new Error(`Model list failed (${res.status}): ${txt.slice(0, 220)}`);
      }
      const json = await res.json();
      const rows = Array.isArray(json) ? json : Array.isArray(json?.models) ? json.models : [];
      rawModels = rows
        .map((row) => (typeof row === "string" ? row : row?.id || row?.name || row?.model))
        .filter((id) => typeof id === "string" && id.trim().length > 0)
        .map((id) => id.trim());
    } catch (e) {
      appendTrace(`[warn] model fetch failed: ${describeError(e)}`);
      // fallback minimal list that should usually exist
      rawModels = [CFG.DEFAULT_CHAT_MODEL, "openai-large", "openai-fast", "mistral", "llama", "qwen-coder"].filter(Boolean);
    }

    // Verify models so only working remain
    const verified = await verifyModels(rawModels);

    runtime.models = verified.length > 0 ? verified : [CFG.DEFAULT_CHAT_MODEL];
    populateSelect(el.model, runtime.models, CFG.DEFAULT_CHAT_MODEL);

    setStatus(`Models ready (${runtime.models.length})`);
    if (userClicked) appendMessage("system", `Model list updated. Verified-working models: ${runtime.models.join(", ")}`);
    onModelChanged();
  }

  async function verifyModels(models) {
    const unique = Array.from(new Set((models || []).filter(Boolean)));

    // Don’t spam verification if only 1 model
    if (unique.length <= 1) return unique;

    appendTrace(`[info] verifying ${unique.length} models...`);

    const results = [];
    let i = 0;

    async function worker() {
      while (i < unique.length) {
        const idx = i++;
        const model = unique[idx];
        const ok = await verifySingleModel(model);
        if (ok) results.push(model);
      }
    }

    const workers = [];
    const n = Math.min(CFG.VERIFY_CONCURRENCY, unique.length);
    for (let k = 0; k < n; k++) workers.push(worker());

    await Promise.all(workers);

    results.sort((a, b) => a.localeCompare(b));
    appendTrace(`[info] verified ${results.length}/${unique.length} models`);
    return results;
  }

  async function verifySingleModel(model) {
    try {
      const payload = {
        model,
        messages: [
          { role: "system", content: "Reply with exactly: OK" },
          { role: "user", content: CFG.VERIFY_TEST_PROMPT },
        ],
        stream: false,
      };

      const res = await withTimeout(
        fetch(getChatEndpoint(), { method: "POST", headers: buildHeaders(), body: JSON.stringify(payload) }),
        CFG.VERIFY_TIMEOUT_MS,
        "verify timeout"
      );

      if (!res.ok) {
        const txt = await res.text();
        // Common: 404 Model not found
        appendTrace(`[warn] model rejected: ${model} (${res.status}) ${txt.slice(0, 80)}`);
        return false;
      }

      const json = await res.json().catch(() => ({}));
      const parsed = await parseChatResponse(json);
      const t = String(parsed.text || "").trim().toLowerCase();
      return t.includes("ok");
    } catch (e) {
      appendTrace(`[warn] verify error for ${model}: ${describeError(e)}`);
      return false;
    }
  }

  function onModelChanged() {
    if (!el.modelHint || !el.model) return;
    const m = el.model.value;
    el.modelHint.textContent = `Selected: ${m} (verified for ${runtime.provider} provider)`;
  }

  // ============================================================
  // CHAT REQUEST
  // ============================================================

  async function callChat(messages, options) {
    const payload = {
      model: options.model || CFG.DEFAULT_CHAT_MODEL,
      messages,
      stream: Boolean(options.stream),
    };

    if (Number.isFinite(options.temperature)) payload.temperature = options.temperature;
    if (Number.isFinite(options.max_tokens)) payload.max_tokens = options.max_tokens;

    if (Array.isArray(options.tools) && options.tools.length > 0) {
      payload.tools = options.tools;
      payload.tool_choice = "auto";
    }

    const res = await fetch(getChatEndpoint(), {
      method: "POST",
      headers: buildHeaders(),
      body: JSON.stringify(payload),
    });

    if (!res.ok) {
      const txt = await res.text();

      // Auto fallback on "Model not found"
      if (res.status === 404 && txt.toLowerCase().includes("model not found")) {
        appendTrace(`[warn] Model not found: ${payload.model}. Forcing fallback to ${CFG.DEFAULT_CHAT_MODEL}`);
        if (payload.model !== CFG.DEFAULT_CHAT_MODEL) {
          payload.model = CFG.DEFAULT_CHAT_MODEL;
          const retry = await fetch(getChatEndpoint(), {
            method: "POST",
            headers: buildHeaders(),
            body: JSON.stringify(payload),
          });
          if (!retry.ok) {
            const txt2 = await retry.text();
            throw new Error(`Chat failed (${retry.status}): ${txt2.slice(0, 260)}`);
          }
          return retry.json();
        }
      }

      throw new Error(`Chat failed (${res.status}): ${txt.slice(0, 260)}`);
    }

    if (payload.stream) {
      // some providers may not stream; fallback to json parsing
      const ct = String(res.headers.get("content-type") || "");
      if (ct.includes("text/event-stream")) {
        const text = await readSSEText(res);
        return { choices: [{ message: { content: text } }] };
      }
    }

    return res.json();
  }

  async function readSSEText(response) {
    if (!response?.body || typeof response.body.getReader !== "function") return "";
    const reader = response.body.getReader();
    const decoder = new TextDecoder("utf-8");
    let buffered = "";
    let text = "";

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;

      buffered += decoder.decode(value, { stream: true });
      const lines = buffered.split("\n");
      buffered = lines.pop() || "";

      for (const line of lines) {
        const trimmed = line.trim();
        if (!trimmed.startsWith("data:")) continue;

        const data = trimmed.slice(5).trim();
        if (!data || data === "[DONE]") continue;

        try {
          const chunk = JSON.parse(data);
          const delta = chunk?.choices?.[0]?.delta;
          if (typeof delta?.content === "string") text += delta.content;
        } catch (_) {}
      }
    }

    return text.trim();
  }

  // ============================================================
  // RESPONSE PARSING
  // ============================================================

  async function parseChatResponse(response) {
    // Handles OpenAI-like responses and also loose shapes
    const choice = Array.isArray(response?.choices) ? response.choices[0] : null;
    const message = choice?.message || response?.message || response;

    const text = normalizeContent(message?.content ?? response?.output_text ?? response?.text ?? "");
    const toolCalls = Array.isArray(message?.tool_calls) ? message.tool_calls : [];

    return {
      text,
      toolCalls: toolCalls.map((tc) => ({
        id: tc?.id || `tool_${Math.random().toString(16).slice(2)}`,
        function: {
          name: tc?.function?.name || "",
          arguments: typeof tc?.function?.arguments === "string" ? tc.function.arguments : safeJSONStringify(tc?.function?.arguments || {}),
        },
      })),
    };
  }

  function normalizeContent(content) {
    if (typeof content === "string") return content;
    if (Array.isArray(content)) {
      return content
        .map((item) => {
          if (typeof item === "string") return item;
          if (typeof item?.text === "string") return item.text;
          if (typeof item?.content === "string") return item.content;
          return safeJSONStringify(item);
        })
        .join("\n")
        .trim();
    }
    if (content == null) return "";
    return safeJSONStringify(content);
  }

  // ============================================================
  // TOOL CALLS (calculate)
  // ============================================================

  async function runTool(toolCall) {
    const name = toolCall?.function?.name || "";
    const args = safeParseJSON(toolCall?.function?.arguments, {});
    appendTrace(`[tool] ${name} ${safeJSONStringify(args)}`);

    try {
      if (name === "calculate") {
        return runCalculate(args);
      }
      return safeJSONStringify({ error: `Unsupported tool: ${name}` });
    } catch (e) {
      return safeJSONStringify({ error: String(e?.message || e) });
    }
  }

  function runCalculate(args) {
    const op = String(args.operation || "").toLowerCase();
    const a = Number(args.a);
    const b = Number(args.b);
    if (!Number.isFinite(a) || !Number.isFinite(b)) throw new Error("Invalid numbers");

    let result;
    if (op === "add") result = a + b;
    else if (op === "subtract") result = a - b;
    else if (op === "multiply") result = a * b;
    else if (op === "divide") {
      if (b === 0) throw new Error("Division by zero");
      result = a / b;
    } else {
      throw new Error("Unknown operation");
    }
    return safeJSONStringify({ operation: op, a, b, result });
  }

  // ============================================================
  // IMAGE GENERATION
  // ============================================================

  async function onGenerateImage() {
    if (busy) return;
    const prompt = String(el.imagePrompt?.value || "").trim();
    if (!prompt) return setStatus("Enter image prompt first");

    setBusy(true);
    setStatus("Generating image...");

    try {
      const model = el.imageModel?.value || CFG.DEFAULT_IMAGE_MODEL;
      const params = new URLSearchParams({ model, width: "1024", height: "1024" });
      const url = `${CFG.IMAGE_ENDPOINT}/${encodeURIComponent(prompt)}?${params.toString()}`;

      const bubble = appendMessage("assistant", `Image generated using ${model}`);
      const img = document.createElement("img");
      img.src = url;
      img.alt = "Generated image";
      img.classList.add("generated");
      bubble?.appendChild(img);

      setStatus("Image ready");
    } catch (e) {
      appendMessage("system", `Image failed: ${describeError(e)}`);
      setStatus("Image failed");
    } finally {
      setBusy(false);
    }
  }

  // ============================================================
  // SPEECH (prefer Android native)
  // ============================================================

  function buildSpeechUrl(text, voice) {
    const q = new URLSearchParams({ model: "openai-audio", voice: voice || CFG.DEFAULT_TTS_VOICE });
    return `${CFG.TEXT_ENDPOINT}/${encodeURIComponent(text)}?${q.toString()}`;
  }

  async function speakText(text) {
    const t = String(text || "").trim();
    if (!t) return;

    const b = androidBridge();
    if (b.hasSpeak) {
      try {
        b.native.ttsSpeak(t, 1.0, 1.0);
        return;
      } catch (e) {
        appendTrace(`[warn] Android TTS failed: ${describeError(e)}`);
      }
    }

    // Fallback: audio URL
    const url = buildSpeechUrl(t, el.ttsModel?.value || CFG.DEFAULT_TTS_VOICE);
    if (el.ttsOutput) el.ttsOutput.innerHTML = "";

    const audio = document.createElement("audio");
    audio.src = url;
    audio.setAttribute("controls", "");
    el.ttsOutput?.appendChild(audio);

    try {
      await withTimeout(audio.play(), CFG.TTS_TIMEOUT_MS, "Speech playback timed out");
    } catch (_) {}
  }

  async function onGenerateSpeech() {
    if (busy) return;
    const t = String(el.ttsInput?.value || "").trim();
    if (!t) return setStatus("Enter text for speech first");

    setBusy(true);
    setStatus("Speaking...");
    try {
      await speakText(t);
      setStatus("Ready");
    } catch (e) {
      appendMessage("system", `Speech failed: ${describeError(e)}`);
      setStatus("Speech failed");
    } finally {
      setBusy(false);
    }
  }

  // ============================================================
  // IMAGE ANALYSIS (URL)
  // ============================================================

  async function onAnalyzeImage() {
    if (busy) return;

    const imageUrl = String(el.analyzeUrl?.value || "").trim();
    const question = String(el.analyzePrompt?.value || "").trim() || "What do you see in this image?";
    if (!imageUrl) return setStatus("Enter image URL first");

    setBusy(true);
    setStatus("Analyzing image...");

    try {
      appendMessage("user", `[Analyze Image] ${imageUrl} | ${question}`);

      const options = cloneOptionsWithoutTools(buildChatOptions());
      options.stream = false;

      const intent = "qa";
      const messages = [
        { role: "system", content: buildSystemPrompt(intent) },
        ...buildMemoryMessages(),
        {
          role: "user",
          content: [
            { type: "text", text: question },
            { type: "image_url", image_url: { url: imageUrl } },
          ],
        },
      ];

      const resp = await callChat(messages, options);
      const parsed = await parseChatResponse(resp);

      const finalText = postProcessAssistantText(parsed.text || "", intent);
      appendMessage("assistant", finalText || "No analysis returned.");
      setStatus("Ready");
    } catch (e) {
      appendMessage("system", `Image analysis failed: ${describeError(e)}`);
      setStatus("Image analysis failed");
    } finally {
      setBusy(false);
    }
  }

  // ============================================================
  // DESKTOP LOCAL OPS (manual only)
  // ============================================================

  function requireLocalOpsEnabled() {
    if (!el.localOps?.checked) throw new Error("Local tools are disabled.");
    if (!window.desktopApp?.ops) throw new Error("Desktop ops not available in this runtime.");
  }

  function confirmAction(message) {
    return window.confirm(message);
  }

  async function onManualReadFile() {
    if (busy) return;
    try {
      requireLocalOpsEnabled();
      const path = String(el.fileReadPath?.value || "").trim();
      if (!path) throw new Error("Enter a file path first.");
      if (!confirmAction(`Read file?\n\nPath: ${path}`)) return;

      setBusy(true);
      setStatus("Reading file...");

      const res = await window.desktopApp.ops.readFile({ path });
      if (!res?.ok) throw new Error(res?.error || "Read failed");

      appendMessage("assistant", `File read: ${path}\n\n${String(res.content || "")}`);
      setStatus("Ready");
    } catch (e) {
      appendMessage("system", `Read failed: ${describeError(e)}`);
      setStatus("Read failed");
    } finally {
      setBusy(false);
    }
  }

  async function onManualWriteFile() {
    if (busy) return;
    try {
      requireLocalOpsEnabled();
      const path = String(el.fileWritePath?.value || "").trim();
      const content = String(el.fileWriteContent?.value || "");
      const append = Boolean(el.fileWriteAppend?.checked);
      if (!path) throw new Error("Enter a target file path.");
      if (!content) throw new Error("Enter file content to write.");
      if (!confirmAction(`Write file?\n\nPath: ${path}\nMode: ${append ? "append" : "overwrite"}`)) return;

      setBusy(true);
      setStatus("Writing file...");

      const res = await window.desktopApp.ops.writeFile({ path, content, append });
      if (!res?.ok) throw new Error(res?.error || "Write failed");

      appendMessage("assistant", `File written: ${path}`);
      setStatus("Ready");
    } catch (e) {
      appendMessage("system", `Write failed: ${describeError(e)}`);
      setStatus("Write failed");
    } finally {
      setBusy(false);
    }
  }

  async function onManualRunCommand() {
    if (busy) return;
    try {
      requireLocalOpsEnabled();
      const command = String(el.commandInput?.value || "").trim();
      const cwd = String(el.commandCwd?.value || "").trim();
      if (!command) throw new Error("Enter a command first.");
      if (!confirmAction(`Run command?\n\nCommand: ${command}\nCWD: ${cwd || "(default)"}`)) return;

      setBusy(true);
      setStatus("Running command...");

      const res = await window.desktopApp.ops.runCommand({ command, cwd: cwd || undefined });
      const summary = [
        `Command: ${command}`,
        `Exit: ${res?.code}`,
        res?.stdout ? `stdout:\n${res.stdout}` : "",
        res?.stderr ? `stderr:\n${res.stderr}` : "",
      ].filter(Boolean).join("\n\n");

      appendMessage(res?.ok ? "assistant" : "system", summary || "(No output)");
      setStatus("Ready");
    } catch (e) {
      appendMessage("system", `Command failed: ${describeError(e)}`);
      setStatus("Command failed");
    } finally {
      setBusy(false);
    }
  }

  // ============================================================
  // UTILS
  // ============================================================

  function withTimeout(promise, ms, message) {
    return new Promise((resolve, reject) => {
      const t = setTimeout(() => reject(new Error(message)), ms);
      Promise.resolve(promise)
        .then((v) => { clearTimeout(t); resolve(v); })
        .catch((e) => { clearTimeout(t); reject(e); });
    });
  }

  function describeError(error) {
    const e = normalizeError(error);
    const msg = (e?.message || String(e || "")).toLowerCase();

    if (msg.includes("failed to fetch") || msg.includes("network") || msg.includes("cors") || msg.includes("dns")) {
      return "Network request failed (internet / endpoint / CORS).";
    }
    if (msg.includes("forbidden") || msg.includes("403") || msg.includes("unauthorized") || msg.includes("not authorized")) {
      return "Request denied (auth or provider restriction).";
    }
    return e?.message || String(e || "Unknown error");
  }

  function normalizeError(error) {
    if (error instanceof Error) return error;
    if (typeof error === "string") return new Error(error);
    if (!error) return new Error("Unknown error");
    if (typeof error === "object") {
      const m = error.message || error.error || error.reason || safeJSONStringify(error);
      return new Error(String(m));
    }
    return new Error(String(error));
  }

  function safeParseJSON(raw, fallback) {
    try {
      if (raw == null) return fallback;
      if (typeof raw === "object") return raw;
      if (typeof raw !== "string") return fallback;
      return JSON.parse(raw);
    } catch (_) {
      return fallback;
    }
  }

  function safeJSONStringify(v) {
    try { return JSON.stringify(v); } catch (_) { return String(v); }
  }
})();