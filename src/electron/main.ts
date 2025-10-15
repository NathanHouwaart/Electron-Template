import { app, BrowserWindow, ipcMain } from 'electron'
import path from 'path'
import { isDev } from './utils.js';
import { MyObject } from './bindings.js';

console.log('[MAIN.TS] loaded from', import.meta.url);
console.log('process.versions:', process.versions);
console.log('process.arch:', process.arch);

// Add global error handlers to catch crashes
process.on('uncaughtException', (error) => {
  console.error('[FATAL] Uncaught exception:', error);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('[FATAL] Unhandled rejection at:', promise, 'reason:', reason);
});

app.on('ready', () => {
  console.log('[MAIN.TS] App ready event fired');
  
  try {
    const mainWindow = new BrowserWindow({
      width: 800,
      height: 600
    });

    if (isDev()) {
      mainWindow.loadURL('http://localhost:5123');
    } else {
      mainWindow.loadFile(path.join(app.getAppPath(), "/dist-react/index.html"));
    }

    console.log('App is ready - window created');

    // Test the native addon
    try {
      const port = 'COM3';
      console.log(`[MAIN.TS] Calling MyObject.Connect('${port}')...`);
      const ok = MyObject.Connect(port);
      console.log(`[MAIN.TS] Connect(${port}) =>`, ok);
      
      const data = MyObject.ReadData();
      console.log('[MAIN.TS] ReadData() =>', data);
    } catch (error) {
      console.error('[MAIN.TS] Error calling addon function:', error);
    }

  } catch (error) {
    console.error('[MAIN.TS] Error in ready handler:', error);
  }
});


// [MAIN.TS] loaded from file:///C:/Users/Nathan-HvA/Documents/Electron-Template/dist-electron/main.js
// process.versions: {
//   node: '22.19.0',
//   acorn: '8.15.0',
//   ada: '2.9.2',
//   amaro: '1.1.0',
//   ares: '1.34.5',
//   brotli: '1.0.9',
//   cjs_module_lexer: '2.1.0',
//   cldr: '44.1',
//   icu: '74.2',
//   llhttp: '9.3.0',
//   modules: '139',
//   napi: '10',
//   nbytes: '0.1.1',
//   ncrypto: '0.0.1',
//   nghttp2: '1.64.0',
//   openssl: '0.0.0',
//   simdjson: '3.13.0',
//   simdutf: '7.3.3',
//   sqlite: '3.50.4',
//   tz: '2025b',
//   undici: '6.21.2',
//   unicode: '15.1',
//   uv: '1.51.0',
//   uvwasi: '0.0.21',
//   v8: '14.0.365.4-electron.0',
//   zlib: '1.3.1',
//   zstd: '1.5.8',
//   electron: '38.2.2',
//   chrome: '140.0.7339.133'
// }
// process.arch: x64