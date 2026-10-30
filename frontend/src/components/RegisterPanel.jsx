import Panel from './Panel.jsx';

const registerNames = ['a', 'b', 'c', 'd', 'e', 'h', 'l', 'sp', 'pc'];
const flags = [['S', 'sign'], ['Z', 'zero'], ['AC', 'auxiliaryCarry'], ['P', 'parity'], ['CY', 'carry']];
const formatHex = (value, width) => Number(value || 0).toString(16).toUpperCase().padStart(width, '0');

export { formatHex };

export default function RegisterPanel({ registers }) {
  return (
    <Panel eyebrow="CPU state" title="Registers">
      <div className="register-grid">
        {registerNames.map((name) => (
          <div className="register-cell" key={name}>
            <span>{name.toUpperCase()}</span>
            <strong>{formatHex(registers[name], name === 'sp' || name === 'pc' ? 4 : 2)}<small>H</small></strong>
          </div>
        ))}
      </div>
      <div className="flags-heading">Condition flags</div>
      <div className="flag-grid">
        {flags.map(([label, key]) => (
          <div className={`flag ${registers[key] ? 'on' : ''}`} key={key}>
            <strong>{registers[key] ? 1 : 0}</strong>
            <span>{label}</span>
          </div>
        ))}
      </div>
    </Panel>
  );
}
