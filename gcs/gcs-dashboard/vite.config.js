
import { defineConfig } from "vite";

export default defineConfig({
  clearScreen: false,

  server: {
    port: 3000,
    strictPort: true,
  },

  envPrefix: ["VITE_", "TAURI_ENV_*"],

  build: {
    target:
      process.env.TAURI_ENV_PLATFORM === "windows"
        ? "chrome105"
        : "safari13",
  },
});
