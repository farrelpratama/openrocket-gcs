import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [react()],
  // Konfigurasi port agar sesuai dengan Tauri v2
  server: {
    port: 3000,
    strictPort: true,
    // INI BAGIAN PENTING UNTUK MENCEGAH EBUSY:
    watch: {
      ignored: ["**/src-tauri/**"],
    },
  },
});