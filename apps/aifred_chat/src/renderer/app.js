const OPENAI_CHAT_ENDPOINT = "https://api.openai.com/v1/chat/completions";
const OPENAI_TTS_ENDPOINT = "https://api.openai.com/v1/audio/speech";
const LEGACY_ANON_CHAT_ENDPOINT = "https://text.pollinations.ai/openai";
const LEGACY_ANON_TTS_ENDPOINT = "https://text.pollinations.ai/openai/audio/speech";

const SUPPORTED_CHAT_MODELS = ["gpt-4o-mini", "gpt-4o", "gpt-4.1-mini", "gpt-4.1"];
const DEFAULT_CHAT_MODEL = "gpt-4o-mini";
const FALLBACK_CHAT_MODEL = "gpt-4o-mini";
const DEFAULT_TTS_MODEL = "gpt-4o-mini-tts";

const CHAT_TIMEOUT_MS = 70_000;
const TTS_TIMEOUT_MS = 30_000;
const LOCAL_TEST_TIMEOUT_MS = 8_000;
const MAX_TOOL_ROUNDS = 6;
const MAX_VISIBLE_MESSAGES = 30;
const MAX_HISTORY_MESSAGES = 200;
const MAX_CONTEXT_MESSAGES = 42;
const MAX_MEMORY_SUMMARY_CHARS = 6_000;
const MAX_TRACE_LINES = 320;
const ANDROID_MAX_BASE64_BYTES = 5 * 1024 * 1024;

const CHAT_TEMPERATURE = 0.3;
const CHAT_MAX_TOKENS = 1200;

const STORAGE_KEYS = {
  history: "aifred_history_v3",
  memorySummary: "aifred_memory_summary_v3",
  stylePrefs: "aifred_style_prefs_v3",
  lastIntent: "aifred_last_intent_v3",
  selectedModel: "aifred_selected_model_v3",
  autoSpeak: "aifred_auto_speak_v3",
  webSearch: "aifred_web_search_v3",
  localTools: "aifred_local_tools_v3",
  ttsModel: "aifred_tts_model_v3",
  personalityEnabled: "aifred_personality_enabled_v1",
  personalityVector: "aifred_personality_vector_v1",
  useVaultContext: "aifred_use_vault_context_v1",
  vaultIndex: "aifred_vault_index_v1",
  routeLocalMode: "aifred_route_local_mode_v1",
  routeLocalEndpoint: "aifred_route_local_endpoint_v1",
  routePreferLocalPrivate: "aifred_route_prefer_local_private_v1",
  routeAllowCloudPrivate: "aifred_route_allow_cloud_private_v1",
  routeLegacyMode: "aifred_route_legacy_mode_v1",
  profile: "aifred_profile_v1"
};

const STYLE_DEFAULTS = {
  verbosity: "balanced",
  tone: "professional",
  baseline: "Sharp by default, reassuring, objective"
};

const TEMPLATE_DIRECTIVES = {
  technical:
    "Template directive: Goal -> Steps -> Notes/Pitfalls -> Next action. Keep it concrete and execution-oriented.",
  explain:
    "Template directive: Concept -> Example -> Common mistake -> Quick check. Teach clearly without filler.",
  music:
    "Template directive: What you're hearing -> Why -> Do this next -> Avoid. Focus on actionable mix decisions.",
  general: "Template directive: concise answer with one practical next action."
};

const LOCAL_TOOLS = [
  {
    type: "function",
    function: {
      name: "get_current_time",
      description: "Get current date and time for a requested IANA timezone.",
      parameters: {
        type: "object",
        properties: {
          timezone: {
            type: "string",
            description: "IANA timezone, for example America/New_York or UTC"
          }
        },
        required: ["timezone"]
      }
    }
  },
  {
    type: "function",
    function: {
      name: "calculate_expression",
      description: "Evaluate a basic arithmetic expression.",
      parameters: {
        type: "object",
        properties: {
          expression: {
            type: "string",
            description: "Arithmetic expression using numbers and + - * / ( ) ^"
          }
        },
        required: ["expression"]
      }
    }
  },
  {
    type: "function",
    function: {
      name: "roll_dice",
      description: "Roll one or more dice and return individual rolls and total.",
      parameters: {
        type: "object",
        properties: {
          sides: {
            type: "integer",
            description: "Number of sides per die"
          },
          count: {
            type: "integer",
            description: "How many dice to roll"
          }
        },
        required: ["sides", "count"]
      }
    }
  },
  {
    type: "function",
    function: {
      name: "scan_folder",
      description:
        "Scan a desktop folder for files and ingest matched items into Memory Vault. Requires explicit user confirmation.",
      parameters: {
        type: "object",
        properties: {
          path: {
            type: "string",
            description: "Absolute folder path on desktop"
          },
          recursive: {
            type: "boolean",
            description: "Whether to recursively scan subfolders"
          },
          fileTypes: {
            type: "array",
            items: {
              type: "string",
              enum: ["text", "image", "audio", "video", "pdf", "other"]
            },
            description: "File categories to include"
          }
        },
        required: ["path", "recursive", "fileTypes"]
      }
    }
  }
];

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

    return {
      timezone,
      formatted: formatter.format(now),
      iso_utc: now.toISOString()
    };
  },

  async calculate_expression(args) {
    const expression = typeof args.expression === "string" ? args.expression.trim() : "";
    if (!expression) {
      throw new Error("expression is required");
    }

    if (expression.length > 200) {
      throw new Error("expression is too long");
    }

    if (!/^[0-9+\-*/().\s%^]+$/.test(expression)) {
      throw new Error("expression contains unsupported characters");
    }

    const normalized = expression.replace(/\^/g, "**");
    const result = Function(`\"use strict\"; return (${normalized});`)();

    if (typeof result !== "number" || !Number.isFinite(result)) {
      throw new Error("expression result is not a finite number");
    }

    return {
      expression,
      result
    };
  },

  async roll_dice(args) {
    const sides = clampInteger(args.sides, 2, 1000, 6);
    const count = clampInteger(args.count, 1, 20, 1);
    const rolls = Array.from({ length: count }, () => Math.floor(Math.random() * sides) + 1);
    const total = rolls.reduce((sum, value) => sum + value, 0);

    return {
      sides,
      count,
      rolls,
      total
    };
  },

  async scan_folder(args) {
    if (state.isAndroid) {
      throw new Error("scan_folder is desktop-only");
    }

    if (!window.desktopApp || typeof window.desktopApp.scanFolder !== "function") {
      throw new Error("Desktop scan bridge unavailable");
    }

    const path = typeof args.path === "string" ? args.path.trim() : "";
    if (!path) {
      throw new Error("path is required");
    }

    const recursive = Boolean(args.recursive);
    const fileTypes = Array.isArray(args.fileTypes) ? args.fileTypes : ["text", "image", "audio", "video", "pdf", "other"];

    const scanResult = await window.desktopApp.scanFolder({
      path,
      recursive,
      fileTypes
    });

    const matchedPaths = Array.isArray(scanResult?.matchedPaths) ? scanResult.matchedPaths : [];
    if (matchedPaths.length === 0) {
      return {
        matched: 0,
        ingested: 0,
        message: "No matching files found"
      };
    }

    const ingested = await ingestDesktopPathsIntoVault(matchedPaths);

    return {
      matched: matchedPaths.length,
      ingested: ingested.length,
      message: `Ingested ${ingested.length} files into Memory Vault`
    };
  }
};

const elements = {
  messages: document.getElementById("messages"),
  composer: document.getElementById("composer"),
  prompt: document.getElementById("prompt-input"),
  sendBtn: document.getElementById("send-btn"),
  clearBtn: document.getElementById("clear-btn"),
  status: document.getElementById("status-pill"),
  modelSelect: document.getElementById("chat-model"),
  refreshModelsBtn: document.getElementById("refresh-models-btn"),
  freeTestModeToggle: document.getElementById("free-test-mode"),
  localToolsToggle: document.getElementById("local-tools"),
  webSearchToggle: document.getElementById("web-search"),
  ttsModelSelect: document.getElementById("tts-model"),
  autoSpeakToggle: document.getElementById("auto-speak"),
  toolTrace: document.getElementById("tool-trace"),
  platformChip: document.getElementById("platform-chip"),
  adaptivePersonalityToggle: document.getElementById("adaptive-personality"),
  personalityTraits: document.getElementById("personality-traits"),
  resetPersonalityBtn: document.getElementById("reset-personality-btn"),
  useVaultContextToggle: document.getElementById("use-vault-context"),
  vaultSearch: document.getElementById("vault-search"),
  vaultResults: document.getElementById("vault-results"),
  chooseFilesBtn: document.getElementById("choose-files-btn"),
  vaultFileInput: document.getElementById("vault-file-input"),
  vaultSelected: document.getElementById("vault-selected"),
  ingestVaultBtn: document.getElementById("ingest-vault-btn"),
  localModeToggle: document.getElementById("local-mode"),
  localEndpointInput: document.getElementById("local-endpoint-url"),
  testLocalEndpointBtn: document.getElementById("test-local-endpoint-btn"),
  preferLocalPrivateToggle: document.getElementById("prefer-local-private"),
  allowCloudPrivateToggle: document.getElementById("allow-cloud-private"),
  legacyModeToggle: document.getElementById("legacy-mode"),
  exportProfileBtn: document.getElementById("export-profile-btn"),
  resetProfileBtn: document.getElementById("reset-profile-btn")
};

const state = {
  history: [],
  memorySummary: "",
  stylePrefs: { ...STYLE_DEFAULTS },
  lastIntent: null,
  busy: false,
  activeRequestToken: 0,
  isAndroid: false,
  androidBridge: null,
  localOpsApprovedForRequestToken: null,
  speechFallbackNotified: false,
  traceLines: [],
  apiKey: "",
  personalityEnabled: true,
  personalityVector: null,
  profile: null,
  localEndpointReachable: false,
  localEndpointLastCheck: 0,
  vaultIndex: [],
  pendingDesktopPaths: [],
  pendingAndroidFiles: [],
  useVaultContext: true,
  lastRetrievedVaultIds: []
};

void initialize();

async function initialize() {
  if (!elements.messages || !elements.composer || !elements.prompt || !elements.sendBtn) {
    throw new Error("AIFRED renderer failed to initialize required DOM elements.");
  }

  detectPlatform();
  await resolveApiKey();
  await loadPersistentState();
  configureStaticControls();
  attachEventHandlers();
  refreshModelList();
  renderPersonalityTraits();
  renderRestoredConversation();
  renderVaultResults(searchVaultItems(""));

  setStatus("Ready.", "ok");

  if (!state.apiKey && !elements.legacyModeToggle.checked) {
    appendMessage("system", "OpenAI API key not configured.");
  }
}

function detectPlatform() {
  const maybeBridge = window.AIFRED_ANDROID && typeof window.AIFRED_ANDROID === "object" ? window.AIFRED_ANDROID : null;
  const userAgent = typeof navigator?.userAgent === "string" ? navigator.userAgent : "";

  state.androidBridge = maybeBridge;
  state.isAndroid = Boolean(maybeBridge) || /Android/i.test(userAgent);

  if (state.isAndroid) {
    elements.platformChip.textContent = "platform: android";

    elements.localToolsToggle.checked = false;
    elements.localToolsToggle.disabled = true;
    elements.localToolsToggle.title = "Desktop-only local operations are disabled on Android";

    const row = elements.localToolsToggle.closest(".toggle-row");
    if (row) {
      row.title = "Desktop-only local operations are disabled on Android";
      row.style.opacity = "0.7";
    }

    appendTrace("[platform] Android detected; desktop local operations disabled");
    return;
  }

  const platform = window.desktopApp?.platform;
  elements.platformChip.textContent = platform ? `platform: ${platform}` : "platform: web";
  appendTrace(`[platform] ${elements.platformChip.textContent}`);
}

async function resolveApiKey() {
  if (window.desktopApp && typeof window.desktopApp.getOpenAIKey === "function") {
    try {
      const key = await window.desktopApp.getOpenAIKey();
      if (typeof key === "string" && key.trim()) {
        state.apiKey = key.trim();
      }
    } catch (error) {
      appendTrace(`[config] failed to read desktop OPENAI_API_KEY: ${formatError(error)}`);
    }
  }

  if (!state.apiKey && typeof window.OPENAI_API_KEY === "string" && window.OPENAI_API_KEY.trim()) {
    state.apiKey = window.OPENAI_API_KEY.trim();
  }

  appendTrace(state.apiKey ? "[config] OpenAI API key detected" : "[config] OpenAI API key missing");
}

async function loadPersistentState() {
  const parsedHistory = safeParseJSON(readStorageString(STORAGE_KEYS.history, "[]"), []);
  state.history = sanitizeHistory(parsedHistory).slice(-MAX_HISTORY_MESSAGES);

  state.memorySummary = readStorageString(STORAGE_KEYS.memorySummary, "").trim();
  if (state.memorySummary.length > MAX_MEMORY_SUMMARY_CHARS) {
    state.memorySummary = state.memorySummary.slice(-MAX_MEMORY_SUMMARY_CHARS);
  }

  const parsedStyle = safeParseJSON(readStorageString(STORAGE_KEYS.stylePrefs, "{}"), {});
  state.stylePrefs = {
    verbosity: normalizeVerbosity(parsedStyle.verbosity),
    tone: normalizeTone(parsedStyle.tone),
    baseline: STYLE_DEFAULTS.baseline
  };

  const parsedIntent = safeParseJSON(readStorageString(STORAGE_KEYS.lastIntent, "null"), null);
  if (parsedIntent && typeof parsedIntent.name === "string") {
    state.lastIntent = {
      name: parsedIntent.name,
      confidence: toConfidence(parsedIntent.confidence),
      weights: parsedIntent.weights && typeof parsedIntent.weights === "object" ? parsedIntent.weights : {}
    };
  }

  state.personalityEnabled = readStorageBool(STORAGE_KEYS.personalityEnabled, true);
  const storedPersonality = safeParseJSON(readStorageString(STORAGE_KEYS.personalityVector, "{}"), {});
  state.personalityVector = window.AIFREDPersonality
    ? window.AIFREDPersonality.normalizeVector(storedPersonality)
    : {
        verbosity: 0.5,
        technicality: 0.6,
        warmth: 0.45,
        directness: 0.7,
        humor: 0.15,
        risk_aversion: 0.65
      };

  state.useVaultContext = readStorageBool(STORAGE_KEYS.useVaultContext, true);

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.loadMemoryState === "function") {
    try {
      const desktopMemory = await window.desktopApp.loadMemoryState();
      if (desktopMemory && typeof desktopMemory === "object") {
        if (Array.isArray(desktopMemory.history)) {
          state.history = sanitizeHistory(desktopMemory.history).slice(-MAX_HISTORY_MESSAGES);
        }

        if (typeof desktopMemory.memorySummary === "string") {
          state.memorySummary = desktopMemory.memorySummary.slice(-MAX_MEMORY_SUMMARY_CHARS);
        }

        if (desktopMemory.stylePrefs && typeof desktopMemory.stylePrefs === "object") {
          state.stylePrefs.verbosity = normalizeVerbosity(desktopMemory.stylePrefs.verbosity);
          state.stylePrefs.tone = normalizeTone(desktopMemory.stylePrefs.tone);
        }

        if (desktopMemory.personality && window.AIFREDPersonality) {
          state.personalityVector = window.AIFREDPersonality.normalizeVector(desktopMemory.personality);
        }
      }
    } catch (error) {
      appendTrace(`[memory] desktop memory load failed: ${formatError(error)}`);
    }
  }

  await loadProfile();
  await loadVaultIndex();

  if (state.history.length > MAX_HISTORY_MESSAGES) {
    state.history = state.history.slice(-MAX_HISTORY_MESSAGES);
  }
}

async function loadProfile() {
  let parsedProfile = safeParseJSON(readStorageString(STORAGE_KEYS.profile, "null"), null);

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.loadProfile === "function") {
    try {
      const desktopProfile = await window.desktopApp.loadProfile();
      if (desktopProfile && typeof desktopProfile === "object") {
        parsedProfile = desktopProfile;
      }
    } catch (error) {
      appendTrace(`[profile] desktop profile load failed: ${formatError(error)}`);
    }
  }

  if (window.AIFREDProfileStore) {
    state.profile = window.AIFREDProfileStore.sanitizeProfile(parsedProfile);
  } else {
    state.profile = parsedProfile || {};
  }

  await persistProfile();
}

async function persistProfile() {
  writeStorageString(STORAGE_KEYS.profile, safeJSONStringify(state.profile));

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.saveProfile === "function") {
    try {
      await window.desktopApp.saveProfile(state.profile);
    } catch (error) {
      appendTrace(`[profile] desktop profile save failed: ${formatError(error)}`);
    }
  }
}

async function loadVaultIndex() {
  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.loadVaultIndex === "function") {
    try {
      const desktopIndex = await window.desktopApp.loadVaultIndex();
      state.vaultIndex = sanitizeVaultIndex(desktopIndex);
      writeStorageString(STORAGE_KEYS.vaultIndex, safeJSONStringify(state.vaultIndex));
      appendTrace(`[vault] loaded ${state.vaultIndex.length} desktop item(s)`);
      return;
    } catch (error) {
      appendTrace(`[vault] desktop vault load failed: ${formatError(error)}`);
    }
  }

  const local = safeParseJSON(readStorageString(STORAGE_KEYS.vaultIndex, "[]"), []);
  state.vaultIndex = sanitizeVaultIndex(local);
  appendTrace(`[vault] loaded ${state.vaultIndex.length} local item(s)`);
}

async function saveVaultIndex() {
  const clean = sanitizeVaultIndex(state.vaultIndex);
  state.vaultIndex = clean;
  writeStorageString(STORAGE_KEYS.vaultIndex, safeJSONStringify(clean));

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.saveVaultIndex === "function") {
    try {
      await window.desktopApp.saveVaultIndex({ index: clean });
    } catch (error) {
      appendTrace(`[vault] desktop vault save failed: ${formatError(error)}`);
    }
  }
}

function configureStaticControls() {
  if (elements.freeTestModeToggle) {
    elements.freeTestModeToggle.checked = false;
    elements.freeTestModeToggle.disabled = true;
  }

  if (elements.ttsModelSelect && elements.ttsModelSelect.options.length > 0) {
    const stored = readStorageString(STORAGE_KEYS.ttsModel, "");
    const target = optionExists(elements.ttsModelSelect, stored) ? stored : DEFAULT_TTS_MODEL;
    if (optionExists(elements.ttsModelSelect, target)) {
      elements.ttsModelSelect.value = target;
    }
  }

  if (elements.autoSpeakToggle) {
    elements.autoSpeakToggle.checked = readStorageBool(STORAGE_KEYS.autoSpeak, false);
  }

  if (elements.webSearchToggle) {
    elements.webSearchToggle.checked = readStorageBool(STORAGE_KEYS.webSearch, true);
  }

  if (elements.localToolsToggle && !state.isAndroid) {
    elements.localToolsToggle.checked = readStorageBool(STORAGE_KEYS.localTools, true);
  }

  if (elements.adaptivePersonalityToggle) {
    elements.adaptivePersonalityToggle.checked = state.personalityEnabled;
  }

  if (elements.useVaultContextToggle) {
    elements.useVaultContextToggle.checked = state.useVaultContext;
  }

  if (elements.localModeToggle) {
    elements.localModeToggle.checked = readStorageBool(STORAGE_KEYS.routeLocalMode, false);
  }

  if (elements.localEndpointInput) {
    const storedEndpoint = readStorageString(
      STORAGE_KEYS.routeLocalEndpoint,
      "http://127.0.0.1:11434/v1/chat/completions"
    );
    elements.localEndpointInput.value = storedEndpoint;
  }

  if (elements.preferLocalPrivateToggle) {
    elements.preferLocalPrivateToggle.checked = readStorageBool(STORAGE_KEYS.routePreferLocalPrivate, true);
  }

  if (elements.allowCloudPrivateToggle) {
    elements.allowCloudPrivateToggle.checked = readStorageBool(STORAGE_KEYS.routeAllowCloudPrivate, false);
  }

  if (elements.legacyModeToggle) {
    elements.legacyModeToggle.checked = readStorageBool(STORAGE_KEYS.routeLegacyMode, false);
  }

  if (elements.vaultSelected) {
    elements.vaultSelected.textContent = "No file selected";
  }
}

function attachEventHandlers() {
  elements.composer.addEventListener("submit", handleSubmit);
  elements.clearBtn.addEventListener("click", clearConversation);

  if (elements.refreshModelsBtn) {
    elements.refreshModelsBtn.addEventListener("click", refreshModelList);
  }

  elements.prompt.addEventListener("keydown", (event) => {
    if (event.key === "Enter" && !event.shiftKey) {
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
  });

  elements.modelSelect.addEventListener("change", () => {
    const selected = normalizeModelId(elements.modelSelect.value) || FALLBACK_CHAT_MODEL;
    writeStorageString(STORAGE_KEYS.selectedModel, selected);
    appendTrace(`[models] selected ${selected}`);
  });

  elements.ttsModelSelect.addEventListener("change", () => {
    writeStorageString(STORAGE_KEYS.ttsModel, elements.ttsModelSelect.value);
  });

  elements.autoSpeakToggle.addEventListener("change", () => {
    writeStorageBool(STORAGE_KEYS.autoSpeak, elements.autoSpeakToggle.checked);
  });

  elements.webSearchToggle.addEventListener("change", () => {
    writeStorageBool(STORAGE_KEYS.webSearch, elements.webSearchToggle.checked);
  });

  elements.localToolsToggle.addEventListener("change", () => {
    if (state.isAndroid) {
      elements.localToolsToggle.checked = false;
      return;
    }
    writeStorageBool(STORAGE_KEYS.localTools, elements.localToolsToggle.checked);
  });

  if (elements.adaptivePersonalityToggle) {
    elements.adaptivePersonalityToggle.addEventListener("change", () => {
      state.personalityEnabled = elements.adaptivePersonalityToggle.checked;
      writeStorageBool(STORAGE_KEYS.personalityEnabled, state.personalityEnabled);
      renderPersonalityTraits();
    });
  }

  if (elements.resetPersonalityBtn) {
    elements.resetPersonalityBtn.addEventListener("click", () => {
      if (!window.AIFREDPersonality) {
        return;
      }

      state.personalityVector = window.AIFREDPersonality.resetVector();
      writeStorageString(STORAGE_KEYS.personalityVector, safeJSONStringify(state.personalityVector));
      renderPersonalityTraits();
      appendTrace("[personality] reset to defaults");
    });
  }

  if (elements.useVaultContextToggle) {
    elements.useVaultContextToggle.addEventListener("change", () => {
      state.useVaultContext = elements.useVaultContextToggle.checked;
      writeStorageBool(STORAGE_KEYS.useVaultContext, state.useVaultContext);
    });
  }

  if (elements.vaultSearch) {
    elements.vaultSearch.addEventListener("input", () => {
      renderVaultResults(searchVaultItems(elements.vaultSearch.value));
    });
  }

  if (elements.chooseFilesBtn) {
    elements.chooseFilesBtn.addEventListener("click", () => {
      void chooseVaultFiles();
    });
  }

  if (elements.vaultFileInput) {
    elements.vaultFileInput.addEventListener("change", () => {
      const files = Array.from(elements.vaultFileInput.files || []);
      state.pendingAndroidFiles = files;
      state.pendingDesktopPaths = [];
      elements.vaultSelected.textContent =
        files.length > 0 ? `${files.length} file(s) selected` : "No file selected";
    });
  }

  if (elements.ingestVaultBtn) {
    elements.ingestVaultBtn.addEventListener("click", () => {
      void ingestSelectedFilesToVault();
    });
  }

  if (elements.localModeToggle) {
    elements.localModeToggle.addEventListener("change", () => {
      writeStorageBool(STORAGE_KEYS.routeLocalMode, elements.localModeToggle.checked);
    });
  }

  if (elements.localEndpointInput) {
    elements.localEndpointInput.addEventListener("change", () => {
      writeStorageString(STORAGE_KEYS.routeLocalEndpoint, elements.localEndpointInput.value.trim());
      state.localEndpointReachable = false;
      state.localEndpointLastCheck = 0;
    });
  }

  if (elements.preferLocalPrivateToggle) {
    elements.preferLocalPrivateToggle.addEventListener("change", () => {
      writeStorageBool(STORAGE_KEYS.routePreferLocalPrivate, elements.preferLocalPrivateToggle.checked);
    });
  }

  if (elements.allowCloudPrivateToggle) {
    elements.allowCloudPrivateToggle.addEventListener("change", () => {
      writeStorageBool(STORAGE_KEYS.routeAllowCloudPrivate, elements.allowCloudPrivateToggle.checked);
    });
  }

  if (elements.legacyModeToggle) {
    elements.legacyModeToggle.addEventListener("change", () => {
      writeStorageBool(STORAGE_KEYS.routeLegacyMode, elements.legacyModeToggle.checked);
    });
  }

  if (elements.testLocalEndpointBtn) {
    elements.testLocalEndpointBtn.addEventListener("click", () => {
      void testLocalEndpoint();
    });
  }

  if (elements.exportProfileBtn) {
    elements.exportProfileBtn.addEventListener("click", () => {
      void exportProfile();
    });
  }

  if (elements.resetProfileBtn) {
    elements.resetProfileBtn.addEventListener("click", () => {
      void resetProfile();
    });
  }

  window.addEventListener("beforeunload", () => {
    void persistConversationState();
  });

  window.aifredSpeech = {
    start: startSpeechRecognition,
    stop: stopSpeechRecognition
  };
}

function refreshModelList() {
  const preferred = normalizeModelId(elements.modelSelect.value) || getPreferredModel();
  rebuildModelSelect(SUPPORTED_CHAT_MODELS, preferred);
  appendTrace("[models] loaded fixed OpenAI model list");
  setStatus("Model list updated.", "ok");
}

function renderPersonalityTraits() {
  if (!elements.personalityTraits) {
    return;
  }

  if (!state.personalityEnabled) {
    elements.personalityTraits.textContent = "Adaptive personality: off";
    return;
  }

  const vector = state.personalityVector || {};
  const format = (value) => toFixedNumber(value, 2);
  elements.personalityTraits.textContent = `v:${format(vector.verbosity)} t:${format(vector.technicality)} w:${format(
    vector.warmth
  )} d:${format(vector.directness)} h:${format(vector.humor)} r:${format(vector.risk_aversion)}`;
}

function renderRestoredConversation() {
  elements.messages.innerHTML = "";

  const renderable = state.history.filter((entry) => {
    if (entry.role !== "user" && entry.role !== "assistant" && entry.role !== "system") {
      return false;
    }

    return typeof entry.content === "string" && entry.content.trim().length > 0;
  });

  const visible = renderable.slice(-MAX_VISIBLE_MESSAGES);
  for (const entry of visible) {
    appendMessage(entry.role, entry.content);
  }

  if (visible.length === 0) {
    appendMessage(
      "system",
      "AIFRED is online. OpenAI chat, function tools, and Memory Vault context are active."
    );
  }

  elements.messages.scrollTop = elements.messages.scrollHeight;
}

async function handleSubmit(event) {
  event.preventDefault();

  if (state.busy) {
    cancelActiveRequest("Stopped current request.");
    return;
  }

  function saveMemory() {
    const payload = {
      messages: memory.messages.slice(-CFG.MAX_TURNS * 2),
    };
    localStorage.setItem(CFG.MEMORY_STORAGE_KEY, JSON.stringify(payload));
  }

  elements.prompt.value = "";
  appendMessage("user", userText);

  applyRetrievalFeedbackFromUserMessage(userText);

  const intentSignal = detectIntentSignal(userText);
  const styleChanged = updateStylePreferences(userText);

  if (styleChanged) {
    appendTrace(`[style] verbosity=${state.stylePrefs.verbosity} tone=${state.stylePrefs.tone}`);
  }

  if (state.personalityEnabled && window.AIFREDPersonality) {
    state.personalityVector = window.AIFREDPersonality.updateVector(state.personalityVector, {
      userText,
      followupCount: countRecentFollowups()
    });

    writeStorageString(STORAGE_KEYS.personalityVector, safeJSONStringify(state.personalityVector));
    renderPersonalityTraits();
  }

  state.lastIntent = {
    name: intentSignal.primary.name,
    confidence: toConfidence(intentSignal.primary.score),
    weights: intentSignal.weights
  };

  appendTrace(
    `[intent] primary=${intentSignal.primary.name} weights=${safeJSONStringify(intentSignal.weights)}`
  );

  addHistoryEntry({
    role: "user",
    content: userText,
    ts: Date.now(),
    intent: intentSignal.primary.name
  });

  if (window.AIFREDProfileStore) {
    state.profile = window.AIFREDProfileStore.updateFromInteraction(state.profile, {
      intentWeights: intentSignal.weights,
      personality: state.personalityVector,
      userText
    });
    await persistProfile();
  }

  await runAssistantLoop(intentSignal, userText);
}

function countRecentFollowups() {
  const recentUsers = state.history.slice(-8).filter((entry) => entry.role === "user");
  return recentUsers.length;
}

function detectIntentSignal(userText) {
  if (window.AIFREDIntent && typeof window.AIFREDIntent.detect === "function") {
    return window.AIFREDIntent.detect(userText);
  }

  return {
    primary: {
      name: "general",
      score: 1
    },
    secondary: null,
    weights: {
      general: 1
    },
    isPrivate: false,
    wantsWebSearch: false,
    wantsTools: true
  };
}

async function runAssistantLoop(intentSignal, userText) {
  const requestToken = beginRequest();
  let activeModel = normalizeModelId(elements.modelSelect.value) || FALLBACK_CHAT_MODEL;

  try {
    for (let round = 0; round < MAX_TOOL_ROUNDS; round += 1) {
      ensureRequestActive(requestToken);

      const toolset = buildToolset(intentSignal);
      const relevantContext = state.useVaultContext ? getRelevantVaultContext(userText, intentSignal) : [];

      const routeDecision = await chooseRoute(intentSignal, relevantContext.length > 0);
      appendTrace(`[route] chosen=${routeDecision.chosen} reason=${routeDecision.reason}`);

      if (routeDecision.chosen === "blocked") {
        throw new Error(`No valid route available: ${routeDecision.reason}`);
      }
    };
  }

      const chatMessages = buildChatRequestMessages(intentSignal, relevantContext);

      appendTrace(
        `[chat] request=${requestToken} round=${round + 1} model=${activeModel} tools=${toolset.length} route=${routeDecision.chosen}`
      );

      const result = await requestAssistantWithRouting({
        preferredModel: activeModel,
        messages: chatMessages,
        tools: toolset,
        routeDecision,
        intentSignal
      });

      ensureRequestActive(requestToken);
      activeModel = result.modelUsed;

      const parsed = result.parsed;
      if (parsed.toolCalls.length > 0 && canUseLocalTools()) {
        addHistoryEntry({
          role: "assistant",
          content: parsed.text,
          tool_calls: parsed.toolCalls,
          ts: Date.now()
        });

        for (const toolCall of parsed.toolCalls) {
          ensureRequestActive(requestToken);
          const toolOutput = await executeToolCall(toolCall, requestToken);
          addHistoryEntry({
            role: "tool",
            tool_call_id: toolCall.id,
            content: toolOutput,
            ts: Date.now()
          });
        }

        continue;
      }

      const finalText = parsed.text.trim();
      if (!finalText) {
        throw new Error("Received an empty assistant response.");
      }

      addHistoryEntry({
        role: "assistant",
        content: finalText,
        ts: Date.now()
      });

      const bubble = appendMessage("assistant", finalText);
      if (elements.autoSpeakToggle.checked) {
        await speakText(finalText, bubble, true);
      }

      setStatus("Ready.", "ok");
      return;
    }

    throw new Error(`Stopped after ${MAX_TOOL_ROUNDS} tool-call rounds.`);
  } catch (error) {
    if (isRequestCancelledError(error)) {
      return;
    }

    const userMessage = formatUserFacingError(error);
    appendMessage("system", userMessage);
    setStatus("Request failed.", "error");
    appendTrace(`[error] ${formatErrorDetails(error)}`);
  } finally {
    endRequest(requestToken);
  }
}

async function chooseRoute(intentSignal, hasVaultContext) {
  let localReachable = state.localEndpointReachable;

  if (elements.localModeToggle.checked) {
    const stale = Date.now() - state.localEndpointLastCheck > 60_000;
    if (stale) {
      localReachable = await probeLocalEndpoint();
    }
  }

  if (!window.AIFREDRouter || typeof window.AIFREDRouter.chooseRoute !== "function") {
    return {
      chosen: "cloud",
      reason: "router module unavailable"
    };
  }

  const decision = window.AIFREDRouter.chooseRoute({
    localMode: elements.localModeToggle.checked,
    localReachable,
    legacyMode: elements.legacyModeToggle.checked,
    preferLocalPrivate: elements.preferLocalPrivateToggle.checked,
    allowCloudPrivate: elements.allowCloudPrivateToggle.checked,
    hasOpenAIKey: Boolean(state.apiKey),
    isPrivate: Boolean(intentSignal.isPrivate || hasVaultContext),
    wantsWebSearch: Boolean(intentSignal.wantsWebSearch || elements.webSearchToggle.checked)
  });

  return decision;
}

async function testLocalEndpoint() {
  setStatus("Testing local endpoint...", "neutral");
  const reachable = await probeLocalEndpoint(true);
  if (reachable) {
    setStatus("Local endpoint reachable.", "ok");
  } else {
    setStatus("Local endpoint unavailable.", "warn");
  }
}

async function probeLocalEndpoint(force) {
  const stale = Date.now() - state.localEndpointLastCheck > 60_000;
  if (!force && !stale) {
    return state.localEndpointReachable;
  }

  state.localEndpointLastCheck = Date.now();

  const localUrl = getLocalEndpointUrl();
  if (!localUrl) {
    state.localEndpointReachable = false;
    appendTrace("[route] local endpoint URL missing");
    return false;
  }

  const payload = {
    model: normalizeModelId(elements.modelSelect.value) || DEFAULT_CHAT_MODEL,
    messages: [{ role: "user", content: "ping" }],
    temperature: 0,
    max_tokens: 8,
    stream: false
  };

  try {
    const response = await apiJsonRequest({
      url: localUrl,
      method: "POST",
      payload,
      timeoutMs: LOCAL_TEST_TIMEOUT_MS
    });

    state.localEndpointReachable = response.status > 0 && response.status < 500;
    appendTrace(`[route] local probe status=${response.status}`);
    return state.localEndpointReachable;
  } catch (error) {
    state.localEndpointReachable = false;
    appendTrace(`[route] local probe failed: ${formatError(error)}`);
    return false;
  }
}

function getLocalEndpointUrl() {
  const value = (elements.localEndpointInput?.value || "").trim();
  return value || readStorageString(STORAGE_KEYS.routeLocalEndpoint, "");
}

async function requestAssistantWithRouting(input) {
  const preferredModel = normalizeModelId(input.preferredModel) || FALLBACK_CHAT_MODEL;
  const route = input.routeDecision?.chosen || "cloud";

  const attempts = [];
  if (route === "local") {
    attempts.push("local");
    if (state.apiKey) {
      attempts.push("cloud");
    }
    if (elements.legacyModeToggle.checked) {
      attempts.push("legacy");
    }
  } else if (route === "legacy") {
    attempts.push("legacy");
    if (state.apiKey) {
      attempts.push("cloud");
    }
  } else {
    attempts.push("cloud");
    if (elements.localModeToggle.checked) {
      attempts.push("local");
    }
    if (elements.legacyModeToggle.checked) {
      attempts.push("legacy");
    }
  }

  let lastError = null;

  for (const attempt of attempts) {
    try {
      const payload = buildChatPayload({
        model: preferredModel,
        messages: input.messages,
        tools: input.tools
      });

      const response =
        attempt === "local"
          ? await requestLocalCompletion(payload)
          : attempt === "legacy"
            ? await requestLegacyCompletion(payload)
            : await requestOpenAICompletion(payload);

      if (response.status !== 200) {
        throw buildApiError(response, {
          model: preferredModel,
          route: attempt,
          endpoint: response.endpoint
        });
      }

      const parsed = parseAssistantPayload(response.json, response.text);
      if (!parsed.text && parsed.toolCalls.length === 0) {
        const emptyError = new Error(`Empty response payload from route ${attempt}`);
        emptyError.name = "EmptyResponseError";
        throw emptyError;
      }

      appendTrace(`[chat] route=${attempt} model=${preferredModel} succeeded`);
      return {
        parsed,
        modelUsed: preferredModel
      };
    } catch (error) {
      lastError = error;
      appendTrace(`[chat] route=${attempt} failed: ${formatError(error)}`);
    }
  }

  throw lastError || new Error("No response from available routes.");
}

function buildChatPayload(input) {
  const payload = {
    model: input.model,
    messages: input.messages,
    temperature: CHAT_TEMPERATURE,
    max_tokens: CHAT_MAX_TOKENS,
    stream: false
  };

  if (Array.isArray(input.tools) && input.tools.length > 0) {
    payload.tools = input.tools;
    payload.tool_choice = "auto";
  }

  return payload;
}

async function requestOpenAICompletion(payload) {
  if (!state.apiKey) {
    const missing = new Error("OpenAI API key not configured.");
    missing.name = "MissingApiKeyError";
    missing.userMessage = "OpenAI API key not configured.";
    throw missing;
  }

  const response = await apiJsonRequest({
    url: OPENAI_CHAT_ENDPOINT,
    method: "POST",
    payload,
    headers: buildOpenAIHeaders(),
    timeoutMs: CHAT_TIMEOUT_MS
  });

  response.endpoint = OPENAI_CHAT_ENDPOINT;
  return response;
}

async function requestLocalCompletion(payload) {
  const url = getLocalEndpointUrl();
  if (!url) {
    throw new Error("Local endpoint URL is not configured.");
  }

  const response = await apiJsonRequest({
    url,
    method: "POST",
    payload,
    headers: {
      "Content-Type": "application/json"
    },
    timeoutMs: CHAT_TIMEOUT_MS
  });

  response.endpoint = url;
  return response;
}

async function requestLegacyCompletion(payload) {
  const response = await apiJsonRequest({
    url: LEGACY_ANON_CHAT_ENDPOINT,
    method: "POST",
    payload,
    headers: {
      "Content-Type": "application/json"
    },
    timeoutMs: CHAT_TIMEOUT_MS
  });

  response.endpoint = LEGACY_ANON_CHAT_ENDPOINT;
  return response;
}

function buildToolset(intentSignal) {
  const tools = [];

  if (canUseLocalTools() && intentSignal.wantsTools !== false) {
    tools.push(...LOCAL_TOOLS);
  }

  if (elements.webSearchToggle.checked) {
    tools.push({ type: "web_search" });
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

function canUseLocalTools() {
  if (state.isAndroid) {
    return false;
  }

  return elements.localToolsToggle.checked;
}

function buildChatRequestMessages(intentSignal, relevantVaultContext) {
  const messages = [];

  messages.push({
    role: "system",
    content: buildDynamicSystemPrompt(intentSignal)
  });

  if (state.personalityEnabled && window.AIFREDPersonality) {
    messages.push({
      role: "system",
      content: window.AIFREDPersonality.buildPrompt(state.personalityVector)
    });
  }

  if (state.memorySummary) {
    messages.push({
      role: "system",
      content: `Conversation memory summary (persistent):\n${state.memorySummary}`
    });
  }

  if (Array.isArray(relevantVaultContext) && relevantVaultContext.length > 0) {
    const contextLines = relevantVaultContext.map((item) => {
      const tagText = Array.isArray(item.tags) && item.tags.length > 0 ? ` tags=${item.tags.join(",")}` : "";
      return `- ${item.filename}: ${item.summaryText || "(no summary)"}${tagText}`;
    });

    messages.push({
      role: "system",
      content: `Relevant user memory:\n${contextLines.join("\n")}`
    });
  }

  const contextEntries = state.history.slice(-MAX_CONTEXT_MESSAGES);
  for (const entry of contextEntries) {
    if (!entry || typeof entry.role !== "string") {
      continue;
    }

    if (entry.role === "tool") {
      messages.push({
        role: "tool",
        tool_call_id: typeof entry.tool_call_id === "string" ? entry.tool_call_id : "tool",
        content: String(entry.content || "")
      });
      continue;
    }

    if (entry.role === "assistant") {
      const assistantMessage = {
        role: "assistant",
        content: String(entry.content || "")
      };
      if (Array.isArray(entry.tool_calls) && entry.tool_calls.length > 0) {
        assistantMessage.tool_calls = entry.tool_calls.map(normalizeToolCall).filter((item) => item.function.name);
      }
      messages.push(assistantMessage);
      continue;
    }

    if (entry.role === "user" || entry.role === "system") {
      messages.push({
        role: entry.role,
        content: String(entry.content || "")
      });
    }
  }

  return messages;
}

function buildDynamicSystemPrompt(intentSignal) {
  const styleVerbosity = state.stylePrefs.verbosity;
  const styleTone = state.stylePrefs.tone;

  const verbosityDirective =
    styleVerbosity === "short"
      ? "Keep responses compact. Prefer <= 120 words unless accuracy requires more."
      : styleVerbosity === "detailed"
        ? "Use richer detail, include rationale and implementation caveats."
        : "Keep responses balanced: concise but complete.";

  const toneDirective =
    styleTone === "friendly"
      ? "Use a friendly, grounded tone without losing precision."
      : styleTone === "blunt"
        ? "Use direct, blunt phrasing with minimal fluff."
        : "Use professional, objective phrasing.";

  const weights = intentSignal.weights || {};
  const technicalWeight = Number(weights["coding/dev"] || 0) + Number(weights.troubleshooting || 0);
  const musicWeight = Number(weights["music/audio"] || 0);
  const explainWeight = Number(weights.planning || 0);

  const templateDirective =
    technicalWeight >= Math.max(musicWeight, explainWeight)
      ? TEMPLATE_DIRECTIVES.technical
      : musicWeight >= Math.max(technicalWeight, explainWeight)
        ? TEMPLATE_DIRECTIVES.music
        : explainWeight >= 0.3
          ? TEMPLATE_DIRECTIVES.explain
          : TEMPLATE_DIRECTIVES.general;

  const followUpHint = state.lastIntent
    ? `Last detected intent: ${state.lastIntent.name} (${toFixedNumber(state.lastIntent.confidence, 2)}).`
    : "";

  const platformHint = state.isAndroid
    ? "Platform: Android. Desktop-only local operations are unavailable."
    : "Platform: Desktop/Web. Local tools require explicit confirmation before execution.";

  return [
    "You are AIFRED, a real-time chatbot assistant.",
    state.stylePrefs.baseline,
    verbosityDirective,
    toneDirective,
    templateDirective,
    `Current primary intent: ${intentSignal.primary.name}.`,
    followUpHint,
    platformHint,
    "Use available tools when they improve factual accuracy. If tools are unavailable, continue with a direct answer.",
    "If a tool call returns an error, recover gracefully and still provide a useful next action."
  ]
    .filter(Boolean)
    .join(" ");
}

async function executeToolCall(toolCall, requestToken) {
  const toolName = toolCall?.function?.name || "";
  const args = safeParseJSON(toolCall?.function?.arguments, {});

  appendTrace(`[tool-call] ${toolName}(${safeJSONStringify(args)})`);

  if (!canUseLocalTools()) {
    const unavailable = JSON.stringify({
      error: "Local tools are unavailable on this platform."
    });
    appendTrace(`[tool-result] ${toolName} -> ${unavailable}`);
    return unavailable;
  }

  if (!confirmLocalOperationsGate(toolName, requestToken)) {
    const denied = JSON.stringify({
      error: "Local tool execution was blocked by user confirmation gate."
    });
    appendTrace(`[tool-result] ${toolName} -> ${denied}`);
    return denied;
  }

  const handler = LOCAL_TOOL_HANDLERS[toolName];
  if (!handler) {
    const unknown = JSON.stringify({ error: `Unknown tool: ${toolName}` });
    appendTrace(`[tool-result] ${toolName} -> ${unknown}`);
    return unknown;
  }

  try {
    const result = await handler(args);
    const output = typeof result === "string" ? result : safeJSONStringify(result);
    appendTrace(`[tool-result] ${toolName} -> ${output}`);
    return output;
  } catch (error) {
    const output = JSON.stringify({ error: formatError(error) });
    appendTrace(`[tool-result] ${toolName} -> ${output}`);
    return output;
  }
}

function confirmLocalOperationsGate(toolName, requestToken) {
  if (state.isAndroid) {
    return false;
  }

  if (state.localOpsApprovedForRequestToken === requestToken) {
    return true;
  }

  const approved = window.confirm(
    `Allow desktop local operation for this assistant turn?\nTool requested: ${toolName}`
  );

  if (approved) {
    state.localOpsApprovedForRequestToken = requestToken;
  }

  return approved;
}

function parseAssistantPayload(responseObject, rawTextFallback) {
  const source = responseObject || {};
  const message = source?.choices?.[0]?.message || {};

  const text = normalizeContent(message.content) || normalizeContent(rawTextFallback);
  const rawToolCalls = message.tool_calls;

  const toolCalls = Array.isArray(rawToolCalls)
    ? rawToolCalls.map(normalizeToolCall).filter((item) => item.function.name)
    : [];

  return {
    text: String(text || "").trim(),
    toolCalls
  };
}

function normalizeToolCall(toolCall) {
  const rawArgs = toolCall?.function?.arguments;
  const argsString =
    typeof rawArgs === "string" ? rawArgs : safeJSONStringify(rawArgs === undefined ? {} : rawArgs);

  return {
    id: typeof toolCall?.id === "string" ? toolCall.id : `tool_${Math.random().toString(16).slice(2)}`,
    type: "function",
    function: {
      name: typeof toolCall?.function?.name === "string" ? toolCall.function.name : "",
      arguments: argsString
    }
  };
}

function normalizeContent(content) {
  if (typeof content === "string") {
    return content.trim();
  }

  if (Array.isArray(content)) {
    return content
      .map((item) => {
        if (typeof item === "string") {
          return item;
        }

        if (item && typeof item === "object") {
          if (typeof item.text === "string") {
            return item.text;
          }

          if (typeof item.content === "string") {
            return item.content;
          }
        }

        return "";
      })
      .join("\n")
      .trim();
  }

  if (content && typeof content === "object") {
    if (typeof content.text === "string") {
      return content.text.trim();
    }
    if (typeof content.content === "string") {
      return content.content.trim();
    }
  }

  return "";
}

  async function runTool(toolCall) {
    const name = toolCall?.function?.name || "";
    const args = safeParseJSON(toolCall?.function?.arguments, {});
    appendTrace(`[tool] ${name} ${safeJSONStringify(args)}`);

  const roleLine = document.createElement("p");
  roleLine.className = "role";
  roleLine.textContent = role;

  const textLine = document.createElement("p");
  textLine.className = "text";
  textLine.textContent = text;

  wrapper.append(roleLine, textLine);

  if (role === "assistant" && text && text.trim()) {
    const actions = document.createElement("div");
    actions.className = "bubble-actions";

    const speakBtn = document.createElement("button");
    speakBtn.type = "button";
    speakBtn.textContent = "Speak";
    speakBtn.addEventListener("click", async () => {
      speakBtn.disabled = true;
      try {
        await speakText(text, wrapper, true);
      } finally {
        speakBtn.disabled = false;
      }
    });

    actions.appendChild(speakBtn);
    wrapper.appendChild(actions);
  }

  elements.messages.appendChild(wrapper);
  elements.messages.scrollTop = elements.messages.scrollHeight;
  return wrapper;
}

async function speakText(text, messageBubble, autoplay) {
  const cleanText = String(text || "").trim();
  if (!cleanText) {
    return;
  }

  if (state.androidBridge && typeof state.androidBridge.ttsSpeak === "function") {
    try {
      const maybePromise = state.androidBridge.ttsSpeak(cleanText);
      if (maybePromise && typeof maybePromise.then === "function") {
        await maybePromise;
      }
      appendTrace("[speech] used Android native ttsSpeak bridge");
      return;
    } catch (error) {
      appendTrace(`[speech] Android ttsSpeak failed: ${formatError(error)}; trying OpenAI audio fallback`);
    }
  }

  try {
    await addOpenAISpeechPlayer(messageBubble, cleanText, autoplay);
    appendTrace("[speech] played OpenAI audio");
    return;
  } catch (error) {
    appendTrace(`[speech] OpenAI audio failed: ${formatError(error)}; using browser speech synthesis fallback`);
  }

  if (typeof window !== "undefined" && "speechSynthesis" in window && typeof SpeechSynthesisUtterance === "function") {
    const utterance = new SpeechSynthesisUtterance(cleanText);
    utterance.rate = 1;
    utterance.pitch = 1;
    utterance.volume = 1;
    window.speechSynthesis.cancel();
    window.speechSynthesis.speak(utterance);

    if (!state.speechFallbackNotified) {
      state.speechFallbackNotified = true;
      appendTrace("[speech] browser speech synthesis fallback active");
    }
    return;
  }

  appendMessage("system", "Speak is unavailable: no Android bridge, OpenAI audio, or browser speech synthesis support.");
}

async function addOpenAISpeechPlayer(messageBubble, text, autoplay) {
  if (!state.apiKey && !elements.legacyModeToggle.checked) {
    throw new Error("OpenAI API key not configured.");
  }

  const payload = {
    model: elements.ttsModelSelect.value || DEFAULT_TTS_MODEL,
    input: text,
    voice: "alloy",
    format: "mp3"
  };

  const tryOpenAI = state.apiKey
    ? await apiRawRequest({
        url: OPENAI_TTS_ENDPOINT,
        method: "POST",
        body: safeJSONStringify(payload),
        headers: buildOpenAIHeaders(),
        timeoutMs: TTS_TIMEOUT_MS
      })
    : null;

  if (tryOpenAI && tryOpenAI.status === 200) {
    const contentType = String(tryOpenAI.contentType || "").toLowerCase();

    if (contentType.startsWith("audio/") || tryOpenAI.bytes.length > 0) {
      const audioElement = attachAudioToBubble(messageBubble, tryOpenAI.bytes, contentType || "audio/mpeg", autoplay);
      if (!audioElement) {
        throw new Error("Failed to attach OpenAI audio element");
      }
      return;
    }
  }

  if (tryOpenAI && tryOpenAI.status !== 200) {
    throw buildApiError(
      {
        ...tryOpenAI,
        json: safeParseJSON(tryOpenAI.text, null)
      },
      {
        route: "cloud",
        model: elements.ttsModelSelect.value || DEFAULT_TTS_MODEL,
        endpoint: OPENAI_TTS_ENDPOINT
      }
    );
  }

  if (elements.legacyModeToggle.checked) {
    const legacyResponse = await apiRawRequest({
      url: LEGACY_ANON_TTS_ENDPOINT,
      method: "POST",
      body: safeJSONStringify(payload),
      headers: {
        "Content-Type": "application/json"
      },
      timeoutMs: TTS_TIMEOUT_MS
    });

    if (legacyResponse.status === 200 && legacyResponse.bytes.length > 0) {
      const audioElement = attachAudioToBubble(
        messageBubble,
        legacyResponse.bytes,
        legacyResponse.contentType || "audio/mpeg",
        autoplay
      );
      if (!audioElement) {
        throw new Error("Failed to attach legacy audio element");
      }
      return;
    }
  }

  throw new Error("Audio speech endpoint did not return playable audio");
}

function attachAudioToBubble(messageBubble, bytes, contentType, autoplay) {
  if (!messageBubble || !(bytes instanceof Uint8Array) || bytes.length === 0) {
    return null;
  }

  const blob = new Blob([bytes], { type: contentType || "audio/mpeg" });
  const url = URL.createObjectURL(blob);

  const existing = messageBubble.querySelector("audio.tts-player");
  if (existing) {
    if (existing.src) {
      URL.revokeObjectURL(existing.src);
    }
    existing.remove();
  }

  const audio = document.createElement("audio");
  audio.className = "tts-player";
  audio.controls = true;
  audio.src = url;

  messageBubble.appendChild(audio);

  if (autoplay) {
    audio.play().catch(() => {
      // Browser autoplay policies can block playback; controls remain available.
    });
  }

  return audio;
}

function startSpeechRecognition() {
  if (state.androidBridge && typeof state.androidBridge.sttStart === "function") {
    try {
      state.androidBridge.sttStart();
      appendTrace("[speech] Android sttStart invoked");
      return true;
    } catch (error) {
      appendTrace(`[speech] Android sttStart failed: ${formatError(error)}`);
      return false;
    }
  }

  appendTrace("[speech] sttStart unavailable on this platform");
  return false;
}

function stopSpeechRecognition() {
  if (state.androidBridge && typeof state.androidBridge.sttStop === "function") {
    try {
      state.androidBridge.sttStop();
      appendTrace("[speech] Android sttStop invoked");
      return true;
    } catch (error) {
      appendTrace(`[speech] Android sttStop failed: ${formatError(error)}`);
      return false;
    }
  }

  appendTrace("[speech] sttStop unavailable on this platform");
  return false;
}

function clearConversation() {
  state.history = [];
  state.memorySummary = "";

  writeStorageString(STORAGE_KEYS.history, "[]");
  writeStorageString(STORAGE_KEYS.memorySummary, "");

  elements.messages.innerHTML = "";
  elements.toolTrace.textContent = "";
  state.traceLines = [];

  appendMessage("system", "Conversation cleared.");
  setStatus("Ready.", "ok");
  appendTrace("[memory] conversation cleared");

  void persistConversationState();
}

function addHistoryEntry(entry) {
  const cleanEntry = sanitizeHistoryEntry(entry);
  if (!cleanEntry) {
    return;
  }

  state.history.push(cleanEntry);
  if (state.history.length > MAX_HISTORY_MESSAGES) {
    compressHistoryIntoMemory(false);
  }

  if (state.history.length > MAX_HISTORY_MESSAGES) {
    state.history = state.history.slice(-MAX_HISTORY_MESSAGES);
  }

  void persistConversationState();
}

function compressHistoryIntoMemory(silent) {
  if (state.history.length <= MAX_HISTORY_MESSAGES) {
    return;
  }

  const splitIndex = Math.max(0, state.history.length - Math.floor(MAX_HISTORY_MESSAGES * 0.7));
  const older = state.history.slice(0, splitIndex);
  const newer = state.history.slice(splitIndex);

  if (older.length === 0) {
    return;
  }

  const summaryChunk = summarizeMessages(older);
  if (summaryChunk) {
    state.memorySummary = mergeMemorySummary(state.memorySummary, summaryChunk);
    writeStorageString(STORAGE_KEYS.memorySummary, state.memorySummary);
  }

  state.history = newer;
  writeStorageString(STORAGE_KEYS.history, safeJSONStringify(state.history));

  renderRestoredConversation();

  if (!silent) {
    appendTrace(`[memory] summarized ${older.length} older message(s) into persistent memory`);
  }
}

function summarizeMessages(messages) {
  if (!Array.isArray(messages) || messages.length === 0) {
    return "";
  }

  const lines = [];
  const intents = new Set();

  for (const item of messages) {
    if (!item || (item.role !== "user" && item.role !== "assistant")) {
      continue;
    }

    if (typeof item.intent === "string" && item.intent) {
      intents.add(item.intent);
    }

    const compact = compactWhitespace(String(item.content || ""));
    if (!compact) {
      continue;
    }

    lines.push(`${item.role}: ${compact.slice(0, 180)}`);
    if (lines.length >= 12) {
      break;
    }
  }

  if (lines.length === 0) {
    return "";
  }

  const stamp = new Date().toISOString().slice(0, 19).replace("T", " ");
  const intentText = intents.size > 0 ? ` intents=${Array.from(intents).join(",")}` : "";
  return `[${stamp}]${intentText} ${lines.join(" | ")}`;
}

function mergeMemorySummary(existingSummary, chunk) {
  const merged = [existingSummary, chunk].filter(Boolean).join("\n");
  if (merged.length <= MAX_MEMORY_SUMMARY_CHARS) {
    return merged;
  }

  return merged.slice(-MAX_MEMORY_SUMMARY_CHARS);
}

async function persistConversationState() {
  writeStorageString(STORAGE_KEYS.history, safeJSONStringify(state.history.slice(-MAX_HISTORY_MESSAGES)));
  writeStorageString(STORAGE_KEYS.memorySummary, state.memorySummary);
  writeStorageString(
    STORAGE_KEYS.stylePrefs,
    safeJSONStringify({
      verbosity: state.stylePrefs.verbosity,
      tone: state.stylePrefs.tone
    })
  );

  writeStorageString(STORAGE_KEYS.personalityVector, safeJSONStringify(state.personalityVector));

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.saveMemoryState === "function") {
    try {
      await window.desktopApp.saveMemoryState({
        version: 3,
        history: state.history.slice(-MAX_HISTORY_MESSAGES),
        memorySummary: state.memorySummary,
        stylePrefs: {
          verbosity: state.stylePrefs.verbosity,
          tone: state.stylePrefs.tone
        },
        lastIntent: state.lastIntent,
        personality: state.personalityVector,
        personalityEnabled: state.personalityEnabled
      });
    } catch (error) {
      appendTrace(`[memory] desktop persistence failed: ${formatError(error)}`);
    }
  }
}

async function chooseVaultFiles() {
  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.pickFiles === "function") {
    try {
      const picked = await window.desktopApp.pickFiles({ multi: true });
      state.pendingDesktopPaths = Array.isArray(picked) ? picked : [];
      state.pendingAndroidFiles = [];

      elements.vaultSelected.textContent =
        state.pendingDesktopPaths.length > 0
          ? `${state.pendingDesktopPaths.length} desktop file(s) selected`
          : "No file selected";
      return;
    } catch (error) {
      appendTrace(`[vault] desktop file picker failed: ${formatError(error)}`);
    }
  }

  if (state.androidBridge && typeof state.androidBridge.pickFiles === "function") {
    try {
      const pickedMeta = await state.androidBridge.pickFiles();
      if (Array.isArray(pickedMeta) && pickedMeta.length > 0) {
        state.pendingAndroidFiles = pickedMeta;
        state.pendingDesktopPaths = [];
        elements.vaultSelected.textContent = `${pickedMeta.length} Android file(s) selected`;
        return;
      }
    } catch (error) {
      appendTrace(`[vault] Android native picker failed: ${formatError(error)}`);
    }
  }

  if (elements.vaultFileInput) {
    elements.vaultFileInput.click();
  }
}

async function ingestSelectedFilesToVault() {
  try {
    setStatus("Ingesting into vault...", "neutral");

    let ingested = [];

    if (state.pendingDesktopPaths.length > 0) {
      ingested = await ingestDesktopPathsIntoVault(state.pendingDesktopPaths);
      state.pendingDesktopPaths = [];
    } else if (state.pendingAndroidFiles.length > 0) {
      ingested = await ingestAndroidFilesIntoVault(state.pendingAndroidFiles);
      state.pendingAndroidFiles = [];
      if (elements.vaultFileInput) {
        elements.vaultFileInput.value = "";
      }
    } else {
      setStatus("No files selected for ingest.", "warn");
      return;
    }

    elements.vaultSelected.textContent = "No file selected";

    if (ingested.length === 0) {
      setStatus("No files ingested.", "warn");
      return;
    }

    setStatus(`Ingested ${ingested.length} file(s) into vault.`, "ok");
    appendTrace(`[vault] ingested ${ingested.length} item(s)`);

    for (const item of ingested) {
      void summarizeVaultItem(item);
    }

    renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
  } catch (error) {
    setStatus("Vault ingest failed.", "error");
    appendTrace(`[vault] ingest failed: ${formatError(error)}`);
  }
}

async function ingestDesktopPathsIntoVault(paths) {
  if (!window.desktopApp || typeof window.desktopApp.ingestVaultPaths !== "function") {
    throw new Error("Desktop vault ingest bridge unavailable");
  }

  const payload = {
    paths: Array.isArray(paths) ? paths : []
  };

  const ingested = await window.desktopApp.ingestVaultPaths(payload);
  const clean = sanitizeVaultIndex(ingested);
  mergeVaultItems(clean);
  await saveVaultIndex();
  return clean;
}

async function ingestAndroidFilesIntoVault(filesOrMeta) {
  const files = Array.isArray(filesOrMeta) ? filesOrMeta : [];
  const ingested = [];

  for (const file of files) {
    if (file && typeof file === "object" && typeof file.name === "string" && typeof file.size === "number" && file.uri) {
      const metaFromBridge = await ingestAndroidBridgeFile(file);
      if (metaFromBridge) {
        ingested.push(metaFromBridge);
      }
      continue;
    }

    if (!(file instanceof File)) {
      continue;
    }

    const arrayBuffer = await file.arrayBuffer();
    const bytes = new Uint8Array(arrayBuffer);
    const hash = await sha256Bytes(bytes);
    const baseMeta = {
      id: hash,
      type: inferFileTypeFromName(file.name, file.type),
      filename: file.name,
      source: "android",
      tags: [],
      createdAt: new Date().toISOString(),
      summaryPath: `vault/summaries/${hash}.json`,
      featuresPath: `vault/features/${hash}.json`,
      sizeBytes: file.size,
      summaryText: "",
      score: 10,
      referenceCount: 0,
      pinned: false,
      hidden: false,
      updatedAt: new Date().toISOString()
    };

    if (file.size <= ANDROID_MAX_BASE64_BYTES) {
      baseMeta.embeddedBase64 = bytesToBase64(bytes);
    } else {
      appendMessage(
        "system",
        `Android large file storage not supported without native file bridge: ${file.name}`
      );
    }

    state.vaultIndex = state.vaultIndex.filter((item) => item.id !== baseMeta.id);
    state.vaultIndex.push(baseMeta);
    ingested.push(baseMeta);
  }

  await saveVaultIndex();
  return ingested;
}

async function ingestAndroidBridgeFile(fileMeta) {
  const size = Number(fileMeta.size);
  const name = String(fileMeta.name || "android-file");
  const uri = String(fileMeta.uri || "");

  if (!uri) {
    return null;
  }

  const hash = await sha256String(`${uri}|${name}|${size}`);
  const item = {
    id: hash,
    type: inferFileTypeFromName(name, fileMeta.mimeType || ""),
    filename: name,
    source: "android",
    tags: [],
    createdAt: new Date().toISOString(),
    summaryPath: `vault/summaries/${hash}.json`,
    featuresPath: `vault/features/${hash}.json`,
    sizeBytes: Number.isFinite(size) ? size : 0,
    summaryText: "",
    score: 10,
    referenceCount: 0,
    pinned: false,
    hidden: false,
    updatedAt: new Date().toISOString(),
    androidUri: uri
  };

  if (state.androidBridge && typeof state.androidBridge.readPickedFileBase64 === "function" && size <= ANDROID_MAX_BASE64_BYTES) {
    try {
      const result = await state.androidBridge.readPickedFileBase64(uri, ANDROID_MAX_BASE64_BYTES);
      if (result && result.error === "too_large") {
        appendMessage(
          "system",
          `Android large file storage not supported without native file bridge: ${name}`
        );
      } else if (typeof result === "string" && result.trim()) {
        item.embeddedBase64 = result.trim();
      } else if (result && typeof result.base64 === "string") {
        item.embeddedBase64 = result.base64.trim();
      }
    } catch (error) {
      appendTrace(`[vault] Android readPickedFileBase64 failed: ${formatError(error)}`);
    }
  } else if (size > ANDROID_MAX_BASE64_BYTES) {
    appendMessage("system", `Android large file storage not supported without native file bridge: ${name}`);
  }

  state.vaultIndex = state.vaultIndex.filter((entry) => entry.id !== item.id);
  state.vaultIndex.push(item);
  await saveVaultIndex();
  return item;
}

function mergeVaultItems(items) {
  if (!Array.isArray(items) || items.length === 0) {
    return;
  }

  const byId = new Map(state.vaultIndex.map((item) => [item.id, item]));
  for (const item of items) {
    const merged = {
      ...(byId.get(item.id) || {}),
      ...item,
      updatedAt: new Date().toISOString()
    };
    byId.set(item.id, sanitizeVaultItem(merged));
  }

  state.vaultIndex = Array.from(byId.values()).sort((a, b) => {
    return new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime();
  });
}

function sanitizeVaultIndex(rawIndex) {
  if (!Array.isArray(rawIndex)) {
    return [];
  }

  const clean = [];
  const seen = new Set();

  for (const item of rawIndex) {
    const normalized = sanitizeVaultItem(item);
    if (!normalized || seen.has(normalized.id)) {
      continue;
    }

    seen.add(normalized.id);
    clean.push(normalized);
  }

  return clean;
}

function sanitizeVaultItem(item) {
  if (!item || typeof item !== "object") {
    return null;
  }

  const id = typeof item.id === "string" ? item.id.trim() : "";
  if (!id) {
    return null;
  }

  const tags = Array.isArray(item.tags)
    ? item.tags.filter((tag) => typeof tag === "string" && tag.trim()).map((tag) => tag.trim())
    : [];

  const clean = {
    id,
    type: typeof item.type === "string" ? item.type : "other",
    filename: typeof item.filename === "string" ? item.filename : `${id}.bin`,
    source: typeof item.source === "string" ? item.source : "desktop",
    tags,
    createdAt: typeof item.createdAt === "string" ? item.createdAt : new Date().toISOString(),
    updatedAt: typeof item.updatedAt === "string" ? item.updatedAt : new Date().toISOString(),
    summaryPath: typeof item.summaryPath === "string" ? item.summaryPath : `vault/summaries/${id}.json`,
    featuresPath: typeof item.featuresPath === "string" ? item.featuresPath : `vault/features/${id}.json`,
    filePath: typeof item.filePath === "string" ? item.filePath : "",
    sizeBytes: Number.isFinite(item.sizeBytes) ? item.sizeBytes : 0,
    summaryText: typeof item.summaryText === "string" ? item.summaryText : "",
    score: Number.isFinite(item.score) ? item.score : 10,
    referenceCount: Number.isFinite(item.referenceCount) ? item.referenceCount : 0,
    pinned: Boolean(item.pinned),
    hidden: Boolean(item.hidden),
    userSignal: item.userSignal === "liked" || item.userSignal === "disliked" ? item.userSignal : ""
  };

  if (typeof item.embeddedBase64 === "string") {
    clean.embeddedBase64 = item.embeddedBase64;
  }

  if (typeof item.androidUri === "string") {
    clean.androidUri = item.androidUri;
  }

  return clean;
}

function searchVaultItems(query) {
  const normalizedQuery = compactWhitespace(String(query || "").toLowerCase());
  const candidates = state.vaultIndex.filter((item) => !item.hidden);

  if (!normalizedQuery) {
    return rankVaultItems(candidates, { intentPrimary: state.lastIntent?.name || "general" }).slice(0, 12);
  }

  const tokens = normalizedQuery.split(" ").filter(Boolean);

  const matched = candidates.filter((item) => {
    const haystack = `${item.filename} ${item.tags.join(" ")} ${item.summaryText}`.toLowerCase();
    return tokens.every((token) => haystack.includes(token));
  });

  return rankVaultItems(matched, { intentPrimary: state.lastIntent?.name || "general" }).slice(0, 12);
}

function rankVaultItems(items, options) {
  if (window.AIFREDMemoryScore && typeof window.AIFREDMemoryScore.rank === "function") {
    return window.AIFREDMemoryScore.rank(items, options);
  }

  return [...items].sort((a, b) => (b.score || 0) - (a.score || 0));
}

function renderVaultResults(results) {
  if (!elements.vaultResults) {
    return;
  }

  const list = Array.isArray(results) ? results : [];
  elements.vaultResults.innerHTML = "";

  if (list.length === 0) {
    const empty = document.createElement("p");
    empty.className = "vault-result-meta";
    empty.textContent = "No vault items";
    elements.vaultResults.appendChild(empty);
    return;
  }

  for (const item of list) {
    const wrapper = document.createElement("div");
    wrapper.className = "vault-result";

    const title = document.createElement("p");
    title.className = "vault-result-title";
    title.textContent = `${item.filename} (${item.type})`;

    const meta = document.createElement("p");
    meta.className = "vault-result-meta";
    meta.textContent = `score=${toFixedNumber(item.score || 0, 1)} refs=${item.referenceCount || 0} ${
      item.tags.length > 0 ? `tags=${item.tags.join(",")}` : ""
    }`;

    const actions = document.createElement("div");
    actions.className = "vault-result-actions";

    const attachBtn = document.createElement("button");
    attachBtn.type = "button";
    attachBtn.textContent = "Attach";
    attachBtn.addEventListener("click", () => {
      void attachVaultItemToChat(item.id);
    });

    const pinBtn = document.createElement("button");
    pinBtn.type = "button";
    pinBtn.textContent = item.pinned ? "Unpin" : "Pin";
    pinBtn.addEventListener("click", () => {
      void togglePinVaultItem(item.id);
    });

    const hideBtn = document.createElement("button");
    hideBtn.type = "button";
    hideBtn.textContent = "Hide";
    hideBtn.addEventListener("click", () => {
      void hideVaultItem(item.id);
    });

    const forgetBtn = document.createElement("button");
    forgetBtn.type = "button";
    forgetBtn.textContent = "Forget";
    forgetBtn.addEventListener("click", () => {
      void forgetVaultItem(item.id);
    });

    actions.append(attachBtn, pinBtn, hideBtn, forgetBtn);
    wrapper.append(title, meta, actions);
    elements.vaultResults.appendChild(wrapper);
  }
}

async function attachVaultItemToChat(id) {
  const item = state.vaultIndex.find((entry) => entry.id === id);
  if (!item) {
    return;
  }

  const summary = item.summaryText || `No summary available for ${item.filename}`;
  const content = `Attached vault item: ${item.filename}\nSummary: ${summary}`;

  addHistoryEntry({
    role: "system",
    content,
    ts: Date.now(),
    intent: "memory_recall"
  });

  appendMessage("system", content);

  if (window.AIFREDMemoryScore && typeof window.AIFREDMemoryScore.applyReference === "function") {
    const updated = window.AIFREDMemoryScore.applyReference(item);
    replaceVaultItem(updated);
  } else {
    item.referenceCount = (item.referenceCount || 0) + 1;
    item.score = (item.score || 10) + 1;
    item.updatedAt = new Date().toISOString();
  }

  state.lastRetrievedVaultIds = [item.id];
  await saveVaultIndex();
  renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
}

async function togglePinVaultItem(id) {
  const item = state.vaultIndex.find((entry) => entry.id === id);
  if (!item) {
    return;
  }

  item.pinned = !item.pinned;
  item.updatedAt = new Date().toISOString();
  if (window.AIFREDMemoryScore) {
    item.score = window.AIFREDMemoryScore.computeScore(item, {
      intentPrimary: state.lastIntent?.name || "general"
    });
  }

  await saveVaultIndex();
  renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
}

async function hideVaultItem(id) {
  const item = state.vaultIndex.find((entry) => entry.id === id);
  if (!item) {
    return;
  }

  item.hidden = true;
  item.updatedAt = new Date().toISOString();
  if (window.AIFREDMemoryScore) {
    item.score = window.AIFREDMemoryScore.computeScore(item, {
      intentPrimary: state.lastIntent?.name || "general"
    });
  }

  await saveVaultIndex();
  renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
}

async function forgetVaultItem(id) {
  const idx = state.vaultIndex.findIndex((entry) => entry.id === id);
  if (idx < 0) {
    return;
  }

  const [item] = state.vaultIndex.splice(idx, 1);

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.deleteVaultItem === "function") {
    try {
      await window.desktopApp.deleteVaultItem({ id: item.id });
    } catch (error) {
      appendTrace(`[vault] desktop delete failed: ${formatError(error)}`);
    }
  }

  await saveVaultIndex();
  renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
}

function getRelevantVaultContext(query, intentSignal) {
  const cleanedQuery = compactWhitespace(String(query || "").toLowerCase());
  const tokens = cleanedQuery.split(" ").filter(Boolean);

  const visible = state.vaultIndex.filter((item) => !item.hidden);

  const ranked = visible
    .map((item) => {
      const haystack = `${item.filename} ${item.tags.join(" ")} ${item.summaryText}`.toLowerCase();
      let keywordScore = 0;
      for (const token of tokens) {
        if (token && haystack.includes(token)) {
          keywordScore += 1;
        }
      }

      const recentBonus = Math.max(0, 12 - daysSinceIso(item.updatedAt || item.createdAt));
      const memoryScore = window.AIFREDMemoryScore
        ? window.AIFREDMemoryScore.computeScore(item, {
            intentPrimary: intentSignal.primary.name
          })
        : item.score || 0;

      const combined = keywordScore * 18 + recentBonus + memoryScore;

      return {
        ...item,
        combined,
        keywordScore,
        recentBonus,
        memoryScore
      };
    })
    .sort((a, b) => b.combined - a.combined);

  const dynamicLimit = intentSignal.weights?.memory_recall > 0.25 || intentSignal.weights?.file_ops > 0.25 ? 5 : 3;
  const top = ranked.filter((item) => item.summaryText).slice(0, dynamicLimit);

  state.lastRetrievedVaultIds = top.map((item) => item.id);

  if (top.length > 0) {
    const reasons = top
      .map((item) => `${item.filename}(k=${item.keywordScore},r=${toFixedNumber(item.recentBonus, 1)},s=${toFixedNumber(item.memoryScore, 1)})`)
      .join(", ");
    appendTrace(`[memory] retrieved ${top.length} vault item(s): ${reasons}`);
  }

  return top;
}

function applyRetrievalFeedbackFromUserMessage(userText) {
  if (!Array.isArray(state.lastRetrievedVaultIds) || state.lastRetrievedVaultIds.length === 0) {
    return;
  }

  const text = String(userText || "").toLowerCase();
  let feedbackKind = null;
  const forgetSignal = /\b(forget this|don't use this|do not use this|stop using this)\b/.test(text);

  if (/\b(that'?s right|exactly|correct|yes that)\b/.test(text)) {
    feedbackKind = "positive";
  } else if (/\b(no|wrong|not right|incorrect)\b/.test(text)) {
    feedbackKind = "negative";
  }

  if (!feedbackKind && !forgetSignal) {
    return;
  }

  for (const id of state.lastRetrievedVaultIds) {
    const item = state.vaultIndex.find((entry) => entry.id === id);
    if (!item) {
      continue;
    }

    if (window.AIFREDMemoryScore && typeof window.AIFREDMemoryScore.applyFeedback === "function") {
      const updated = window.AIFREDMemoryScore.applyFeedback(item, feedbackKind || "negative");
      if (forgetSignal) {
        updated.hidden = true;
      }
      replaceVaultItem(updated);
    } else if (forgetSignal) {
      item.hidden = true;
      item.updatedAt = new Date().toISOString();
    }
  }

  void saveVaultIndex();
}

function replaceVaultItem(nextItem) {
  const idx = state.vaultIndex.findIndex((entry) => entry.id === nextItem.id);
  if (idx >= 0) {
    state.vaultIndex[idx] = sanitizeVaultItem(nextItem);
  }
}

async function summarizeVaultItem(item) {
  if (!item || !item.id) {
    return;
  }

  let contextSnippet = "";

  if (item.type === "text") {
    if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.readVaultTextSnippet === "function") {
      try {
        contextSnippet = await window.desktopApp.readVaultTextSnippet({ id: item.id, maxChars: 6000 });
      } catch (_) {
        contextSnippet = "";
      }
    } else if (item.embeddedBase64) {
      const bytes = base64ToBytes(item.embeddedBase64);
      contextSnippet = decodeBytesToText(bytes, "text/plain").slice(0, 6000);
    }
  }

  const summaryOutput = await generateSummaryAndTags(item, contextSnippet);
  if (!summaryOutput) {
    return;
  }

  item.summaryText = summaryOutput.summary;
  item.tags = Array.isArray(summaryOutput.tags) ? summaryOutput.tags.slice(0, 12) : [];
  item.updatedAt = new Date().toISOString();

  if (window.AIFREDMemoryScore) {
    item.score = window.AIFREDMemoryScore.computeScore(item, {
      intentPrimary: state.lastIntent?.name || "general"
    });
  }

  const summaryJson = {
    id: item.id,
    filename: item.filename,
    type: item.type,
    summary: item.summaryText,
    tags: item.tags,
    createdAt: item.createdAt,
    updatedAt: item.updatedAt
  };

  const featuresJson = buildMediaFeatures(item);

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.writeVaultJson === "function") {
    try {
      await window.desktopApp.writeVaultJson({
        path: item.summaryPath,
        data: summaryJson
      });

      if (item.type === "audio" || item.type === "video") {
        await window.desktopApp.writeVaultJson({
          path: item.featuresPath,
          data: featuresJson
        });
      }
    } catch (error) {
      appendTrace(`[vault] summary/features write failed: ${formatError(error)}`);
    }
  } else {
    writeStorageString(`${item.summaryPath}.local`, safeJSONStringify(summaryJson));
    if (item.type === "audio" || item.type === "video") {
      writeStorageString(`${item.featuresPath}.local`, safeJSONStringify(featuresJson));
    }
  }

  await saveVaultIndex();
  renderVaultResults(searchVaultItems(elements.vaultSearch?.value || ""));
}

async function generateSummaryAndTags(item, snippet) {
  const baseSummary = {
    summary: `${item.type.toUpperCase()} file: ${item.filename}`,
    tags: [
      `type:${item.type}`,
      `source:${item.source || "unknown"}`
    ]
  };

  if (!state.apiKey) {
    return baseSummary;
  }

  const prompt = [
    "Summarize this user vault item in <= 2 sentences and suggest 3 to 6 tags.",
    "Return strict JSON with keys: summary (string), tags (array of strings).",
    `filename: ${item.filename}`,
    `type: ${item.type}`,
    snippet ? `content_snippet: ${snippet.slice(0, 3500)}` : "content_snippet: (unavailable)"
  ].join("\n");

  const payload = {
    model: "gpt-4o-mini",
    messages: [
      {
        role: "system",
        content: "You generate compact vault metadata. Output only valid JSON."
      },
      {
        role: "user",
        content: prompt
      }
    ],
    temperature: 0.2,
    max_tokens: 220,
    stream: false
  };

  try {
    const response = await apiJsonRequest({
      url: OPENAI_CHAT_ENDPOINT,
      method: "POST",
      payload,
      headers: buildOpenAIHeaders(),
      timeoutMs: CHAT_TIMEOUT_MS
    });

    if (response.status !== 200) {
      throw buildApiError(response, {
        route: "cloud",
        model: "gpt-4o-mini",
        endpoint: OPENAI_CHAT_ENDPOINT
      });
    }

    const parsed = parseAssistantPayload(response.json, response.text);
    const maybeJson = parseJsonFromText(parsed.text);

    if (maybeJson && typeof maybeJson.summary === "string" && Array.isArray(maybeJson.tags)) {
      return {
        summary: maybeJson.summary.trim(),
        tags: maybeJson.tags.filter((tag) => typeof tag === "string" && tag.trim()).map((tag) => tag.trim())
      };
    }

    return {
      summary: parsed.text.slice(0, 240) || baseSummary.summary,
      tags: baseSummary.tags
    };
  } catch (error) {
    appendTrace(`[vault] summary generation failed: ${formatError(error)}`);
    return baseSummary;
  }
}

function buildMediaFeatures(item) {
  return {
    id: item.id,
    type: item.type,
    filename: item.filename,
    sizeBytes: item.sizeBytes,
    generatedAt: new Date().toISOString(),
    pipeline: "heuristic-v1"
  };
}

async function exportProfile() {
  if (!state.profile) {
    return;
  }

  if (!state.isAndroid && window.desktopApp && typeof window.desktopApp.exportProfile === "function") {
    try {
      const result = await window.desktopApp.exportProfile(state.profile);
      if (result?.ok) {
        appendTrace(`[profile] exported to ${result.path}`);
        setStatus("Profile exported.", "ok");
        return;
      }
    } catch (error) {
      appendTrace(`[profile] desktop export failed: ${formatError(error)}`);
    }
  }

  const blob = new Blob([safeJSONStringify(state.profile)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = `aifred_profile_${new Date().toISOString().slice(0, 10)}.json`;
  anchor.click();
  URL.revokeObjectURL(url);
  appendTrace("[profile] exported via browser download");
  setStatus("Profile exported.", "ok");
}

async function resetProfile() {
  if (!window.AIFREDProfileStore) {
    return;
  }

  state.profile = window.AIFREDProfileStore.defaultProfile();
  await persistProfile();
  appendTrace("[profile] reset");
  setStatus("Profile reset.", "ok");
}

function buildOpenAIHeaders() {
  return {
    "Content-Type": "application/json",
    Authorization: `Bearer ${state.apiKey}`
  };
}

async function apiJsonRequest(input) {
  const body = input.payload !== undefined ? safeJSONStringify(input.payload) : input.body;
  const response = await apiRawRequest({
    url: input.url,
    method: input.method || "GET",
    headers: {
      "Content-Type": "application/json",
      ...(input.headers || {})
    },
    body,
    timeoutMs: input.timeoutMs
  });

  let parsedJson = null;
  if (response.text) {
    parsedJson = safeParseJSON(response.text, null);
  }

  return {
    ...response,
    json: parsedJson
  };
}

async function apiRawRequest(input) {
  const method = input.method || "GET";
  const timeoutMs = clampInteger(input.timeoutMs, 1_000, 180_000, 30_000);

  if (window.desktopApp && typeof window.desktopApp.httpRequest === "function") {
    const desktopResponse = await window.desktopApp.httpRequest({
      url: input.url,
      method,
      headers: input.headers || {},
      body: input.body,
      timeoutMs
    });

    const bytes = base64ToBytes(desktopResponse?.bodyBase64 || "");
    const text = decodeBytesToText(bytes, desktopResponse?.contentType || "");

    return {
      ok: Boolean(desktopResponse?.ok),
      status: Number.isFinite(desktopResponse?.status) ? desktopResponse.status : 0,
      statusText: typeof desktopResponse?.statusText === "string" ? desktopResponse.statusText : "",
      contentType: typeof desktopResponse?.contentType === "string" ? desktopResponse.contentType : "",
      bytes,
      text
    };
  }

  const controller = new AbortController();
  const timer = setTimeout(() => {
    controller.abort();
  }, timeoutMs);

  try {
    const response = await fetch(input.url, {
      method,
      headers: input.headers,
      body: method === "GET" ? undefined : input.body,
      signal: controller.signal
    });

    const buffer = await response.arrayBuffer();
    const bytes = new Uint8Array(buffer);
    const contentType = response.headers.get("content-type") || "";
    const text = decodeBytesToText(bytes, contentType);

    return {
      ok: response.ok,
      status: response.status,
      statusText: response.statusText,
      contentType,
      bytes,
      text
    };
  } catch (error) {
    if (error?.name === "AbortError") {
      const timeoutError = new Error(`Request timed out after ${Math.floor(timeoutMs / 1000)}s`);
      timeoutError.name = "TimeoutError";
      throw timeoutError;
    }

    throw error;
  } finally {
    clearTimeout(timer);
  }
}

function buildApiError(response, context) {
  const jsonError = response?.json?.error;
  const extractedMessage =
    (jsonError && typeof jsonError.message === "string" && jsonError.message) ||
    (typeof jsonError === "string" && jsonError) ||
    response?.statusText ||
    "Unknown API failure";

  const detailsPayload = {
    status: response?.status || 0,
    route: context?.route || "unknown",
    model: context?.model || "unknown",
    endpoint: context?.endpoint || "",
    message: extractedMessage,
    raw: response?.text?.slice(0, 1200) || ""
  };

  const error = new Error(`HTTP ${detailsPayload.status}: ${extractedMessage}`);
  error.name = "ApiError";
  error.status = detailsPayload.status;
  error.userMessage = `Request failed (${detailsPayload.status}): ${extractedMessage}`;
  error.details = detailsPayload;
  return error;
}

function formatUserFacingError(error) {
  if (error?.userMessage && typeof error.userMessage === "string") {
    return error.userMessage;
  }

  if (error?.name === "MissingApiKeyError") {
    return "OpenAI API key not configured.";
  }

  return `Request failed: ${formatError(error)}`;
}

function formatErrorDetails(error) {
  if (error?.details) {
    return safeJSONStringify(error.details);
  }

  return formatError(error);
}

function appendTrace(line) {
  const timestamp = new Date().toLocaleTimeString();
  const formatted = `${timestamp} ${line}`;

  state.traceLines.push(formatted);
  if (state.traceLines.length > MAX_TRACE_LINES) {
    state.traceLines = state.traceLines.slice(-MAX_TRACE_LINES);
  }

  elements.toolTrace.textContent = state.traceLines.join("\n");
  elements.toolTrace.scrollTop = elements.toolTrace.scrollHeight;
}

function setBusy(isBusy) {
  state.busy = isBusy;
  elements.sendBtn.textContent = isBusy ? "Stop" : "Send";
  elements.clearBtn.disabled = isBusy;
  elements.refreshModelsBtn.disabled = isBusy;
}

function beginRequest() {
  state.activeRequestToken += 1;
  state.localOpsApprovedForRequestToken = null;
  setBusy(true);
  setStatus("Thinking...", "neutral");
  return state.activeRequestToken;
}

function endRequest(requestToken) {
  if (requestToken === state.activeRequestToken) {
    setBusy(false);
  }
}

function cancelActiveRequest(statusText) {
  if (!state.busy) {
    return;
  }

  state.activeRequestToken += 1;
  setBusy(false);
  setStatus(statusText || "Stopped.", "warn");
  appendTrace("[cancel] request stopped by user");
}

function ensureRequestActive(requestToken) {
  if (requestToken !== state.activeRequestToken) {
    const cancelledError = new Error("Request cancelled");
    cancelledError.name = "RequestCancelledError";
    throw cancelledError;
  }
}

function isRequestCancelledError(error) {
  return error?.name === "RequestCancelledError";
}

function setStatus(text, tone) {
  elements.status.textContent = text;
  elements.status.dataset.tone = tone;
}

function sanitizeHistory(rawHistory) {
  if (!Array.isArray(rawHistory)) {
    return [];
  }

  const clean = [];
  for (const item of rawHistory) {
    const entry = sanitizeHistoryEntry(item);
    if (entry) {
      clean.push(entry);
    }
  }

  return clean;
}

function sanitizeHistoryEntry(entry) {
  if (!entry || typeof entry !== "object") {
    return null;
  }

  const role = typeof entry.role === "string" ? entry.role : "";
  if (!["system", "user", "assistant", "tool"].includes(role)) {
    return null;
  }

  const clean = {
    role,
    content: typeof entry.content === "string" ? entry.content : "",
    ts: Number.isFinite(entry.ts) ? entry.ts : Date.now()
  };

  if (role === "tool") {
    clean.tool_call_id = typeof entry.tool_call_id === "string" ? entry.tool_call_id : "tool";
  }

  if (typeof entry.intent === "string") {
    clean.intent = entry.intent;
  }

  if (Array.isArray(entry.tool_calls) && entry.tool_calls.length > 0) {
    clean.tool_calls = entry.tool_calls.map(normalizeToolCall).filter((item) => item.function.name);
  }

  return clean;
}

function rebuildModelSelect(modelIds, preferredModel) {
  const sanitized = sanitizeModelList(modelIds);
  if (sanitized.length === 0) {
    sanitized.push(FALLBACK_CHAT_MODEL);
  }

  elements.modelSelect.innerHTML = "";

  for (const modelId of sanitized) {
    const option = document.createElement("option");
    option.value = modelId;
    option.textContent = modelId;
    elements.modelSelect.appendChild(option);
  }

  const selected = choosePreferredModel(sanitized, preferredModel);
  if (selected) {
    elements.modelSelect.value = selected;
    writeStorageString(STORAGE_KEYS.selectedModel, selected);
  }
}

function choosePreferredModel(modelIds, preferredModel) {
  const preferred = normalizeModelId(preferredModel);
  if (preferred && modelIds.includes(preferred)) {
    return preferred;
  }

  const storedPreferred = normalizeModelId(readStorageString(STORAGE_KEYS.selectedModel, ""));
  if (storedPreferred && modelIds.includes(storedPreferred)) {
    return storedPreferred;
  }

  if (modelIds.includes(DEFAULT_CHAT_MODEL)) {
    return DEFAULT_CHAT_MODEL;
  }

  if (modelIds.includes(FALLBACK_CHAT_MODEL)) {
    return FALLBACK_CHAT_MODEL;
  }

  return modelIds[0] || FALLBACK_CHAT_MODEL;
}

function getPreferredModel() {
  return normalizeModelId(readStorageString(STORAGE_KEYS.selectedModel, "")) || DEFAULT_CHAT_MODEL;
}

function sanitizeModelList(list) {
  if (!Array.isArray(list)) {
    return [];
  }

  const out = [];
  const seen = new Set();

  for (const item of list) {
    const normalized = normalizeModelId(item);
    if (!normalized) {
      continue;
    }

    if (!SUPPORTED_CHAT_MODELS.includes(normalized)) {
      continue;
    }

    const lowered = normalized.toLowerCase();
    if (seen.has(lowered)) {
      continue;
    }

    seen.add(lowered);
    out.push(normalized);
  }

  return out;
}

function normalizeModelId(value) {
  if (typeof value !== "string") {
    return "";
  }

  const trimmed = value.trim();
  if (!trimmed) {
    return "";
  }

  if (!/^[a-zA-Z0-9._:/-]+$/.test(trimmed)) {
    return "";
  }

  return trimmed;
}

function optionExists(selectElement, value) {
  if (!selectElement || typeof value !== "string" || !value) {
    return false;
  }

  return Array.from(selectElement.options).some((option) => option.value === value);
}

function updateStylePreferences(userText) {
  const text = String(userText || "").toLowerCase();
  const before = `${state.stylePrefs.verbosity}|${state.stylePrefs.tone}`;

  if (/\b(short|brief|concise|tl;dr|tldr|one[- ]liner|minimal)\b/.test(text)) {
    state.stylePrefs.verbosity = "short";
  } else if (/\b(detailed|detail|in[- ]depth|deep dive|thorough|verbose|step[- ]by[- ]step)\b/.test(text)) {
    state.stylePrefs.verbosity = "detailed";
  } else if (/\b(balanced|normal length|medium length)\b/.test(text)) {
    state.stylePrefs.verbosity = "balanced";
  }

  if (/\b(friendly|casual|warm|chill|conversational)\b/.test(text)) {
    state.stylePrefs.tone = "friendly";
  } else if (/\b(professional|formal|business|objective)\b/.test(text)) {
    state.stylePrefs.tone = "professional";
  } else if (/\b(blunt|direct|no fluff|straight to the point)\b/.test(text)) {
    state.stylePrefs.tone = "blunt";
  }

  writeStorageString(
    STORAGE_KEYS.stylePrefs,
    safeJSONStringify({
      verbosity: state.stylePrefs.verbosity,
      tone: state.stylePrefs.tone
    })
  );

  const after = `${state.stylePrefs.verbosity}|${state.stylePrefs.tone}`;
  return before !== after;
}

function normalizeVerbosity(value) {
  if (value === "short" || value === "balanced" || value === "detailed") {
    return value;
  }
  return STYLE_DEFAULTS.verbosity;
}

function normalizeTone(value) {
  if (value === "friendly" || value === "professional" || value === "blunt") {
    return value;
  }
  return STYLE_DEFAULTS.tone;
}

function decodeBytesToText(bytes, contentType) {
  if (!(bytes instanceof Uint8Array) || bytes.length === 0) {
    return "";
  }

  const type = String(contentType || "").toLowerCase();
  const probablyText =
    type.includes("application/json") ||
    type.includes("text/") ||
    type.includes("application/javascript") ||
    type.includes("application/xml") ||
    type.includes("application/x-www-form-urlencoded") ||
    type.includes("application/problem+json");

  if (!probablyText) {
    try {
      return new TextDecoder().decode(bytes);
    } catch (_) {
      return "";
    }
  }

  try {
    return new TextDecoder().decode(bytes);
  } catch (_) {
    return "";
  }
}

function base64ToBytes(value) {
  if (typeof value !== "string" || !value) {
    return new Uint8Array();
  }

  try {
    const binary = atob(value);
    const bytes = new Uint8Array(binary.length);
    for (let index = 0; index < binary.length; index += 1) {
      bytes[index] = binary.charCodeAt(index);
    }
    return bytes;
  } catch (_) {
    return new Uint8Array();
  }
}

function bytesToBase64(bytes) {
  if (!(bytes instanceof Uint8Array) || bytes.length === 0) {
    return "";
  }

  let binary = "";
  const chunkSize = 0x8000;
  for (let i = 0; i < bytes.length; i += chunkSize) {
    const chunk = bytes.subarray(i, i + chunkSize);
    binary += String.fromCharCode(...chunk);
  }
  return btoa(binary);
}

function parseJsonFromText(text) {
  const source = String(text || "").trim();
  if (!source) {
    return null;
  }

  const direct = safeParseJSON(source, null);
  if (direct && typeof direct === "object") {
    return direct;
  }

  const match = source.match(/\{[\s\S]*\}/);
  if (!match) {
    return null;
  }

  return safeParseJSON(match[0], null);
}

function compactWhitespace(value) {
  return String(value || "").replace(/\s+/g, " ").trim();
}

function toConfidence(value) {
  const number = Number(value);
  if (!Number.isFinite(number)) {
    return 0.5;
  }

  return Math.max(0.01, Math.min(0.99, number));
}

function toFixedNumber(value, digits) {
  const number = Number(value);
  if (!Number.isFinite(number)) {
    return Number(0).toFixed(digits);
  }
  return number.toFixed(digits);
}

function clampInteger(value, min, max, defaultValue) {
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed)) {
    return defaultValue;
  }

  return Math.min(Math.max(parsed, min), max);
}

function safeParseJSON(rawValue, fallbackValue) {
  if (typeof rawValue !== "string") {
    return fallbackValue;
  }

  try {
    return JSON.parse(rawValue);
  } catch (_) {
    return fallbackValue;
  }
}

function safeJSONStringify(value) {
  try {
    return JSON.stringify(value);
  } catch (_) {
    return "{}";
  }
}

function formatError(error) {
  if (!error) {
    return "Unknown error";
  }

  function confirmAction(message) {
    return window.confirm(message);
  }

  if (typeof error.message === "string" && error.message) {
    return error.message;
  }

  return safeJSONStringify(error);
}

function readStorageString(key, defaultValue) {
  try {
    const value = window.localStorage.getItem(key);
    return value === null ? defaultValue : value;
  } catch (_) {
    return defaultValue;
  }
}

function writeStorageString(key, value) {
  try {
    window.localStorage.setItem(key, String(value));
  } catch (_) {
    // no-op
  }
}

function readStorageBool(key, defaultValue) {
  const raw = readStorageString(key, defaultValue ? "1" : "0");
  return raw === "1" || raw === "true";
}

function writeStorageBool(key, value) {
  writeStorageString(key, value ? "1" : "0");
}

function inferFileTypeFromName(name, mime) {
  const loweredName = String(name || "").toLowerCase();
  const loweredMime = String(mime || "").toLowerCase();

  if (loweredMime.startsWith("image/") || /\.(png|jpg|jpeg|gif|webp|bmp|svg)$/.test(loweredName)) {
    return "image";
  }

  if (loweredMime.startsWith("audio/") || /\.(mp3|wav|m4a|flac|ogg|aac)$/.test(loweredName)) {
    return "audio";
  }

  if (loweredMime.startsWith("video/") || /\.(mp4|mov|mkv|webm|avi|m4v)$/.test(loweredName)) {
    return "video";
  }

  if (/\.pdf$/.test(loweredName) || loweredMime === "application/pdf") {
    return "pdf";
  }

  if (
    loweredMime.startsWith("text/") ||
    /\.(txt|md|json|csv|log|yaml|yml|xml|html|js|ts|py|java|c|cpp|rb|go|rs)$/.test(loweredName)
  ) {
    return "text";
  }

  return "other";
}

function daysSinceIso(isoString) {
  if (typeof isoString !== "string" || !isoString) {
    return 365;
  }

  const then = new Date(isoString).getTime();
  if (!Number.isFinite(then)) {
    return 365;
  }

  return Math.max(0, (Date.now() - then) / (24 * 60 * 60 * 1000));
}

async function sha256Bytes(bytes) {
  const hashBuffer = await crypto.subtle.digest("SHA-256", bytes);
  return [...new Uint8Array(hashBuffer)].map((b) => b.toString(16).padStart(2, "0")).join("");
}

async function sha256String(value) {
  const encoder = new TextEncoder();
  const bytes = encoder.encode(String(value || ""));
  return sha256Bytes(bytes);
}
