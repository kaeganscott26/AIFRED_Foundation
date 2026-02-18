const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("desktopApp", {
  platform: process.platform,
  httpRequest(request) {
    return ipcRenderer.invoke("desktop:http-request", request);
  },
  getOpenAIKey() {
    return ipcRenderer.invoke("desktop:get-openai-key");
  },
  loadMemoryState() {
    return ipcRenderer.invoke("desktop:memory-load");
  },
  saveMemoryState(payload) {
    return ipcRenderer.invoke("desktop:memory-save", payload);
  },
  pickFiles(options) {
    return ipcRenderer.invoke("desktop:vault-pick-files", options);
  },
  ingestVaultPaths(payload) {
    return ipcRenderer.invoke("desktop:vault-ingest-paths", payload);
  },
  loadVaultIndex() {
    return ipcRenderer.invoke("desktop:vault-read-index");
  },
  saveVaultIndex(payload) {
    return ipcRenderer.invoke("desktop:vault-write-index", payload);
  },
  readVaultJson(payload) {
    return ipcRenderer.invoke("desktop:vault-read-json", payload);
  },
  writeVaultJson(payload) {
    return ipcRenderer.invoke("desktop:vault-write-json", payload);
  },
  readVaultTextSnippet(payload) {
    return ipcRenderer.invoke("desktop:vault-read-text-snippet", payload);
  },
  deleteVaultItem(payload) {
    return ipcRenderer.invoke("desktop:vault-delete-item", payload);
  },
  scanFolder(payload) {
    return ipcRenderer.invoke("desktop:vault-scan-folder", payload);
  },
  loadProfile() {
    return ipcRenderer.invoke("desktop:profile-load");
  },
  saveProfile(payload) {
    return ipcRenderer.invoke("desktop:profile-save", payload);
  },
  exportProfile(payload) {
    return ipcRenderer.invoke("desktop:profile-export", payload);
  }
});
