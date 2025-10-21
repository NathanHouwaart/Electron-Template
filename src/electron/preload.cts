const electron = require('electron');

electron.contextBridge.exposeInMainWorld("electron", {
    getStaticData: () => ipcInvoke("getStaticData"),
    connect: (port: string) => ipcInvoke("connect", port),
    disconnect: () => ipcInvoke("disconnect"),
    getFirmwareVersion: () => ipcInvoke("getFirmwareVersion"),
    listComPorts: () => ipcInvoke("listComPorts"),
    getVersion: () => ipcInvoke("getVersion"),
} satisfies Window["electron"]);


function ipcInvoke<Key extends keyof EventPayloadMapping>(
    key: Key,
    ...args: EventInvokeArgs[Key]
): Promise<EventPayloadMapping[Key]> {
    return electron.ipcRenderer.invoke(key, ...args);
}

function ipcOn<Key extends keyof EventPayloadMapping>(
    key: Key,
    callback: (payload: EventPayloadMapping[Key]) => void
) {
    const cb = (_ : Electron.IpcRendererEvent, payload: any) => callback(payload);
    electron.ipcRenderer.on(key, cb)
    return () => electron.ipcRenderer.off(key, cb); // Return unsubscribe function
}
