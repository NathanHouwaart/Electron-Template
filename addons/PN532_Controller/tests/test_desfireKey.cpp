#include <gtest/gtest.h>

#include "desfireKey.h"
#include "Cards/KeyVersion.h"

using namespace desfire;

TEST(DesfireKeyBasic, DES_GeneratesCmac)
{
    constexpr size_t KEY_SIZE = DesfireKeyTraits<DesfireKeyType::DES>::keySize;
    std::array<uint8_t, KEY_SIZE> keyData{};
    for (size_t i = 0; i < KEY_SIZE; ++i) keyData[i] = static_cast<uint8_t>(i + 1);

    uint8_t version = makeKeyVersion(DesfireKeyType::DES, 1);
    DesfireKey<DesfireKeyType::DES> key(keyData, version);

    EXPECT_EQ(key.getKeyTypeAsString(), std::string("DES"));

    auto kspan = key.getKey();
    ASSERT_EQ(kspan.size(), keyData.size());
    for (size_t i = 0; i < KEY_SIZE; ++i)
        EXPECT_EQ(kspan[i], keyData[i]);

    // Generate CMAC subkeys and ensure they are not all zero
    EXPECT_TRUE(key.GenerateCmacSubkeys());
    auto subkeys = key.getCmacSubkeys();

    bool allzero1 = std::all_of(subkeys.first.begin(), subkeys.first.end(), [](uint8_t b) { return b == 0; });
    bool allzero2 = std::all_of(subkeys.second.begin(), subkeys.second.end(), [](uint8_t b) { return b == 0; });

    EXPECT_FALSE(allzero1);
    EXPECT_FALSE(allzero2);
}
