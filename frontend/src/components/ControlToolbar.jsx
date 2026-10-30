const controls = [
  ['Compile / Assemble', 'primary', 'compile'],
  ['Run', 'success', 'run'],
  ['Pause', 'warning', 'pause'],
  ['Step Into', 'default', 'step'],
  ['Stop / Reset', 'danger', 'reset'],
];

export default function ControlToolbar({ busy, feedback, onAction }) {
  return (
    <div className="control-toolbar">
      {controls.map(([label, tone, action]) => (
        <button
          className={`control-button ${tone}`}
          disabled={busy}
          key={`${label}-${action}`}
          onClick={() => onAction(action)}
          type="button"
        >
          <span className="button-icon">{action === 'compile' ? '▶' : action === 'run' ? '●' : action === 'pause' ? 'Ⅱ' : action === 'reset' ? '■' : '▸'}</span>
          {label}
        </button>
      ))}
      <span className={`toolbar-feedback ${feedback?.error ? 'error' : ''}`}>
        {busy ? 'Working…' : feedback?.message || 'Ready'}
      </span>
    </div>
  );
}
