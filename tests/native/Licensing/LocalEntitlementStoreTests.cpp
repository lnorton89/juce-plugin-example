#include "LumaScope/Licensing/LocalEntitlementStore.h"
#include <juce_core/juce_core.h>
#include <iostream>

static juce::File createTempDir()
{
    auto tmp = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto dir = tmp.getNonexistentChildFile("lms_entitlement_test_", "", 0);
    dir.createDirectory();
    return dir;
}

int runLocalEntitlementStoreTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // testWriteReadRoundtrip
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        const std::string json = R"({"hello":"world","num":42})";
        expect(store.write(json), "write succeeds");
        auto read = store.read();
        expect(read.has_value(), "read returns value");
        expect(read.value() == json, "read content matches written content");

        dir.deleteRecursively();
    }

    // testReadNonExistent
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        auto read = store.read();
        expect(!read.has_value(), "read returns nullopt for empty store");

        dir.deleteRecursively();
    }

    // testClearAfterWrite
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        store.write("test data");
        expect(store.clear(), "clear returns true");
        expect(!store.read().has_value(), "read returns nullopt after clear");

        dir.deleteRecursively();
    }

    // testWriteEmptyRejected
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        expect(!store.write(""), "write empty returns false");

        dir.deleteRecursively();
    }

    // testWriteOversizeRejected
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        std::string big(65 * 1024, 'x');
        expect(!store.write(big), "write oversized returns false");

        dir.deleteRecursively();
    }

    // testExistsAfterWrite
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        expect(!store.exists(), "exists false before write");
        store.write("data");
        expect(store.exists(), "exists true after write");

        dir.deleteRecursively();
    }

    // testExistsAfterClear
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        store.write("data");
        store.clear();
        expect(!store.exists(), "exists false after clear");

        dir.deleteRecursively();
    }

    // testWriteCorruptRead
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        // Write a corrupt file directly (not through the store) so we
        // can create a known-bad blob without DPAPI involvement
        auto file = dir.getChildFile("entitlement.dat");
        {
            juce::FileOutputStream stream(file);
            if (stream.openedOk())
            {
                stream.write("NOT_A_DAPI_BLOB", 16);
                stream.flush();
            }
        }
        expect(file.existsAsFile(), "entitlement file exists");

        auto read = store.read();
        expect(!read.has_value(), "corrupted file read returns nullopt");

        dir.deleteRecursively();
    }

    // testFilePathStructure
    {
        auto path = lumascope::LocalEntitlementStore::getEntitlementFilePath();
        expect(path.getFullPathName().contains("LumaScope"), "path contains LumaScope");
        expect(path.getFileName() == "entitlement.dat", "filename is entitlement.dat");
    }

    // testWriteBinaryNotHumanReadable
    {
        auto dir = createTempDir();
        lumascope::LocalEntitlementStore store;
        store.setBasePathForTesting(dir);

        store.write(R"({"secret":"value"})");

        auto file = dir.getChildFile("entitlement.dat");
        juce::FileInputStream stream(file);
        if (stream.openedOk())
        {
            auto size = stream.getTotalLength();
            std::vector<char> raw(static_cast<size_t>(size));
            stream.read(raw.data(), size);
            std::string rawStr(raw.data(), raw.size());
            expect(rawStr.find("secret") == std::string::npos,
                   "DPAPI blob does not contain plaintext JSON");
        }

        dir.deleteRecursively();
    }

    if (failures > 0)
        std::cerr << "runLocalEntitlementStoreTests: " << failures << " failure(s)\n";
    return failures;
}
