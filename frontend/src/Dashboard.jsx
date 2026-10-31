import { useEffect, useRef, useState } from 'react';
import {
  compileSource,
  addBreakpoint,
  removeBreakpoint,
  readMemory,
  pauseCpu,
  runCpu,
  resetCpu,
  stepCpu,
} from './services/simulatorApi.js';
import { useSimulatorWebSocket } from './hooks/useSimulatorWebSocket.js';
import ControlToolbar from './components/ControlToolbar.jsx';
import ConsolePanel from './components/ConsolePanel.jsx';
import EditorPanel from './components/EditorPanel.jsx';
import MachineCodePanel from './components/MachineCodePanel.jsx';
import MemoryPanel from './components/MemoryPanel.jsx';
import RegisterPanel from './components/RegisterPanel.jsx';

const initialSource = `; 8085 program
MVI A, 05H
MVI B, 03H
ADD B
HLT`;

function instructionLength(instruction) {
  const mnemonic = instruction.trim().split(/\s+/)[0]?.toUpperCase();
  if ([
    'LXI', 'LDA', 'STA', 'LHLD', 'SHLD', 'JMP', 'CALL',
    'JNZ', 'JZ', 'JNC', 'JC', 'JPO', 'JPE', 'JP', 'JM',
    'CNZ', 'CZ', 'CNC', 'CC', 'CPO', 'CPE', 'CP', 'CM',
  ].includes(mnemonic)) return 3;
  if (['MVI', 'ADI', 'ACI', 'SUI', 'SBI', 'ANI', 'XRI', 'ORI', 'CPI', 'IN', 'OUT'].includes(mnemonic)) return 2;
  return mnemonic ? 1 : 0;
}

function buildLineAddressMap(source, machineCode, loadAddress) {
  const map = {};
  let offset = 0;
  source.split('\n').forEach((rawLine, index) => {
    let line = rawLine.split(';')[0].trim();
    line = line.replace(/^[A-Za-z_][A-Za-z0-9_]*:\s*/, '').trim();
    if (!line) return;
    map[index + 1] = loadAddress + offset;
    offset += instructionLength(line);
  });
  return map;
}

export default function Dashboard() {
  const { registers: liveRegisters, executionState, ioState, sendIoInput } = useSimulatorWebSocket();
  const [source, setSource] = useState(initialSource);
  const [registers, setRegisters] = useState(liveRegisters);
  const [machineCode, setMachineCode] = useState([]);
  const [lineAddresses, setLineAddresses] = useState({});
  const [memory, setMemory] = useState([]);
  const [memoryAddress, setMemoryAddress] = useState('2000H');
  const [memoryError, setMemoryError] = useState('');
  const [breakpoints, setBreakpoints] = useState([]);
  const [consoleLines, setConsoleLines] = useState(['Simulator ready.', 'Compile a program to load it into memory.']);
  const [ioPort, setIoPort] = useState('01');
  const [ioValue, setIoValue] = useState('');
  const [busy, setBusy] = useState(false);
  const [feedback, setFeedback] = useState({ message: 'Ready' });
  const editorRef = useRef(null);
  const decorationsRef = useRef([]);
  const lineAddressesRef = useRef({});
  const breakpointsRef = useRef([]);

  useEffect(() => setRegisters(liveRegisters), [liveRegisters]);
  useEffect(() => {
    if (ioState.direction === 'out' && Number.isInteger(ioState.port) && Number.isInteger(ioState.value)) {
      log(`Output: port ${ioState.port.toString(16).toUpperCase().padStart(2, '0')}H = ${ioState.value.toString(16).toUpperCase().padStart(2, '0')}H`);
    }
  }, [ioState]);
  useEffect(() => {
    lineAddressesRef.current = lineAddresses;
  }, [lineAddresses]);
  useEffect(() => {
    breakpointsRef.current = breakpoints;
  }, [breakpoints]);

  const log = (message) => setConsoleLines((current) => [...current.slice(-99), message]);
  const perform = async (action, message) => {
    setBusy(true);
    try {
      const result = await action();
      if (result?.registers) setRegisters(result.registers);
      log(message);
      setFeedback({ message });
    } catch (error) {
      log(`ERROR: ${error.message}`);
      setFeedback({ error: true, message: error.message });
    } finally {
      setBusy(false);
    }
  };

  const compile = async () => {
    setBusy(true);
    try {
      const result = await compileSource(source);
      setMachineCode((result.machineCode || []).map((value, index) => ({ address: (result.address || 0) + index, value })));
      const addresses = buildLineAddressMap(source, result.machineCode || [], result.address || 0);
      lineAddressesRef.current = addresses;
      setLineAddresses(addresses);
      breakpointsRef.current = [];
      setBreakpoints([]);
      if (result.registers) setRegisters(result.registers);
      const message = `${result.message || 'Assembly compiled'} (${result.byteLength || 0} bytes)`;
      log(message);
      setFeedback({ message });
    } catch (error) {
      log(`ERROR: ${error.message}`);
      setFeedback({ error: true, message: error.message });
    } finally {
      setBusy(false);
    }
  };

  const toggleBreakpoint = async (line) => {
    const address = lineAddressesRef.current[line];
    if (address === undefined) {
      log(`ERROR: Line ${line} has no executable instruction. Click an instruction line after compiling.`);
      return;
    }
    const exists = breakpointsRef.current.some((breakpoint) => breakpoint.line === line);
    try {
      if (exists) await removeBreakpoint(address);
      else await addBreakpoint(address);
      const nextBreakpoints = breakpointsRef.current.filter((breakpoint) => breakpoint.line !== line);
      if (!exists) nextBreakpoints.push({ line, address });
      nextBreakpoints.sort((a, b) => a.line - b.line);
      breakpointsRef.current = nextBreakpoints;
      setBreakpoints(nextBreakpoints);
      log(`Breakpoint ${exists ? 'removed from' : 'set on'} line ${line} (${address.toString(16).toUpperCase().padStart(4, '0')}H).`);
    } catch (error) {
      log(`ERROR: ${error.message}`);
    }
  };

  const handleAction = (action) => {
    if (action === 'compile') return compile();
    if (action === 'run') return perform(runCpu, 'Execution started.');
    if (action === 'pause') return perform(pauseCpu, 'Execution paused.');
    if (action === 'step') return perform(stepCpu, 'Single instruction executed.');
    return perform(resetCpu, 'CPU reset to initial state.');
  };

  const editorDidMount = (editor, monaco) => {
    editorRef.current = editor;
    editor.onMouseDown((event) => {
      if (event.target.type !== monaco.editor.MouseTargetType.GUTTER_GLYPH_MARGIN) return;
      const line = event.target.position.lineNumber;
      void toggleBreakpoint(line);
    });
  };

  useEffect(() => {
    if (!editorRef.current) return;
    decorationsRef.current = editorRef.current.deltaDecorations(decorationsRef.current, breakpoints.map(({ line }) => ({
      range: { startLineNumber: line, startColumn: 1, endLineNumber: line, endColumn: 1 },
      options: { glyphMarginClassName: 'breakpoint-glyph', isWholeLine: true, className: 'breakpoint-line' },
    })));
  }, [breakpoints]);

  const sendInput = () => {
    const port = Number.parseInt(ioPort, 16);
    const value = Number.parseInt(ioValue, 16);
    if (!Number.isInteger(port) || port < 0 || port > 0xff || !Number.isInteger(value) || value < 0 || value > 0xff) {
      log('ERROR: Port and value must be hexadecimal bytes from 00H to FFH.');
      return;
    }
    sendIoInput(port, value, (result) => {
      if (!result?.success) {
        log(`ERROR: ${result?.error || 'Unable to queue input.'}`);
        return;
      }
      log(`Input queued: ${value.toString(16).toUpperCase().padStart(2, '0')}H on port ${port.toString(16).toUpperCase().padStart(2, '0')}H.`);
      setIoValue('');
    });
  };

  const readMemoryByte = async () => {
    const normalized = memoryAddress.trim().toUpperCase().replace(/H$/, '');
    const parsed = Number.parseInt(normalized, 16);
    if (!Number.isInteger(parsed) || parsed < 0 || parsed > 0xffff) {
      setMemoryError('Enter an address from 0000H to FFFFH.');
      return;
    }
    try {
      const result = await readMemory(parsed);
      setMemory((current) => [...current.filter((byte) => byte.address !== result.address), result].sort((a, b) => a.address - b.address));
      setMemoryError('');
      log(`Read ${result.value.toString(16).toUpperCase().padStart(2, '0')}H from ${normalized.padStart(4, '0')}H.`);
    } catch (error) {
      setMemoryError(error.message);
      log(`ERROR: ${error.message}`);
    }
  };

  return (
    <main className="dashboard-shell">
      <header className="dashboard-header">
        <div className="brand"><div className="brand-mark">85</div><div><div className="brand-kicker">MICROPROCESSOR LAB</div><h1>8085 Simulator</h1></div></div>
        <div className="header-right"><span className={`connection-dot ${executionState === 'disconnected' ? 'offline' : ''}`} /> <span>{executionState}</span><span className="header-divider" /><span className="header-caption">REAL-TIME CPU MONITOR</span></div>
      </header>
      <ControlToolbar busy={busy} feedback={feedback} onAction={handleAction} />
      <div className="dashboard-grid">
        <EditorPanel busy={busy} breakpoints={breakpoints} executionState={executionState} onAction={handleAction} onChange={setSource} onMount={editorDidMount} source={source} />
        <RegisterPanel registers={registers} />
        <MachineCodePanel machineCode={machineCode} />
        <MemoryPanel address={memoryAddress} error={memoryError} memory={memory} onAddressChange={setMemoryAddress} onRead={readMemoryByte} />
        <ConsolePanel lines={consoleLines} onPortChange={setIoPort} onSend={sendInput} onValueChange={setIoValue} port={ioPort} value={ioValue} />
      </div>
      <footer className="dashboard-footer"><span>8085 MICROCONTROLLER SIMULATOR</span><span>Backend: localhost:3000</span><span>Socket.IO stream enabled</span></footer>
    </main>
  );
}
