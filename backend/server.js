import express from 'express';
import cors from 'cors';
import http from 'node:http';
import { Server } from 'socket.io';
import cpuRoutes from './routes/cpuRoutes.js';
import { registerSocketHandlers } from './sockets/socketHandler.js';
import { sendError } from './utils/errorResponse.js';
import { sendSuccess } from './utils/successResponse.js';
import { enqueueInput } from './utils/ioQueue.js';

const app = express();
const httpServer = http.createServer(app);
const io = new Server(httpServer, {
  cors: {
    origin: process.env.CLIENT_ORIGIN || '*',
  },
});
const port = Number(process.env.PORT || 3000);

app.use(express.json());
app.use(cors());

app.get('/health', (_request, response) => {
  sendSuccess(response, { status: 'ok' });
});

app.use('/api/cpu', cpuRoutes);
app.use((error, _request, response, _next) => {
  sendError(response, error);
});
registerSocketHandlers(io, enqueueInput);

httpServer.listen(port, () => {
  console.log(`8085 simulator backend listening on port ${port}`);
});

export { app, httpServer, io };