import { useEffect, useRef, useState } from 'react';
import { io } from 'socket.io-client';

const initialRegisters = {
  a: 0, b: 0, c: 0, d: 0, e: 0, h: 0, l: 0, pc: 0, sp: 0xffff,
  sign: false, zero: false, auxiliaryCarry: false, parity: false, carry: false,
  halted: false,
};

export function useSimulatorWebSocket() {
  const [registers, setRegisters] = useState(initialRegisters);
  const [executionState, setExecutionState] = useState('disconnected');
  const [memory, setMemory] = useState([]);
  const [ioState, setIoState] = useState({ input: '', output: '' });
  const socketRef = useRef(null);

  useEffect(() => {
    const socket = io(import.meta.env.VITE_API_ORIGIN || undefined, {
      transports: ['websocket'],
    });
    socketRef.current = socket;
    socket.on('connect', () => setExecutionState('paused'));
    socket.on('disconnect', () => setExecutionState('disconnected'));
    socket.on('registerState', (payload) => {
      if (!payload?.registers) return;
      setRegisters((current) => ({ ...current, ...payload.registers }));
      if (payload.executionState) setExecutionState(payload.executionState);
      if (payload.memory) setMemory(payload.memory);
      if (payload.io) setIoState((current) => ({ ...current, ...payload.io }));
    });
    return () => {
      socket.disconnect();
      socketRef.current = null;
    };
  }, []);

  return { registers, executionState, memory, ioState };
}
