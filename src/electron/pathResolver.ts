import path from "path";
import { app } from "electron";
import { isDev } from "./utils.js";

export function getPreloadPath(){
    const preloadPath = path.join(
        app.getAppPath(),
        isDev() ? ".": "..",
        "/dist-electron/preload.cjs"
    )
    console.log('getPreloadPath: preloadPath =', preloadPath);
    return preloadPath;
}

export function getUIPath(){
    return path.join(app.getAppPath(), "/dist-electron/index.html");
}