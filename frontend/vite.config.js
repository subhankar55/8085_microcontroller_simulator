import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { loadEnv } from 'vite';

const projectRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, projectRoot, '');
  const backendOrigin = env.VITE_API_ORIGIN || 'http://localhost:3000';

  return {
    envDir: projectRoot,
    plugins: [react(), tailwindcss()],
    server: {
      port: Number(env.FRONTEND_PORT || 5173),
      proxy: {
        '/api': backendOrigin,
        '/socket.io': {
          target: backendOrigin,
          ws: true,
        },
      },
    },
  };
});
