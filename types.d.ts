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

// Define each channel as a function type (args -> Promise<return>)
type IPCHandlers = {
  statistics: () => Promise<Statistics>;
  getStaticData: () => Promise<StaticData>;
  connect: (port: string) => Promise<boolean>;
  disconnect: () => Promise<boolean>;
  getFirmwareVersion: () => Promise<string>;
  listComPorts: () => Promise<{ path: string; manufacturer?: string }[]>;
};

// Derived helpers
type EventInvokeArgs = { [K in keyof IPCHandlers]: Parameters<IPCHandlers[K]> };
type EventPayloadMapping = { [K in keyof IPCHandlers]: Awaited<ReturnType<IPCHandlers[K]>> };

// Keep Window.electron typed conveniently
type UnsubscribeFunction = () => void;

interface Window {
  electron: {
    subscribeStatistics: (callback: (statistics: Statistics) => void) => UnsubscribeFunction;
    getStaticData: () => Promise<StaticData>;
    connect: (port: string) => Promise<boolean>;
    disconnect: () => Promise<boolean>;
    getFirmwareVersion: () => Promise<string>;
    listComPorts: () => Promise<{ path: string; manufacturer?: string }[]>;
  }
}