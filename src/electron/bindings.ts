// ...existing code...
import { createRequire } from 'module';
import path from 'path';
import fs from 'fs';
import { fileURLToPath } from 'url';

const require = createRequire(import.meta.url);
const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

// common places cmake-js / node-gyp put the .node file
const candidates = [
  path.resolve(process.cwd(), 'build', 'Release', 'deviceaddon.node'),
  path.resolve(process.cwd(), 'build', 'deviceaddon.node'),
  path.resolve(__dirname, '..', '..', 'build', 'Release', 'deviceaddon.node'),
  path.resolve(__dirname, '..', '..', 'build', 'deviceaddon.node')
];

console.log('bindings: process.cwd() =', process.cwd());
console.log('bindings: __filename =', __filename);
console.log('bindings: __dirname =', __dirname);
console.log('bindings: candidates =', candidates);

const addonPath = candidates.find(p => fs.existsSync(p));
console.log('bindings: addonPath found =', addonPath);

if (!addonPath) {
  throw new Error(
    'deviceaddon.node not found. Run `npm run build:addon:rebuild` and confirm output location. Checked: ' +
    candidates.join(';')
  );
}

const addon: any = require(addonPath);

export interface MyObject {
    greet(str: string): string;
    add(a: number, b: number): number;
}

export interface PN532_Wrapper {
    init(info: string): void;
    connect(port: string): Promise<boolean>;
    disconnect(): Promise<boolean>;
    getFirmwareVersion(): string;
    getVersion(): boolean;
}

export const MyObject: {
    new(name: string): MyObject;
} = addon.MyObject;

export const PN532_Wrapper: {
    new(): PN532_Wrapper;
} = addon.PN532_Wrapper;
// ...existing code...