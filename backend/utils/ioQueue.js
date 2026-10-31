import { Queue, QueueEvents, Worker } from 'bullmq';
import { getCoreEngine } from './coreEngine.js';

const redisUrl = new URL(process.env.REDIS_URL || 'redis://127.0.0.1:6379');
const connection = {
  host: redisUrl.hostname,
  port: Number(redisUrl.port || 6379),
  username: decodeURIComponent(redisUrl.username),
  password: decodeURIComponent(redisUrl.password),
  db: redisUrl.pathname.length > 1 ? Number(redisUrl.pathname.slice(1)) : 0,
  maxRetriesPerRequest: null,
};

const inputQueue = new Queue('simulator-io-input', { connection });
const inputQueueEvents = new QueueEvents('simulator-io-input', { connection });

function validateByte(value, name) {
  const parsed = Number(value);
  if (!Number.isInteger(parsed) || parsed < 0 || parsed > 0xff) {
    const error = new Error(`${name} must be an integer between 0 and 255`);
    error.statusCode = 400;
    throw error;
  }
  return parsed;
}

async function enqueueInput({ port, value, source = 'socket' }) {
  const normalizedPort = validateByte(port, 'port');
  const normalizedValue = validateByte(value, 'value');
  const job = await inputQueue.add(
    'input-byte',
    { port: normalizedPort, value: normalizedValue, source },
    {
      attempts: 3,
      backoff: { type: 'exponential', delay: 100 },
      removeOnComplete: true,
      removeOnFail: 100,
    },
  );
  await job.waitUntilFinished(inputQueueEvents);
  return { jobId: job.id, port: normalizedPort, value: normalizedValue };
}

const inputWorker = new Worker(
  'simulator-io-input',
  async (job) => {
    const engine = getCoreEngine();
    engine.EnqueueInput(job.data.port, job.data.value);
  },
  { connection },
);

inputWorker.on('error', (error) => {
  console.error(`I/O input worker error: ${error.message}`);
});

export { enqueueInput, inputQueue, inputQueueEvents, inputWorker, validateByte };
