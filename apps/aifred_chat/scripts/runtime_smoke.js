#!/usr/bin/env node
const fs = require("fs");
const path = require("path");

const appPath = path.join(__dirname, "..", "src", "renderer", "app.js");
const readmePath = path.join(__dirname, "..", "README.md");

function read(filePath) {
  return fs.readFileSync(filePath, "utf8");
}

function expect(content, regex, label, failures) {
  if (!regex.test(content)) {
    failures.push(label);
  }
}

function run() {
  const failures = [];
  const appJs = read(appPath);
  const readme = read(readmePath);

  expect(appJs, /const\s+POLLINATIONS_BASE_URL\s*=\s*"https:\/\/text\.pollinations\.ai"/, "pollinations base URL", failures);
  expect(appJs, /const\s+MODELS_ENDPOINT\s*=\s*`\$\{POLLINATIONS_BASE_URL\}\/models`/, "models endpoint", failures);
  expect(appJs, /const\s+OPENAI_ENDPOINT\s*=\s*`\$\{POLLINATIONS_BASE_URL\}\/openai`/, "openai endpoint", failures);
  expect(appJs, /SEEDED_CHAT_MODELS\s*=\s*\["openai",\s*"openai-large"\]/, "seeded model list", failures);
  expect(appJs, /MODEL_CACHE_TTL_MS\s*=\s*24 \* 60 \* 60 \* 1000/, "24h model cache TTL", failures);
  expect(appJs, /function\s+detectIntent\(/, "intent parser", failures);
  expect(appJs, /Template directive: Goal -> Steps -> Notes\/Pitfalls -> Next action/, "technical template directive", failures);
  expect(appJs, /Template directive: Concept -> Example -> Common mistake -> Quick check/, "explain template directive", failures);
  expect(appJs, /Template directive: What you're hearing -> Why -> Do this next -> Avoid/, "music template directive", failures);
  expect(appJs, /aifred_history_v2/, "persistent chat history key", failures);
  expect(appJs, /aifred_memory_summary_v2/, "memory summary key", failures);
  expect(appJs, /state\.history\.length\s*>\s*MAX_HISTORY_MESSAGES/, "history bound check", failures);
  expect(appJs, /\[fallback\]/, "model fallback trace", failures);
  expect(appJs, /window\.AIFRED_ANDROID/, "android bridge detection", failures);
  expect(appJs, /ttsSpeak/, "native tts bridge", failures);
  expect(appJs, /sttStart/, "native stt start bridge", failures);
  expect(appJs, /sttStop/, "native stt stop bridge", failures);
  expect(readme, /AIFRED-Mobile-debug\.apk/, "README APK link", failures);

  if (failures.length > 0) {
    console.error("runtime_smoke failed checks:");
    for (const failure of failures) {
      console.error(`- ${failure}`);
    }
    process.exit(1);
  }

  console.log("runtime_smoke: all static checks passed");
}

run();
