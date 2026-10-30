import Panel from './Panel.jsx';

export default function ConsolePanel({ lines, input, onInputChange, onSend }) {
  return (
    <Panel eyebrow="Runtime output" title="I/O & console">
      <div className="console-output">
        {lines.map((line, index) => <div className={line.startsWith('ERROR') ? 'console-error' : ''} key={`${line}-${index}`}><span className="console-prompt">&gt;</span>{line}</div>)}
      </div>
      <div className="console-input">
        <span>IN</span>
        <input onChange={(event) => onInputChange(event.target.value)} onKeyDown={(event) => event.key === 'Enter' && onSend()} placeholder="Enter CPU input..." value={input} />
        <button onClick={onSend} type="button">Send</button>
      </div>
    </Panel>
  );
}
