#pragma once

#include <napi.h>

class PN532_Wrapper : public Napi::ObjectWrap<PN532_Wrapper> {
public:
    PN532_Wrapper(const Napi::CallbackInfo&);
    
    Napi::Value Connect(const Napi::CallbackInfo&);
    Napi::Value Disconnect(const Napi::CallbackInfo&);

    Napi::Value GetFirmwareVersion(const Napi::CallbackInfo&);

    static Napi::Function GetClass(Napi::Env);
};
