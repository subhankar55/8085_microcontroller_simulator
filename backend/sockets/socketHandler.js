let io;

function registerSocketHandlers(socketIo) {
  io = socketIo;

  io.on('connection', (socket) => {
    socket.emit('registerState', {
      message: 'Connected to the 8085 simulator',
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
