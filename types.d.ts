type Statistics = {
  cpuUsage: number;
  ramUsage: number;
  storageUsage: number;
}

type StaticData = {
  totalStorage: number;
  cpuModel: string;
  totalMem: number;
}

type SelfTestUpdate = {
  test: string;
  status: 'pending' | 'running' | 'success' | 'failed';
}

type RendererEvents = {
  'self-test-progress': SelfTestUpdate;
  'self-test-complete': { error: Error | null; result: boolean };
}

// 1) canonical single source: define your IPC handlers here
type IPCHandlers = {
  getStaticData: () => Promise<StaticData>;
  connect: (port: string) => Promise<boolean>;
  disconnect: () => Promise<boolean>;
  getFirmwareVersion: () => Promise<string>;
  getVersion: () => Promise<boolean>;
  listComPorts: () => Promise<{ path: string; manufacturer?: string }[]>;
};

// 2) helpers derived from IPCHandlers
type EventInvokeArgs = { [K in keyof IPCHandlers]: Parameters<IPCHandlers[K]> };
type EventPayloadMapping = { [K in keyof IPCHandlers]: Awaited<ReturnType<IPCHandlers[K]>> } & RendererEvents;

// 3) derive the exact shape to expose on window.electron
type ExposedElectronAPI = {
  [K in keyof IPCHandlers]: (...args: EventInvokeArgs[K]) => ReturnType<IPCHandlers[K]>;
} & {
  runSelfTests: (
    progressCallback: (result: SelfTestUpdate) => void,
    completeCallback: (error: Error | null, result: boolean) => void
  ) => void;
  onDeviceDisconnected: (callback: () => void) => () => void;
  onDeviceConnected: (callback: (payload: { port: string }) => void) => () => void;
};

// 4) augment global Window so you only maintain IPCHandlers
interface Window {
  electron: ExposedElectronAPI;
}
