import Editor from '@monaco-editor/react';
import Panel from './Panel.jsx';

export default function EditorPanel({
  source,
  onChange,
  onMount,
  onAction,
  busy,
  executionState,
  breakpoints,
}) {
  return (
    <Panel
      className="editor-panel"
      eyebrow="Source workspace"
      title="8085 Assembly"
      actions={<span className={`status-pill ${executionState}`}>{executionState}</span>}
    >
      <div className="editor-toolbar">
        <span className="file-tab"><span className="file-dot" />main.asm</span>
        <span className="editor-hint">Click the gutter to toggle breakpoints</span>
        <span className="breakpoint-count">{breakpoints.length} breakpoint{breakpoints.length === 1 ? '' : 's'}</span>
      </div>
      <div className="editor-frame">
        <Editor
          beforeMount={(monaco) => {
            monaco.languages.register({ id: '8085asm' });
            monaco.languages.setMonarchTokensProvider('8085asm', {
              tokenizer: {
                root: [
                  [/;.*$/, 'comment'],
                  [/\b(MOV|MVI|LXI|ADD|ADI|SUB|SUI|INR|DCR|JMP|JZ|JNZ|CALL|RET|HLT|STA|LDA|IN|OUT)\b/i, 'keyword'],
                  [/\b[0-9A-F]+H\b/i, 'number'],
                  [/[A-Z][A-Z0-9_]*/, 'type.identifier'],
                ],
              },
            });
          }}
          defaultLanguage="8085asm"
          height="100%"
          loading={<div className="editor-loading">Loading Monaco editor…</div>}
          onChange={(value) => onChange(value || '')}
          onMount={onMount}
          options={{
            automaticLayout: true,
            glyphMargin: true,
            lineNumbersMinChars: 3,
            minimap: { enabled: false },
            padding: { top: 16, bottom: 16 },
            renderLineHighlight: 'all',
            scrollBeyondLastLine: false,
            fontSize: 15,
            tabSize: 2,
          }}
          theme="vs-dark"
          value={source}
        />
      </div>
      <div className="editor-footer">
        <span>8085 ASM</span>
        <span>{source.split('\n').length} lines</span>
        <span>UTF-8</span>
      </div>
      <div className="editor-actions">
        <button className="control-button primary" disabled={busy} onClick={() => onAction('compile')} type="button">Compile / Assemble</button>
        <button className="control-button success" disabled={busy} onClick={() => onAction('run')} type="button">Run program</button>
        <button className="control-button default" disabled={busy} onClick={() => onAction('step')} type="button">Step</button>
        <button className="control-button danger" disabled={busy} onClick={() => onAction('reset')} type="button">Reset</button>
      </div>
    </Panel>
  );
}
