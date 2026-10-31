import Panel from './Panel.jsx';

export default function ConsolePanel({ lines, port, value, onPortChange, onValueChange, onSend }) {
  return (
    <Panel className="console-panel" eyebrow="Runtime output" title="I/O & console">
      <div className="console-output">
        {lines.map((line, index) => <div className={line.startsWith('ERROR') ? 'console-error' : ''} key={`${line}-${index}`}><span className="console-prompt">&gt;</span>{line}</div>)}
      </div>
      <div className="console-input">
        <span>IN</span>
        <label htmlFor="io-port">PORT</label>
        <input id="io-port" inputMode="numeric" onChange={(event) => onPortChange(event.target.value)} placeholder="01" value={port} />
        <label htmlFor="io-value">VALUE</label>
        <input id="io-value" inputMode="numeric" onChange={(event) => onValueChange(event.target.value)} onKeyDown={(event) => event.key === 'Enter' && onSend()} placeholder="FF" value={value} />
        <button onClick={onSend} type="button">Queue input</button>
      </div>
    </Panel>
  );
}
