#include <napi.h>
#include "Device.hpp"

Napi::Boolean Connect(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string port = info[0].As<Napi::String>().Utf8Value();
    // Log via Node's console to ensure output appears in Electron/Node logs
    if (env.Global().Has("console")) {
        Napi::Object console = env.Global().Get("console").As<Napi::Object>();
        if (console.Has("log")) {
            Napi::Function log = console.Get("log").As<Napi::Function>();
            log.Call(console, { Napi::String::New(env, "[deviceaddon] Connecting to serial port: " + port) });
        }
    }

    bool success = Device::connect(port);
    return Napi::Boolean::New(env, success);
}

Napi::String ReadData(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    std::string data = Device::readData();
    // Log data via Node's console so it's visible in Electron's output
    if (env.Global().Has("console")) {
        Napi::Object console = env.Global().Get("console").As<Napi::Object>();
        if (console.Has("log")) {
            Napi::Function log = console.Get("log").As<Napi::Function>();
            log.Call(console, { Napi::String::New(env, std::string("[deviceaddon] ReadData: ") + data) });
        }
    }

    return Napi::String::New(env, data);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("connect", Napi::Function::New(env, Connect));
    exports.Set("readData", Napi::Function::New(env, ReadData));
    return exports;
}

NODE_API_MODULE(deviceaddon, Init)
