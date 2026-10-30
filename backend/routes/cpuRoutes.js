import express from 'express';
import * as cpuController from '../controllers/cpuController.js';

const router = express.Router();

router.post('/compile', cpuController.compile);
router.post('/run', cpuController.run);
router.post('/step', cpuController.step);
router.post('/reset', cpuController.reset);
router.post('/pause', cpuController.pause);
router.post('/breakpoint', cpuController.addBreakpoint);
router.delete('/breakpoint', cpuController.removeBreakpoint);
router.get('/memory/:address', cpuController.readMemory);
router.post('/memory', cpuController.writeMemory);
router.post('/register', cpuController.overrideRegister);

export default router;
