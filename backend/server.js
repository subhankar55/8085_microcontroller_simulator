const express = require('express');

const app = express();
const port = Number(process.env.PORT || 3000);

app.use(express.json());

app.get('/health', (_request, response) => {
  response.json({ status: 'ok' });
});

app.listen(port, () => {
  console.log(`8085 simulator backend listening on port ${port}`);
});