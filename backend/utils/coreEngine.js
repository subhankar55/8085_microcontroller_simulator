import { createRequire } from 'node:module';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const require = createRequire(import.meta.url);
const backendDirectory = path.dirname(fileURLToPath(import.meta.url));
const defaultAddonPath = path.resolve(
  backendDirectory,
  '../../Core_engine/build/Release/8085_simulator.node',
);
const configuredAddonPath = process.env.CORE_ENGINE_PATH;
const addonPath = configuredAddonPath
  ? (path.isAbsolute(configuredAddonPath)
    ? configuredAddonPath
    : path.resolve(process.cwd(), configuredAddonPath))
  : defaultAddonPath;

let engine;

function getCoreEngine() {
  if (!engine) {
    try {
      const addon = require(addonPath);
      if (!addon?.SimulatorWrapper) {
        throw new Error('SimulatorWrapper export was not found');
      }
      engine = new addon.SimulatorWrapper();
    } catch (error) {
      const engineError = new Error(
        `Unable to load the 8085 core engine at "${addonPath}": ${error.message}`,
      );
      engineError.statusCode = 503;
      throw engineError;
    }
  }

  return engine;
}

export { getCoreEngine };
