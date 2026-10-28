function sendError(response, error) {
  const statusCode = error.statusCode || 500;
  const message = statusCode >= 500 ? 'Internal server error' : error.message;

  return response.status(statusCode).json({
    success: false,
    error: message,
  });
}

export { sendError };
