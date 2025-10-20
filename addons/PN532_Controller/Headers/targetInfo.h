#include <cstdint>
#include <cstring>

#if defined(_MSC_VER)
#   pragma pack(push, 1)
#endif

    #include <variant>
    #include <cstdint>
    #include <array>

    struct Iso14443ATarget
    {
        uint8_t  tg;
        uint16_t atqa;        // SENS_RES
        uint8_t  sak;         // SEL_RES
        uint8_t  uidLen;
        std::array<uint8_t, 10> uid{}; // UID max 10 bytes
        uint8_t  atsLen;
        std::array<uint8_t, 64> ats{}; // ATS bytes if ISO-DEP
    }
    #if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
    #endif
    ;

    struct Iso14443BTarget
    {
        uint8_t tg;
        std::array<uint8_t, 4> pupi{};
        std::array<uint8_t, 4> applicationData{};
        std::array<uint8_t, 3> protocolInfo{};
        uint8_t attribResLen;
        std::array<uint8_t, 17> attribRes{};
    }
    #if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
    #endif
    ;

    struct FelicaTarget
    {
        uint8_t tg;
        uint8_t sensResLen;
        std::array<uint8_t, 8> idm{};
        std::array<uint8_t, 8> pmm{};
        std::array<uint8_t, 2> sysCode{};
    }
    #if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
    #endif
    ;

    struct JewelTarget
    {
        uint8_t tg;
        std::array<uint8_t, 2> sensRes{};
        std::array<uint8_t, 4> id{};
    }
    #if defined(__GNUC__) || defined(__clang__)
    __attribute__((packed))
    #endif
    ;

#if defined(_MSC_VER)
#   pragma pack(pop)
#endif

using TargetInfo = std::variant<Iso14443ATarget, Iso14443BTarget, FelicaTarget, JewelTarget>;


#include <variant>

using Pn532TargetInfo = std::variant<Iso14443ATarget,
                                     Iso14443BTarget,
                                     FelicaTarget,
                                     JewelTarget>;

#include <iostream>

struct Pn532Target
{
    enum class Type { TypeA, TypeB, FeliCa, Jewel, Unknown } type;
    uint8_t nbTg;
    Pn532TargetInfo info;

    bool isTypeA() const { return type == Type::TypeA; }
    bool isTypeB() const { return type == Type::TypeB; }
    bool isFeliCa() const { return type == Type::FeliCa; }
    bool isJewel() const { return type == Type::Jewel; }

    void print() const
    {
        std::visit([](const auto& t) {
            using T = std::decay_t<decltype(t)>;
            if constexpr (std::is_same_v<T, Iso14443ATarget>) {
                std::cout << "TypeA UID: ";
                for (auto b : t.uid) std::cout << std::hex << +b << " ";
                std::cout << "\n";
            } else if constexpr (std::is_same_v<T, Iso14443BTarget>) {
                std::cout << "TypeB PUPI: ";
                for (auto b : t.pupi) std::cout << std::hex << +b << " ";
                std::cout << "\n";
            } else if constexpr (std::is_same_v<T, FelicaTarget>) {
                std::cout << "FeliCa IDm: ";
                for (auto b : t.idm) std::cout << std::hex << +b << " ";
                std::cout << "\n";
            } else if constexpr (std::is_same_v<T, JewelTarget>) {
                std::cout << "Jewel ID: ";
                for (auto b : t.id) std::cout << std::hex << +b << " ";
                std::cout << "\n";
            }
        }, info);
    }
};