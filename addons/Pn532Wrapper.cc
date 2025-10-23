#include "Pn532Wrapper.h"
#include "PN532_Controller/Headers/SerialCommunication.h"
#include "PN532_Controller/Headers/pn532.h"
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <utility>
#include <thread>
#include <chrono>
#include "AddonLog.h"
#include "PN532_Controller/Headers/Commands/getFirmwareVersion.h"
#include "PN532_Controller/Headers/Commands/performSelfTestCommand.h"
#include "PN532_Controller/Headers/Commands/inListPassiveTarget.h"
#include "PN532_Controller/Headers/Cards/KeyVersion.h"

using namespace Napi;

std::unique_ptr<NFC_Controller::Cpp::SerialCommunication> m_protocol = std::make_unique<NFC_Controller::Cpp::SerialCommunication>();
std::unique_ptr<NFC_Controller::Cpp::PN532_chip> m_nfc_chip = std::make_unique<NFC_Controller::Cpp::PN532_chip>(*m_protocol);

// -----------------------------------------------------------------------------
// Self Test Async Worker
// -----------------------------------------------------------------------------

class SelfTestWorker : public Napi::AsyncProgressWorker<std::pair<std::string, std::string>>
{
public:
    SelfTestWorker(Napi::Function &callback, Napi::Function &progress)
        : Napi::AsyncProgressWorker<std::pair<std::string, std::string>>(callback),
          progressCallback(Napi::Persistent(progress))
    {
    }

    void Execute(const ExecutionProgress &progress) override
    {
        using PerformSelfTestCommand = NFC_Controller::Cpp::PerformSelfTestCommand;
        using Options = PerformSelfTestCommand::Options;

        // ROM Self-test
        auto romRunning = std::make_pair(std::string("rom"), std::string("running"));
        progress.Send(&romRunning, 1);
        auto romCommand = PerformSelfTestCommand(Options{.test = PerformSelfTestCommand::Test::RomChecksum});
        auto romResult = m_nfc_chip->executeCommand(romCommand);
        std::string romStatus = (int(romResult.status) == 0) ? "success" : "failed";
        auto romComplete = std::make_pair(std::string("rom"), romStatus);
        progress.Send(&romComplete, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // RAM Self-test
        auto ramRunning = std::make_pair(std::string("ram"), std::string("running"));
        progress.Send(&ramRunning, 1);
        auto ramCommand = PerformSelfTestCommand(Options{.test = PerformSelfTestCommand::Test::RamIntegrity});
        auto ramResult = m_nfc_chip->executeCommand(ramCommand);
        std::string ramStatus = (int(ramResult.status) == 0) ? "success" : "failed";
        auto ramComplete = std::make_pair(std::string("ram"), ramStatus);
        progress.Send(&ramComplete, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Communication Line Test
        auto commRunning = std::make_pair(std::string("communication"), std::string("running"));
        progress.Send(&commRunning, 1);
        auto commCommand = PerformSelfTestCommand(Options{
            .test = PerformSelfTestCommand::Test::CommunicationLine,
            .parameters = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA}});
        auto commResult = m_nfc_chip->executeCommand(commCommand);
        std::string commStatus = (int(commResult.status) == 0) ? "success" : "failed";
        auto commComplete = std::make_pair(std::string("communication"), commStatus);
        progress.Send(&commComplete, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Echo Back Test
        auto echoRunning = std::make_pair(std::string("echo"), std::string("running"));
        progress.Send(&echoRunning, 1);
        auto echoCommand = PerformSelfTestCommand(Options{
            .test = PerformSelfTestCommand::Test::EchoBack,
            .parameters = {0xBA, 0xAD, 0xF0, 0x0D, 0x12, 0x34, 0x56, 0x78}});
        auto echoResult = m_nfc_chip->executeCommand(echoCommand);
        std::string echoStatus = (int(echoResult.status) == 0) ? "success" : "failed";
        auto echoComplete = std::make_pair(std::string("echo"), echoStatus);
        progress.Send(&echoComplete, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Antenna Continuity Test
        auto antennaRunning = std::make_pair(std::string("antenna"), std::string("running"));
        progress.Send(&antennaRunning, 1);
        auto antennaCommand = PerformSelfTestCommand(Options{
            .test = PerformSelfTestCommand::Test::AntennaContinuity,
            .parameters = {PerformSelfTestCommand::makeAntennaThreshold(
                static_cast<uint8_t>(1u << 1),
                static_cast<uint8_t>(1u << 0),
                true, true)}});
        auto antennaResult = m_nfc_chip->executeCommand(antennaCommand);
        std::string antennaStatus = (int(antennaResult.status) == 0) ? "success" : "failed";
        auto antennaComplete = std::make_pair(std::string("antenna"), antennaStatus);
        progress.Send(&antennaComplete, 1);
    }

    void OnProgress(const std::pair<std::string, std::string> *data, size_t count) override
    {
        Napi::HandleScope scope(Env());
        for (size_t i = 0; i < count; i++)
        {
            Napi::Object result = Napi::Object::New(Env());
            result.Set("test", data[i].first);
            result.Set("status", data[i].second);
            progressCallback.Value().Call({result});
        }
    }

    void OnOK() override
    {
        Callback().Call({Env().Null(), Napi::Boolean::New(Env(), true)});
    }

    void OnError(const Napi::Error &error) override
    {
        Callback().Call({error.Value(), Env().Undefined()});
    }

private:
    Napi::FunctionReference progressCallback;
};

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

    using TargetType = NFC_Controller::Cpp::TargetType;

    auto detectedCard = m_nfc_chip->detectCard(TargetType::TypeA_106kbps);

    // Read-only access example for DESFire base
    if (auto df = std::get_if<MifareDesfireEV2Card>(&detectedCard)) {
        // DesfireVersionInfo info;
        // if (df->getVersion(info)) {
        //     std::cout << "Desfire SW version: "
        //               << int(info.softwareInfo.swMajorVersion) << "."
        //               << int(info.softwareInfo.swMinorVersion) << std::endl;
        // }
        uint8_t keyVersion = 0;
        if(!df->getKeyVersion(0x00, keyVersion)) {
            Napi::Error::New(env, "Failed to get key version")
                .ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }
        auto algo = desfire::keyVersionGetAlgo(keyVersion);
        auto rev = desfire::keyVersionGetRevision(keyVersion);
        std::cout << "Key 0 ver=0x" << std::hex << int(keyVersion) << std::dec
                  << " algo=" << desfire::keyAlgoToString(algo)
                  << " rev=" << int(rev) << "\n";

        std::cout << "Key version for key 0: " << Hex0x(keyVersion) << "\n";

        Sleep(1000);
        std::array<uint8_t, 16> RndB = {};
        // df->authenticateAES(0x00, RndB);            // Authenticate with key 0 on AES
        
        df->authenticate();

        return Napi::Boolean::New(env, true);
    }

    // Mifare Classic example
    if (auto mc = std::get_if<MifareClassicCard>(&detectedCard)) {
        std::cout << "MIFARE Classic UID:";
        for (auto b : mc->uid()) std::cout << " " << Hex0x(b);
        std::cout << std::endl;
        return Napi::Boolean::New(env, true);
    }

    std::cout << "No usable card inside variant (monostate or unknown type)\n";    

    Napi::Error::New(env, "No usable card detected")
        .ThrowAsJavaScriptException();
    return Napi::Boolean::New(env, false);

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
}

Napi::Value PN532_Wrapper::RunSelfTests(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsFunction() || !info[1].IsFunction())
    {
        Napi::TypeError::New(env, "Expected two callback functions (progress, complete)")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    Napi::Function progressCallback = info[0].As<Napi::Function>();
    Napi::Function completeCallback = info[1].As<Napi::Function>();

    SelfTestWorker *worker = new SelfTestWorker(completeCallback, progressCallback);
    worker->Queue();

    return env.Undefined();
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
         InstanceMethod("runSelfTests", &PN532_Wrapper::RunSelfTests),
        });
}
