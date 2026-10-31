import Panel from './Panel.jsx';
import { formatHex } from './RegisterPanel.jsx';

export default function MemoryPanel({ memory, address, onAddressChange, onRead, error }) {
  return (
    <Panel className="memory-panel" eyebrow="Address space" title="Memory view" actions={<span className="panel-meta">HEX DUMP</span>}>
      <div className="memory-controls">
        <label htmlFor="memory-address">Address</label>
        <input id="memory-address" inputMode="text" onChange={(event) => onAddressChange(event.target.value)} onKeyDown={(event) => event.key === 'Enter' && onRead()} placeholder="2000H" value={address} />
        <button onClick={onRead} type="button">Read</button>
      </div>
      {error && <div className="memory-error">{error}</div>}
      <div className="memory-table">
        <div className="memory-row memory-header"><span>ADDRESS</span><span>VALUE</span><span>ASCII</span></div>
        {memory.length ? memory.map((byte) => (
          <div className="memory-row" key={byte.address}>
            <span>{formatHex(byte.address, 4)}H</span><span className="cyan-value">{formatHex(byte.value, 2)}H</span><span>{byte.value >= 32 && byte.value < 127 ? String.fromCharCode(byte.value) : '·'}</span>
          </div>
        )) : <div className="empty-state">Memory streaming will appear here when exposed by the backend.</div>}
      </div>
    </Panel>
  );
}
