import { createRequire } from 'module';
import fs from 'fs';

const require = createRequire(import.meta.url);

const addonPath = 'C:\\Users\\Nathan-HvA\\Documents\\Electron-Template\\build\\deviceaddon.node';

console.log('[bindings] Checking addon path:', addonPath);
console.log('[bindings] Addon exists:', fs.existsSync(addonPath));

if (!fs.existsSync(addonPath)) {
  throw new Error(`Native addon not found at: ${addonPath}`);
}

console.log('[bindings] Attempting to require addon...');
let addon: any;
try {
  addon = require(addonPath);
  console.log('[bindings] Addon loaded successfully, keys:', Object.keys(addon));
} catch (error) {
  console.error('[bindings] Failed to load addon:', error);
  throw error;
}

export const MyObject = {
  Connect(port: string) { 
    console.log('[MyObject] Connecting to port:', port);
    return addon.connect(port);
  },
  ReadData() { 
    console.log('[MyObject] Reading data from device');
    return addon.readData();
   }
};

export default MyObject;