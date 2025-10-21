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
  onSelfTestProgress: (callback: (payload: SelfTestUpdate) => void) => () => void;
};

// 4) augment global Window so you only maintain IPCHandlers
interface Window {
  electron: ExposedElectronAPI;
}
