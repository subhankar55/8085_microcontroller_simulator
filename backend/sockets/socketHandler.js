let io;

function registerSocketHandlers(socketIo, enqueueInput) {
  io = socketIo;

  io.on('connection', (socket) => {
    socket.emit('registerState', {
      message: 'Connected to the 8085 simulator',
    });

    socket.on('ioInput', async (payload, acknowledge) => {
      try {
        const result = await enqueueInput(payload || {});
        acknowledge?.({ success: true, data: result });
        socket.emit('ioEvent', { direction: 'in', ...result });
      } catch (error) {
        acknowledge?.({ success: false, error: error.message });
      }
    });

    socket.on('requestRegisterState', () => {
      socket.emit('registerState', {
        message: 'Register state is managed by the CPU controller',
      });
    });
  });
}

function broadcastRegisterState(state) {
  if (io) {
    io.emit('registerState', state);
  }
}

export {
  registerSocketHandlers,
  broadcastRegisterState,
};
