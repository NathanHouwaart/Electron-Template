const electron = require('electron');

electron.contextBridge.exposeInMainWorld("electron", {
    getStaticData: () => ipcInvoke("getStaticData"),
    connect: (port: string) => ipcInvoke("connect", port),
    disconnect: () => ipcInvoke("disconnect"),
    getFirmwareVersion: () => ipcInvoke("getFirmwareVersion"),
    listComPorts: () => ipcInvoke("listComPorts"),
    getVersion: () => ipcInvoke("getVersion"),
    runSelfTests: (
        progressCallback: (result: SelfTestUpdate) => void,
        completeCallback: (error: Error | null, result: boolean) => void
    ) => {
        // Listen for progress updates
        const progressHandler = (_event: any, result: SelfTestUpdate) => {
            progressCallback(result);
        };
        electron.ipcRenderer.on('self-test-progress', progressHandler);

        // Listen for completion (once)
        electron.ipcRenderer.once('self-test-complete', (_event: any, error: Error | null, result: boolean) => {
            // Clean up progress listener
            electron.ipcRenderer.removeListener('self-test-progress', progressHandler);
            completeCallback(error, result);
        });

        // Trigger the self-test
        electron.ipcRenderer.send('run-self-tests');
    },
} satisfies Window["electron"]);


function ipcInvoke<Key extends keyof IPCHandlers>(
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
