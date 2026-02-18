const path = require("path");
const fs = require("fs");
const fsPromises = require("fs/promises");
const crypto = require("crypto");
const { app, BrowserWindow, ipcMain, dialog } = require("electron");

const OPENAI_HOST = "api.openai.com";
const LEGACY_HOST = "text.pollinations.ai";
const LOOPBACK_HOSTS = new Set(["localhost", "127.0.0.1", "::1"]);

const DEFAULT_BRIDGE_TIMEOUT_MS = 30_000;
const MIN_BRIDGE_TIMEOUT_MS = 1_000;
const MAX_BRIDGE_TIMEOUT_MS = 180_000;
const MAX_DESKTOP_TEXT_SNIPPET = 200_000;

const FILETYPE_EXTENSION_MAP = {
  text: new Set([".txt", ".md", ".json", ".csv", ".log", ".yaml", ".yml", ".xml", ".html", ".js", ".ts", ".py", ".java", ".c", ".cpp", ".rb", ".go", ".rs"]),
  image: new Set([".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".svg"]),
  audio: new Set([".mp3", ".wav", ".m4a", ".flac", ".ogg", ".aac"]),
  video: new Set([".mp4", ".mov", ".mkv", ".webm", ".avi", ".m4v"]),
  pdf: new Set([".pdf"])
};

function createWindow() {
  const win = new BrowserWindow({
    width: 1380,
    height: 900,
    minWidth: 1080,
    minHeight: 720,
    title: "AIFRED Desktop",
    backgroundColor: "#03080f",
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: false,
      spellcheck: false
    }
  });

  win.webContents.setWindowOpenHandler(({ url }) => {
    void shell.openExternal(url);
    return { action: "deny" };
  });

  win.loadFile(path.join(__dirname, "renderer", "index.html"));
}

function clampTimeout(timeoutMs) {
  const parsed = Number.parseInt(timeoutMs, 10);
  if (!Number.isFinite(parsed)) {
    return DEFAULT_BRIDGE_TIMEOUT_MS;
  }

  return Math.min(Math.max(parsed, MIN_BRIDGE_TIMEOUT_MS), MAX_BRIDGE_TIMEOUT_MS);
}

function sanitizeHeaders(headers) {
  if (!headers || typeof headers !== "object") {
    return {};
  }

  const output = {};
  for (const [key, value] of Object.entries(headers)) {
    if (typeof key !== "string" || !key.trim()) {
      continue;
    }

    if (typeof value !== "string") {
      continue;
    }

    output[key] = value;
  }

  return output;
}

function isHostAllowed(hostname) {
  if (!hostname) {
    return false;
  }

  if (hostname === OPENAI_HOST || hostname === LEGACY_HOST) {
    return true;
  }

  if (LOOPBACK_HOSTS.has(hostname)) {
    return true;
  }

  return false;
}

function protocolAllowed(protocol, hostname) {
  if (protocol === "https:") {
    return true;
  }

  if (protocol === "http:" && LOOPBACK_HOSTS.has(hostname)) {
    return true;
  }

  return false;
}

function getUserDataPath() {
  return app.getPath("userData");
}

function getMemoryFilePath() {
  return path.join(getUserDataPath(), "aifred_memory.json");
}

function getVaultRootPath() {
  return path.join(getUserDataPath(), "vault");
}

function getVaultIndexPath() {
  return path.join(getVaultRootPath(), "index.json");
}

function getVaultFilesPath() {
  return path.join(getVaultRootPath(), "files");
}

function getVaultNotesPath() {
  return path.join(getVaultRootPath(), "notes");
}

function getVaultSummariesPath() {
  return path.join(getVaultRootPath(), "summaries");
}

function getVaultFeaturesPath() {
  return path.join(getVaultRootPath(), "features");
}

function getVaultProfilePath() {
  return path.join(getVaultRootPath(), "profile.json");
}

function relativeVaultSummaryPath(id) {
  return `vault/summaries/${id}.json`;
}

function relativeVaultFeaturesPath(id) {
  return `vault/features/${id}.json`;
}

function resolveVaultRelativePath(relativePath) {
  const target = String(relativePath || "").trim().replace(/^\/+/, "");
  if (!target.startsWith("vault/")) {
    throw new Error("Vault path must start with vault/");
  }

  const absolute = path.resolve(getUserDataPath(), target);
  const allowedRoot = path.resolve(getVaultRootPath()) + path.sep;
  if (!absolute.startsWith(allowedRoot)) {
    throw new Error("Vault path traversal blocked");
  }

  return absolute;
}

async function ensureVaultStructure() {
  await fsPromises.mkdir(getVaultRootPath(), { recursive: true });
  await fsPromises.mkdir(getVaultFilesPath(), { recursive: true });
  await fsPromises.mkdir(getVaultNotesPath(), { recursive: true });
  await fsPromises.mkdir(getVaultSummariesPath(), { recursive: true });
  await fsPromises.mkdir(getVaultFeaturesPath(), { recursive: true });
}

async function readJsonFile(filePath, fallbackValue) {
  try {
    const raw = await fsPromises.readFile(filePath, "utf8");
    return JSON.parse(raw);
  } catch (_) {
    return fallbackValue;
  }
}

async function writeJsonFile(filePath, data) {
  await fsPromises.mkdir(path.dirname(filePath), { recursive: true });
  await fsPromises.writeFile(filePath, JSON.stringify(data, null, 2), "utf8");
}

function inferFileType(extension) {
  const ext = String(extension || "").toLowerCase();
  for (const [type, set] of Object.entries(FILETYPE_EXTENSION_MAP)) {
    if (set.has(ext)) {
      return type;
    }
  }
  return "other";
}

function sanitizeIndexEntry(entry) {
  if (!entry || typeof entry !== "object") {
    return null;
  }

  const id = typeof entry.id === "string" ? entry.id.trim() : "";
  if (!id) {
    return null;
  }

  const type = typeof entry.type === "string" && entry.type ? entry.type : "other";
  const filename = typeof entry.filename === "string" ? entry.filename : `${id}.bin`;
  const source = typeof entry.source === "string" ? entry.source : "desktop";
  const tags = Array.isArray(entry.tags)
    ? entry.tags.filter((tag) => typeof tag === "string" && tag.trim()).map((tag) => tag.trim())
    : [];

  return {
    id,
    type,
    filename,
    source,
    tags,
    createdAt: typeof entry.createdAt === "string" ? entry.createdAt : new Date().toISOString(),
    summaryPath: typeof entry.summaryPath === "string" ? entry.summaryPath : relativeVaultSummaryPath(id),
    featuresPath: typeof entry.featuresPath === "string" ? entry.featuresPath : relativeVaultFeaturesPath(id),
    filePath: typeof entry.filePath === "string" ? entry.filePath : `vault/files/${id}`,
    sizeBytes: Number.isFinite(entry.sizeBytes) ? entry.sizeBytes : 0,
    summaryText: typeof entry.summaryText === "string" ? entry.summaryText : "",
    score: Number.isFinite(entry.score) ? entry.score : 10,
    referenceCount: Number.isFinite(entry.referenceCount) ? entry.referenceCount : 0,
    pinned: Boolean(entry.pinned),
    hidden: Boolean(entry.hidden)
  };
}

async function readVaultIndex() {
  await ensureVaultStructure();
  const raw = await readJsonFile(getVaultIndexPath(), []);
  if (!Array.isArray(raw)) {
    return [];
  }

  return raw.map(sanitizeIndexEntry).filter(Boolean);
}

async function writeVaultIndex(index) {
  const clean = Array.isArray(index) ? index.map(sanitizeIndexEntry).filter(Boolean) : [];
  await writeJsonFile(getVaultIndexPath(), clean);
  return clean;
}

function toVaultFilePathByMeta(meta) {
  const relative = String(meta.filePath || "").replace(/^\/+/, "");
  const absolute = path.resolve(getUserDataPath(), relative);
  const filesRoot = path.resolve(getVaultFilesPath()) + path.sep;
  if (!absolute.startsWith(filesRoot)) {
    throw new Error("Invalid vault file path");
  }
  return absolute;
}

function sha256Buffer(buffer) {
  return crypto.createHash("sha256").update(buffer).digest("hex");
}

async function hashFile(filePath) {
  const hash = crypto.createHash("sha256");
  await new Promise((resolve, reject) => {
    const stream = fs.createReadStream(filePath);
    stream.on("error", reject);
    stream.on("data", (chunk) => hash.update(chunk));
    stream.on("end", resolve);
  });
  return hash.digest("hex");
}

async function ingestDesktopFilePaths(filePaths) {
  await ensureVaultStructure();
  const index = await readVaultIndex();
  const byId = new Map(index.map((item) => [item.id, item]));

  const ingested = [];

  for (const candidate of filePaths) {
    if (typeof candidate !== "string" || !candidate.trim()) {
      continue;
    }

    const sourcePath = candidate.trim();
    let stat;
    try {
      stat = await fsPromises.stat(sourcePath);
    } catch (_) {
      continue;
    }

    if (!stat.isFile()) {
      continue;
    }

    const fileHash = await hashFile(sourcePath);
    const extension = path.extname(sourcePath).toLowerCase() || ".bin";
    const targetFilename = `${fileHash}${extension}`;
    const targetAbsolutePath = path.join(getVaultFilesPath(), targetFilename);

    try {
      await fsPromises.access(targetAbsolutePath);
    } catch (_) {
      await fsPromises.copyFile(sourcePath, targetAbsolutePath);
    }

    const existing = byId.get(fileHash);
    const metadata = {
      id: fileHash,
      type: inferFileType(extension),
      filename: path.basename(sourcePath),
      source: "desktop",
      tags: existing?.tags || [],
      createdAt: existing?.createdAt || new Date().toISOString(),
      summaryPath: existing?.summaryPath || relativeVaultSummaryPath(fileHash),
      featuresPath: existing?.featuresPath || relativeVaultFeaturesPath(fileHash),
      filePath: `vault/files/${targetFilename}`,
      sizeBytes: stat.size,
      summaryText: existing?.summaryText || "",
      score: Number.isFinite(existing?.score) ? existing.score : 10,
      referenceCount: Number.isFinite(existing?.referenceCount) ? existing.referenceCount : 0,
      pinned: Boolean(existing?.pinned),
      hidden: Boolean(existing?.hidden)
    };

    byId.set(fileHash, metadata);
    ingested.push(metadata);
  }

  const mergedIndex = Array.from(byId.values()).sort((a, b) => {
    return new Date(b.createdAt).getTime() - new Date(a.createdAt).getTime();
  });

  await writeVaultIndex(mergedIndex);
  return ingested;
}

async function writeVaultJson(relativePath, data) {
  const absolute = resolveVaultRelativePath(relativePath);
  await writeJsonFile(absolute, data);
}

async function readVaultJson(relativePath, fallback) {
  const absolute = resolveVaultRelativePath(relativePath);
  return readJsonFile(absolute, fallback);
}

function normalizeFileTypes(fileTypes) {
  if (!Array.isArray(fileTypes) || fileTypes.length === 0) {
    return new Set(["text", "image", "audio", "video", "pdf", "other"]);
  }

  const normalized = new Set();
  for (const fileType of fileTypes) {
    if (typeof fileType !== "string") {
      continue;
    }

    const trimmed = fileType.trim().toLowerCase();
    if (["text", "image", "audio", "video", "pdf", "other"].includes(trimmed)) {
      normalized.add(trimmed);
    }
  }

  if (normalized.size === 0) {
    normalized.add("text");
    normalized.add("image");
    normalized.add("audio");
    normalized.add("video");
    normalized.add("pdf");
    normalized.add("other");
  }

  return normalized;
}

async function collectFolderFiles(rootPath, recursive, fileTypes) {
  const allowedTypes = normalizeFileTypes(fileTypes);
  const output = [];

  async function visit(currentPath) {
    const entries = await fsPromises.readdir(currentPath, { withFileTypes: true });
    for (const entry of entries) {
      const absolute = path.join(currentPath, entry.name);
      if (entry.isDirectory()) {
        if (recursive) {
          await visit(absolute);
        }
        continue;
      }

      if (!entry.isFile()) {
        continue;
      }

      const extension = path.extname(entry.name).toLowerCase();
      const inferredType = inferFileType(extension);
      if (!allowedTypes.has(inferredType)) {
        continue;
      }

      output.push(absolute);
    }
  }

  await visit(rootPath);
  return output;
}

ipcMain.handle("desktop:get-openai-key", async () => {
  return process.env.OPENAI_API_KEY || "";
});

ipcMain.handle("desktop:http-request", async (_event, request) => {
  try {
    const payload = request && typeof request === "object" ? request : {};
    const targetUrl = typeof payload.url === "string" ? payload.url : "";

    if (!targetUrl) {
      throw new Error("Missing request URL");
    }

    const parsedUrl = new URL(targetUrl);
    if (!protocolAllowed(parsedUrl.protocol, parsedUrl.hostname)) {
      throw new Error(`Protocol not allowed: ${parsedUrl.protocol}`);
    }

    if (!isHostAllowed(parsedUrl.hostname)) {
      throw new Error(`Host not allowed: ${parsedUrl.hostname}`);
    }

    const method = typeof payload.method === "string" ? payload.method.toUpperCase() : "GET";
    const timeoutMs = clampTimeout(payload.timeoutMs);
    const headers = sanitizeHeaders(payload.headers);
    const body = method === "GET" ? undefined : payload.body;

    const controller = new AbortController();
    const timer = setTimeout(() => {
      controller.abort();
    }, timeoutMs);

    let response;
    try {
      response = await fetch(parsedUrl.toString(), {
        method,
        headers,
        body,
        signal: controller.signal
      });
    } finally {
      clearTimeout(timer);
    }

    const contentType = response.headers.get("content-type") || "";
    const buffer = Buffer.from(await response.arrayBuffer());

    return {
      ok: response.ok,
      status: response.status,
      statusText: response.statusText,
      contentType,
      bodyBase64: buffer.toString("base64")
    };
  } catch (error) {
    const body = JSON.stringify({
      error: error?.message || "desktop http bridge failed"
    });

    return {
      ok: false,
      status: 0,
      statusText: "desktop bridge error",
      contentType: "application/json",
      bodyBase64: Buffer.from(body, "utf8").toString("base64")
    };
  }
});

ipcMain.handle("desktop:memory-load", async () => {
  const memoryPath = getMemoryFilePath();
  return readJsonFile(memoryPath, null);
});

ipcMain.handle("desktop:memory-save", async (_event, payload) => {
  const memoryPath = getMemoryFilePath();
  await writeJsonFile(memoryPath, payload && typeof payload === "object" ? payload : {});
  return { ok: true, path: memoryPath };
});

ipcMain.handle("desktop:vault-pick-files", async (_event, payload) => {
  const options = payload && typeof payload === "object" ? payload : {};
  const multi = Boolean(options.multi !== false);

  const response = await dialog.showOpenDialog({
    title: "Select file(s) for Memory Vault",
    properties: multi ? ["openFile", "multiSelections"] : ["openFile"]
  });

  if (response.canceled) {
    return [];
  }

  return response.filePaths;
});

ipcMain.handle("desktop:vault-ingest-paths", async (_event, payload) => {
  const paths = payload && Array.isArray(payload.paths) ? payload.paths : [];
  return ingestDesktopFilePaths(paths);
});

ipcMain.handle("desktop:vault-read-index", async () => {
  return readVaultIndex();
});

ipcMain.handle("desktop:vault-write-index", async (_event, payload) => {
  const index = payload && Array.isArray(payload.index) ? payload.index : [];
  return writeVaultIndex(index);
});

ipcMain.handle("desktop:vault-read-json", async (_event, payload) => {
  const relativePath = payload?.path;
  if (typeof relativePath !== "string" || !relativePath.trim()) {
    return null;
  }
  return readVaultJson(relativePath, null);
});

ipcMain.handle("desktop:vault-write-json", async (_event, payload) => {
  const relativePath = payload?.path;
  if (typeof relativePath !== "string" || !relativePath.trim()) {
    throw new Error("Missing vault json path");
  }

  await writeVaultJson(relativePath, payload?.data ?? {});
  return { ok: true };
});

ipcMain.handle("desktop:vault-read-text-snippet", async (_event, payload) => {
  const id = typeof payload?.id === "string" ? payload.id.trim() : "";
  const maxChars = Number.isFinite(payload?.maxChars)
    ? Math.min(Math.max(Number(payload.maxChars), 200), MAX_DESKTOP_TEXT_SNIPPET)
    : 12_000;

  if (!id) {
    return "";
  }

  const index = await readVaultIndex();
  const item = index.find((entry) => entry.id === id);
  if (!item) {
    return "";
  }

  if (item.type !== "text" && item.type !== "pdf") {
    return "";
  }

  try {
    const filePath = toVaultFilePathByMeta(item);
    const content = await fsPromises.readFile(filePath, "utf8");
    return content.slice(0, maxChars);
  } catch (_) {
    return "";
  }
});

ipcMain.handle("desktop:vault-delete-item", async (_event, payload) => {
  const id = typeof payload?.id === "string" ? payload.id.trim() : "";
  if (!id) {
    return { ok: false };
  }

  const index = await readVaultIndex();
  const item = index.find((entry) => entry.id === id);
  if (!item) {
    return { ok: false };
  }

  const filtered = index.filter((entry) => entry.id !== id);
  await writeVaultIndex(filtered);

  const targets = [item.filePath, item.summaryPath, item.featuresPath].filter(Boolean);
  for (const relativePath of targets) {
    try {
      const absolute = resolveVaultRelativePath(relativePath);
      await fsPromises.unlink(absolute);
    } catch (_) {
      // ignore missing files
    }
  }

  return { ok: true };
});

ipcMain.handle("desktop:vault-scan-folder", async (_event, payload) => {
  const targetPath = typeof payload?.path === "string" ? payload.path.trim() : "";
  if (!targetPath) {
    throw new Error("scan_folder path is required");
  }

  const recursive = Boolean(payload?.recursive);
  const fileTypes = Array.isArray(payload?.fileTypes) ? payload.fileTypes : [];

  const stat = await fsPromises.stat(targetPath);
  if (!stat.isDirectory()) {
    throw new Error("scan_folder path must be a directory");
  }

  const matchedPaths = await collectFolderFiles(targetPath, recursive, fileTypes);
  return { matchedPaths };
});

ipcMain.handle("desktop:profile-load", async () => {
  await ensureVaultStructure();
  return readJsonFile(getVaultProfilePath(), null);
});

ipcMain.handle("desktop:profile-save", async (_event, payload) => {
  await ensureVaultStructure();
  await writeJsonFile(getVaultProfilePath(), payload && typeof payload === "object" ? payload : {});
  return { ok: true, path: getVaultProfilePath() };
});

ipcMain.handle("desktop:profile-export", async (_event, payload) => {
  await ensureVaultStructure();
  const stamp = new Date().toISOString().replace(/[.:]/g, "-");
  const fileName = `profile_export_${stamp}.json`;
  const exportPath = path.join(getVaultRootPath(), fileName);
  await writeJsonFile(exportPath, payload && typeof payload === "object" ? payload : {});
  return { ok: true, path: exportPath };
});

app.whenReady().then(() => {
  registerIpcHandlers();
  createWindow();

  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow();
    }
  });
});

app.on("window-all-closed", () => {
  if (process.platform !== "darwin") {
    app.quit();
  }
});
