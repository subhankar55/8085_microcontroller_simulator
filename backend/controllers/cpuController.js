import { getCoreEngine } from '../utils/coreEngine.js';
import { asyncHandler } from '../utils/asyncHandler.js';
import { sendError } from '../utils/errorResponse.js';
import { sendSuccess } from '../utils/successResponse.js';
import { broadcastRegisterState } from '../sockets/socketHandler.js';

function parseNumber(value, fieldName, maximum = 0xffff) {
  const parsed = typeof value === 'number' ? value : Number(value);
  if (!Number.isInteger(parsed) || parsed < 0 || parsed > maximum) {
    const error = new Error(`${fieldName} must be an integer between 0 and ${maximum}`);
    error.statusCode = 400;
    throw error;
  }
  return parsed;
}

function getRegisterState(engine) {
  return engine.GetRegisters();
}

// Replace these state mutations with calls to the native C++ addon or executable
// once its command interface is finalized. child_process.spawn is an alternative
// if the engine remains a standalone executable.
const compile = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const source = request.body?.source ?? request.body?.code;
    if (typeof source !== 'string' || source.trim() === '') {
      const error = new Error('source must be a non-empty assembly string');
      error.statusCode = 400;
      throw error;
    }
    const loadAddress = parseNumber(request.body?.address ?? 0, 'address');
    const machineCode = engine.Compile(source);
    engine.InjectMemory(loadAddress, machineCode);
    return sendSuccess(response, {
      message: 'Assembly compiled and loaded into core engine memory',
      address: loadAddress,
      byteLength: machineCode.length,
      machineCode: Array.from(machineCode),
      registers: getRegisterState(engine),
    });
  } catch (error) {
    return sendError(response, error);
  }
});

const run = asyncHandler(async (_request, response) => {
  try {
    const engine = getCoreEngine();
    engine.Run((state) => {
      broadcastRegisterState({
        registers: state,
        executionState: state.halted ? 'halted' : 'running',
      });
    });
    return sendSuccess(response, {
      executionState: 'running',
      registers: getRegisterState(engine),
    });
  } catch (error) {
    return sendError(response, error);
  }
});

const step = asyncHandler(async (_request, response) => {
  try {
    const engine = getCoreEngine();
    const registers = engine.Step();
    broadcastRegisterState({ registers, executionState: registers.halted ? 'halted' : 'paused' });
    return sendSuccess(response, { registers, executionState: registers.halted ? 'halted' : 'paused' });
  } catch (error) {
    return sendError(response, error);
  }
});

const reset = asyncHandler(async (_request, response) => {
  try {
    const engine = getCoreEngine();
    engine.Reset();
    const registers = getRegisterState(engine);
    broadcastRegisterState({ registers, executionState: 'paused' });
    return sendSuccess(response, { registers, executionState: 'paused' });
  } catch (error) {
    return sendError(response, error);
  }
});

const pause = asyncHandler(async (_request, response) => {
  try {
    const engine = getCoreEngine();
    engine.Pause();
    const registers = getRegisterState(engine);
    broadcastRegisterState({ registers, executionState: 'paused' });
    return sendSuccess(response, { executionState: 'paused', registers });
  } catch (error) {
    return sendError(response, error);
  }
});

const addBreakpoint = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const address = parseNumber(request.body?.address, 'address');
    engine.AddBreakpoint(address);
    return sendSuccess(response, { address });
  } catch (error) {
    return sendError(response, error);
  }
});

const removeBreakpoint = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const address = parseNumber(request.body?.address, 'address');
    engine.RemoveBreakpoint(address);
    return sendSuccess(response, { address });
  } catch (error) {
    return sendError(response, error);
  }
});

const readMemory = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const address = parseNumber(request.params.address, 'address');
    const bytes = engine.InspectMemory(address, 1);
    return sendSuccess(response, { address, value: bytes[0] });
  } catch (error) {
    return sendError(response, error);
  }
});

const writeMemory = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const address = parseNumber(request.body?.address, 'address');
    const value = parseNumber(request.body?.value, 'value', 0xff);
    engine.InjectMemory(address, Buffer.from([value]));
    return sendSuccess(response, { address, value });
  } catch (error) {
    return sendError(response, error);
  }
});

const overrideRegister = asyncHandler(async (request, response) => {
  try {
    const engine = getCoreEngine();
    const name = String(request.body?.name || '').toUpperCase();
    const validRegisters = new Set(['A', 'B', 'C', 'D', 'E', 'H', 'L', 'PC', 'SP']);
    if (!validRegisters.has(name)) {
      const error = new Error(`Unknown register: ${name}`);
      error.statusCode = 400;
      throw error;
    }
    const value = parseNumber(request.body?.value, 'value', name === 'PC' || name === 'SP' ? 0xffff : 0xff);
    engine.SetRegister(name, value);
    const registers = getRegisterState(engine);
    broadcastRegisterState({ registers, executionState: 'paused' });
    return sendSuccess(response, { name, value, registers });
  } catch (error) {
    return sendError(response, error);
  }
});

export {
  compile,
  run,
  step,
  reset,
  pause,
  addBreakpoint,
  removeBreakpoint,
  readMemory,
  writeMemory,
  overrideRegister,
};
