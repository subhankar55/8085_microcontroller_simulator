const jsonRequest = async (path, options = {}) => {
  let response;
  try {
    response = await fetch(path, {
      headers: { 'Content-Type': 'application/json' },
      ...options,
    });
  } catch {
    throw new Error('Backend is unreachable. Start it with "npm start" on port 3000.');
  }
  const body = await response.json().catch(() => ({}));
  if (!response.ok || body.success === false) {
    const message = typeof body.error === 'string' ? body.error : body.error?.message || body.message;
    throw new Error(message || `Request failed (${response.status})`);
  }
  return body.data ?? body;
};

export const compileSource = (source, address = 0) =>
  jsonRequest('/api/cpu/compile', {
    method: 'POST',
    body: JSON.stringify({ source, address }),
  });

export const runCpu = () => jsonRequest('/api/cpu/run', { method: 'POST' });
export const pauseCpu = () => jsonRequest('/api/cpu/pause', { method: 'POST' });
export const stepCpu = () => jsonRequest('/api/cpu/step', { method: 'POST' });
export const resetCpu = () => jsonRequest('/api/cpu/reset', { method: 'POST' });
export const readMemory = (address) => jsonRequest(`/api/cpu/memory/${address}`, { method: 'GET' });

export const addBreakpoint = (address) =>
  jsonRequest('/api/cpu/breakpoint', {
    method: 'POST',
    body: JSON.stringify({ address }),
  });
export const removeBreakpoint = (address) =>
  jsonRequest('/api/cpu/breakpoint', {
    method: 'DELETE',
    body: JSON.stringify({ address }),
  });
