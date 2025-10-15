declare module '../../build/deviceaddon.node' {
  export function connect(port: string): boolean;
  export function readData(): string;
}

declare module '*.node' {
  const value: any;
  export default value;
}
