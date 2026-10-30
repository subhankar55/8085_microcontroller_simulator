import Panel from './Panel.jsx';
import { formatHex } from './RegisterPanel.jsx';

export default function MachineCodePanel({ machineCode }) {
  return (
    <Panel eyebrow="Assembler output" title="Machine code" actions={<span className="panel-meta">{machineCode.length} BYTES</span>}>
      <div className="machine-code-grid">
        <div className="machine-code-header"><span>ADDRESS</span><span>OPCODE</span><span>DECIMAL</span></div>
        {machineCode.length ? machineCode.map((byte) => (
          <div className="machine-code-row" key={byte.address}><span>{formatHex(byte.address, 4)}H</span><strong>{formatHex(byte.value, 2)}H</strong><span>{byte.value}</span></div>
        )) : <div className="empty-state">Compile your assembly to inspect generated opcodes.</div>}
      </div>
    </Panel>
  );
}
