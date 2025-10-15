import { app, BrowserWindow, ipcMain } from 'electron'
import path from 'path'
import { isDev } from './utils.js';
// import { MyObject } from './bindings_own.js';
import { MyObject } from './bindings.js';

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

  const mainWindow = new BrowserWindow({
    width: 800,
    height: 600
  });

  if (isDev()) {
    mainWindow.loadURL('http://localhost:5123');
  } else {
    mainWindow.loadFile(path.join(app.getAppPath(), "/dist-react/index.html"));
  }

  const obj = new MyObject('Example');
  obj.greet('Nathan');
  console.log(obj.add(5, 3));
});