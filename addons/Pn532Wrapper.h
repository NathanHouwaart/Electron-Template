#pragma once

#include <napi.h>

class PN532_Wrapper : public Napi::ObjectWrap<PN532_Wrapper> {
public:
    PN532_Wrapper(const Napi::CallbackInfo&);
    Napi::Value Init(const Napi::CallbackInfo&);
    Napi::Value GetFirmwareVersion(const Napi::CallbackInfo&);

    static Napi::Function GetClass(Napi::Env);
};
