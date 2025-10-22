

#include "MyObject.h"
#include "Pn532Wrapper.h"

#include <napi.h>


Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    Napi::String name;
    
    name = Napi::String::New(env, "MyObject");
    exports.Set(name, MyObject::GetClass(env));

    name = Napi::String::New(env, "PN532_Wrapper");
    exports.Set(name, PN532_Wrapper::GetClass(env));

    return exports;
}

NODE_API_MODULE(addon, Init)