function sendSuccess(response, data) {
  return response.json({ success: true, data });
}

export { sendSuccess };
