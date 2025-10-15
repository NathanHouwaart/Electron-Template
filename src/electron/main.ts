import { app, BrowserWindow, ipcMain, IpcMainInvokeEvent } from 'electron'
import path from 'path'
import { isDev } from './utils.js';
import { MyObject, PN532_Wrapper } from './bindings.js';
import { getPreloadPath } from './pathResolver.js';

import { SerialPort } from 'serialport';



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
    height: 600,
    webPreferences: {
      preload: getPreloadPath()
    }
  });

  console.log("HERE")

  if (isDev()) {
    mainWindow.loadURL('http://localhost:5123');
  } else {
    mainWindow.loadFile(path.join(app.getAppPath(), "/dist-react/index.html"));
  }

  const obj = new MyObject('Example');
  obj.greet('Nathan');
  console.log(obj.add(5, 3));
});

ipcMain.handle('connect', async (event: IpcMainInvokeEvent, port: string) => {
  console.log('ipcMain: connect called with args:', port);

  const obj = new PN532_Wrapper();
  return obj.connect(port);
});

ipcMain.handle('disconnect', async () => {
  console.log('ipcMain: disconnect called');

  const obj = new PN532_Wrapper();
  return obj.disconnect();
});

ipcMain.handle('getFirmwareVersion', () => {
  console.log('ipcMain: getFirmwareVersion called');
  const obj = new PN532_Wrapper();
  return obj.getFirmwareVersion();
});

ipcMain.handle('listComPorts', async () => {
  const ports = await SerialPort.list();
  console.log(ports);
  return ports.map(p => ({ path: p.path, manufacturer: p.manufacturer }));
});