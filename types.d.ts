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

type EventPayloadMapping = {
  statistics: Statistics;
  getStaticData: StaticData;
  getFirmwareVersion: string;
  listComPorts: { path: string; manufacturer?: string }[];
}

type UnsubscribeFunction = () => void;

interface Window {
  electron: {
    subscribeStatistics: (callback: (statistics: Statistics) => void) => UnsubscribeFunction;
    getStaticData: () => Promise<StaticData>;
    getFirmwareVersion: () => Promise<string>;
    listComPorts: () => Promise<{ path: string; manufacturer?: string }[]>;
  }
}
