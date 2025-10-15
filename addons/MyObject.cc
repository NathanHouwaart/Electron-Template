#include "MyObject.h"

using namespace Napi;

MyObject::MyObject(const Napi::CallbackInfo& info)
    : ObjectWrap(info)
{
    Napi::Env env = info.Env();
    
    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "You need to name yourself!")
            .ThrowAsJavaScriptException();
        return;
    }

    _greeterName = info[0].As<Napi::String>().Utf8Value();
}


Napi::Value MyObject::Greet(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1)
    {
        Napi::TypeError::New(env, "Wrong Number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "You need to introduce yourself to greet!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::String name = info[0].As<Napi::String>();

    printf("Hello %s\n", name.Utf8Value().c_str());
    printf("My name is %s\n", _greeterName.c_str());

    return Napi::Value();
}

Napi::Value MyObject::Add(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2)
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsNumber() || !info[1].IsNumber())
    {
        Napi::TypeError::New(env, "You need to provide two numbers to add!")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    double arg0 = info[0].As<Napi::Number>().DoubleValue();
    double arg1 = info[1].As<Napi::Number>().DoubleValue();
    Napi::Number sum = Napi::Number::New(env, arg0 + arg1);

    return sum;
}

Napi::Function MyObject::GetClass(Napi::Env env)
{
    return DefineClass(
        env,
        "MyObject",
        {
            InstanceMethod("greet", &MyObject::Greet),
            InstanceMethod("add", &MyObject::Add)
        }
    );
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    Napi::String name = Napi::String::New(env, "MyObject");
    exports.Set(name, MyObject::GetClass(env));
    return exports;
}

NODE_API_MODULE(addon, Init)