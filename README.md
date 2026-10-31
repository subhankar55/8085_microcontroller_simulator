# 8085_microcontroller_simulator
This is an educational project to build 8085 microcontroller simulator

## Interactive I/O

Start Redis before starting the backend:

```powershell
docker compose up -d
npm start
```

Copy `.env.example` to `.env` if the local environment file does not exist.
The root `.env` file is loaded by the backend and Vite configuration; it is
ignored by Git so local Redis credentials and paths are not committed.

The Compose file creates the `8085-simulator-network` Docker network and starts
Redis with a persistent volume. Check Redis with `docker compose ps` or
`docker compose exec redis redis-cli ping`; stop it with
`docker compose down`.

In the **I/O & console** panel, enter a hexadecimal port and byte value
(for example `01` and `FF`) and select **Send**. The value is delivered
through the BullMQ `simulator-io-input` queue to the native CPU input buffer.
An `IN` instruction consumes the next value for its port. Values produced by
an `OUT` instruction are emitted through Socket.IO and displayed in the
console.

The Redis connection defaults to `redis://127.0.0.1:6379`; set `REDIS_URL` to
override it.
