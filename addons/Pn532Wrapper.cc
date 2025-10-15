#include "Pn532Wrapper.h"
#include "PN532_Controller/Headers/SerialCommunication.h"
#include "PN532_Controller/Headers/pn532.h"
#include <iostream>
#include <memory>

using namespace Napi;

std::unique_ptr<NFC_Controller::Cpp::SerialCommunication> m_protocol = std::make_unique<NFC_Controller::Cpp::SerialCommunication>();
std::unique_ptr<NFC_Controller::Cpp::NFC> m_nfc_chip = std::make_unique<NFC_Controller::Cpp::PN532_chip>(*m_protocol);
// -----------------------------------------------------------------------------
// PN532 Wrapper class declaration
// -----------------------------------------------------------------------------

PN532_Wrapper::PN532_Wrapper(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<PN532_Wrapper>(info)
{
    Napi::Env env = info.Env();
}

Napi::Value PN532_Wrapper::Init(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    m_protocol->init("COM5", 115200);
    m_nfc_chip->SAMConfiguration(NFC_Controller::Cpp::pn532::command::SAMmode::Normal_mode);
    m_nfc_chip->setMaxRetries(0xFF);

    return Napi::Boolean::New(env, true);
}

Napi::Value PN532_Wrapper::GetFirmwareVersion(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    char firmware_string[255];
    auto firmware = m_nfc_chip->getFirmwareVersion();
    if(firmware[0] == NFC_Controller::Cpp::statusCode::pn532StatusOK){
        sprintf(firmware_string, "Found NFC device PN5%X\nFirmware version %X.%X.%X", firmware[1], firmware[2], firmware[3], firmware[4]);
        // std::cout << firmware_string << std::endl;
    } else {
        sprintf(firmware_string, "Didn't find PN532 board");
        // std::cout << firmware_string << std::endl;
    }

    return Napi::String::New(env, firmware_string);
}

Napi::Function PN532_Wrapper::GetClass(Napi::Env env)
{
    return DefineClass(
        env,
        "PN532_Wrapper",
        {
            InstanceMethod("init", &PN532_Wrapper::Init),
            InstanceMethod("getFirmwareVersion", &PN532_Wrapper::GetFirmwareVersion)
        }
    );
}