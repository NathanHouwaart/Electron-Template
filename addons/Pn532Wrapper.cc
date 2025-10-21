#include "Pn532Wrapper.h"
#include "PN532_Controller/Headers/SerialCommunication.h"
#include "PN532_Controller/Headers/pn532.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include "AddonLog.h"
#include "PN532_Controller/Headers/Commands/getFirmwareVersion.h"
#include "PN532_Controller/Headers/Commands/performSelfTestCommand.h"

using namespace Napi;

std::unique_ptr<NFC_Controller::Cpp::SerialCommunication> m_protocol = std::make_unique<NFC_Controller::Cpp::SerialCommunication>();
std::unique_ptr<NFC_Controller::Cpp::PN532_chip> m_nfc_chip = std::make_unique<NFC_Controller::Cpp::PN532_chip>(*m_protocol);

// -----------------------------------------------------------------------------
// PN532 Wrapper class declaration
// -----------------------------------------------------------------------------

PN532_Wrapper::PN532_Wrapper(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<PN532_Wrapper>(info)
{
    Napi::Env env = info.Env();
}

Napi::Value PN532_Wrapper::Connect(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() != 1)
    {
        Napi::TypeError::New(env, "Connect expects exactly one argument")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    if (!info[0].IsString())
    {
        Napi::TypeError::New(env, "Expected a string argument")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    Napi::String comPort = info[0].As<Napi::String>();

    Log("Initializing PN532 on port: " + comPort.Utf8Value());

    if (!m_protocol->init(comPort, 115200))
    {
        Napi::Error::New(env, "Failed to initialize serial communication")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    auto result = m_nfc_chip->SAMConfiguration(NFC_Controller::Cpp::pn532::command::SAMmode::Normal_mode);
    if (result != NFC_Controller::Cpp::statusCode::pn532StatusOK)
    {
        Napi::Error::New(env, "Failed to configure SAM")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    result = m_nfc_chip->setMaxRetries(0x01); // Set max retries to 1
    if (result != NFC_Controller::Cpp::statusCode::pn532StatusOK)
    {
        Napi::Error::New(env, "Failed to set max retries")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }

    return Napi::Boolean::New(env, true);
}

Napi::Value PN532_Wrapper::Disconnect(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    auto result = m_protocol->close_port();
    if (!result)
    {
        Napi::Error::New(env, "Failed to close serial communication")
            .ThrowAsJavaScriptException();
        return Napi::Boolean::New(env, false);
    }
    return Napi::Boolean::New(env, result);
}

Napi::Value PN532_Wrapper::GetFirmwareVersion(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    char firmware_string[255];
    auto firmware = m_nfc_chip->getFirmwareVersion();
    if (firmware[0] == NFC_Controller::Cpp::statusCode::pn532StatusOK)
    {
        sprintf(firmware_string, "Found NFC device PN5%X\nFirmware version %X.%X.%X", firmware[1], firmware[2], firmware[3], firmware[4]);
        // std::cout << firmware_string << std::endl;
    }
    else
    {
        sprintf(firmware_string, "Didn't find PN532 board");
        // std::cout << firmware_string << std::endl;
    }

    return Napi::String::New(env, firmware_string);
}

Napi::Value PN532_Wrapper::GetVersion(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    auto cardInfo = NFC_Controller::Cpp::card();
    uint8_t cardType = NFC_Controller::Cpp::pn532::command::CardType::TypeA_ISO_IEC14443; // Cardtype we want to detect
    uint8_t cardNumber = 0x01;
    Ringbuffer<uint8_t, 64> response;

    auto command = NFC_Controller::Cpp::GetFirmwareVersionCommand();
    auto res = m_nfc_chip->executeCommand(command);

    command.firmware().printInfo();

    Log("Performing self-test...");
    using PerformSelfTestCommand = NFC_Controller::Cpp::PerformSelfTestCommand;
    using Options = PerformSelfTestCommand::Options;

    {
        auto romSelfTestCommand = PerformSelfTestCommand(
            Options{
                .test = NFC_Controller::Cpp::PerformSelfTestCommand::Test::RomChecksum});
        auto selfTestResult = m_nfc_chip->executeCommand(romSelfTestCommand);
        Log("ROM Self-test result: " + std::to_string(int(selfTestResult.status)));
    }
    {
        auto ramSelfTestCommand = PerformSelfTestCommand(
            Options{
                .test = NFC_Controller::Cpp::PerformSelfTestCommand::Test::RamIntegrity});
        auto selfTestResult = m_nfc_chip->executeCommand(ramSelfTestCommand);
        Log("RAM Self-test result: " + std::to_string(int(selfTestResult.status)));
    }
    {
        auto communicationLineTestCommand = PerformSelfTestCommand(
            Options{
                .test = NFC_Controller::Cpp::PerformSelfTestCommand::Test::CommunicationLine,
                .parameters = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA}});
        auto selfTestResult = m_nfc_chip->executeCommand(communicationLineTestCommand);
        Log("Communication Line Self-test result: " + std::to_string(int(selfTestResult.status)));
    }
    {
        auto echoBackTestCommand = PerformSelfTestCommand(
            Options{
                .test = NFC_Controller::Cpp::PerformSelfTestCommand::Test::EchoBack,
                .parameters = {0xBA, 0xAD, 0xF0, 0x0D, 0x12, 0x34, 0x56, 0x78}});
        auto selfTestResult = m_nfc_chip->executeCommand(echoBackTestCommand);
        Log("Echo Back Self-test result: " + std::to_string(int(selfTestResult.status)));
    }
    {
        auto selfAntennaTestCommand = PerformSelfTestCommand(
            Options{
                .test = NFC_Controller::Cpp::PerformSelfTestCommand::Test::AntennaContinuity,
                .parameters = {
                    PerformSelfTestCommand::makeAntennaThreshold(
                        /*highThresholdCode=*/static_cast<uint8_t>(1u << 1),
                        /*lowThresholdCode=*/static_cast<uint8_t>(1u << 0),
                        /*useUpperComparator=*/true,
                        /*useLowerComparator=*/true)}});
        Log("Performing Antenna Continuity Self-test...");
        auto selfTestResult = m_nfc_chip->executeCommand(selfAntennaTestCommand);
        Log("Antenna Continuity Self-test result: " + std::to_string(int(selfTestResult.status)));
    }

    // if(!m_nfc_chip->detectCard(cardInfo, cardNumber, cardType, &response)){
    //     Napi::Error::New(env, "No card detected")
    //         .ThrowAsJavaScriptException();
    //     return Napi::Boolean::New(env, false);
    // }

    // auto result = m_nfc_chip->getVersion();
    // if(result != NFC_Controller::Cpp::statusCode::pn532StatusOK){
    //     Napi::Error::New(env, "Failed to get version")
    //         .ThrowAsJavaScriptException();
    //     return Napi::Boolean::New(env, false);
    // }

    return Napi::Boolean::New(env, true);
}


Napi::Function PN532_Wrapper::GetClass(Napi::Env env)
{
    return DefineClass(
        env,
        "PN532_Wrapper",
        {InstanceMethod("connect", &PN532_Wrapper::Connect),
         InstanceMethod("disconnect", &PN532_Wrapper::Disconnect),
         InstanceMethod("getFirmwareVersion", &PN532_Wrapper::GetFirmwareVersion),
         InstanceMethod("getVersion", &PN532_Wrapper::GetVersion),
         InstanceMethod("runSelfTests", &PN532_Wrapper::RunSelfTests)});
}
