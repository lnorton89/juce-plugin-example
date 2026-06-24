#include "LumaScope/Licensing/ActivationClient.h"
#include <juce_core/juce_core.h>
#include <iostream>
#include <atomic>
#include <thread>

// A simple InputStream wrapper that provides a status code signal
class MockHttpStream : public juce::InputStream {
public:
    MockHttpStream(const std::string& body, int statusCode)
        : inner_(body.data(), body.size(), false), statusCode_(statusCode) {}

    int getStatusCode() const { return statusCode_; }

    // juce::InputStream overrides — delegate to inner
    juce::int64 getPosition() override { return inner_.getPosition(); }
    bool setPosition(juce::int64 pos) override { return inner_.setPosition(pos); }
    juce::int64 getTotalLength() override { return inner_.getTotalLength(); }
    bool isExhausted() override { return inner_.isExhausted(); }
    int read(void* dest, int maxBytes) override { return inner_.read(dest, maxBytes); }
    juce::String readEntireStreamAsString() override { return inner_.readEntireStreamAsString(); }

private:
    juce::MemoryInputStream inner_;
    int statusCode_ = 0;
};

static std::unique_ptr<juce::InputStream> createMockResponse(int statusCode, const std::string& body)
{
    return std::unique_ptr<juce::InputStream>(new MockHttpStream(body, statusCode));
}

static lumascope::ActivationClient::HttpFactory mockSuccessFactory(
    const std::string& responseBody = R"({"status":"ok","token":"mock-token"})")
{
    return [responseBody](const juce::URL&, const std::string&) {
        return createMockResponse(200, responseBody);
    };
}

static lumascope::ActivationClient::HttpFactory mockErrorFactory(int statusCode, const std::string& body)
{
    return [statusCode, body](const juce::URL&, const std::string&) {
        return createMockResponse(statusCode, body);
    };
}

static lumascope::ActivationClient::HttpFactory mockNetworkErrorFactory()
{
    return [](const juce::URL&, const std::string&) -> std::unique_ptr<juce::InputStream> {
        return nullptr;
    };
}

int runActivationClientTests()
{
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* message) {
        if (!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
    };

    // testConstruction
    {
        lumascope::ActivationClient client("http://localhost:8787");
        expect(true, "client constructed without error");
    }

    // testSuccessfulActivation
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockSuccessFactory());

        std::atomic<bool> called{false};
        lumascope::ActivationResult result;

        client.activate({"test-license", "test-machine"}, [&](lumascope::ActivationResult r) {
            result = std::move(r);
            called = true;
        });

        int waited = 0;
        while (!called && waited < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++waited;
        }

        expect(called, "activation callback was invoked");
        expect(result.type == lumascope::ActivationResult::Type::success, "activation succeeded");
    }

    // testNetworkError
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockNetworkErrorFactory());

        std::atomic<bool> called{false};
        lumascope::ActivationResult result;

        client.activate({"test-license", "test-machine"}, [&](lumascope::ActivationResult r) {
            result = std::move(r);
            called = true;
        });

        int waited = 0;
        while (!called && waited < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++waited;
        }

        expect(called, "network error callback was invoked");
        expect(result.type == lumascope::ActivationResult::Type::networkError, "network error type");
    }

    // testValidateEndpoint
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockSuccessFactory());

        std::atomic<bool> called{false};
        client.validate({"test-license", "test-machine"}, [&](lumascope::ActivationResult) {
            called = true;
        });

        int waited = 0;
        while (!called && waited < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++waited;
        }

        expect(called, "validate callback was invoked");
    }

    // testDeactivateEndpoint
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockSuccessFactory());

        std::atomic<bool> called{false};
        client.deactivate({"test-license", "test-machine"}, [&](lumascope::ActivationResult) {
            called = true;
        });

        int waited = 0;
        while (!called && waited < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++waited;
        }

        expect(called, "deactivate callback was invoked");
    }

    // testCancelAll
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockSuccessFactory());

        std::atomic<int> callCount{0};
        client.activate({"test-1", "machine"}, [&](lumascope::ActivationResult) { ++callCount; });
        client.activate({"test-2", "machine"}, [&](lumascope::ActivationResult) { ++callCount; });

        client.cancelAll();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        expect(callCount == 0, "no callbacks invoked after cancelAll");
    }

    // testRequestBodyFormat
    {
        std::string capturedBody;
        auto capturingFactory = [&capturedBody](const juce::URL&, const std::string& body) {
            capturedBody = body;
            return createMockResponse(200, R"({"status":"ok"})");
        };

        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(std::move(capturingFactory));

        std::atomic<bool> called{false};
        client.activate({"KEY-123", "MACHINE-A"}, [&](lumascope::ActivationResult) {
            called = true;
        });

        int waited = 0;
        while (!called && waited < 100) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            ++waited;
        }

        expect(capturedBody.find("KEY-123") != std::string::npos, "body contains licenseKey");
        expect(capturedBody.find("MACHINE-A") != std::string::npos, "body contains machineId");
        expect(capturedBody.find("licenseKey") != std::string::npos, "body has licenseKey field");
        expect(capturedBody.find("machineId") != std::string::npos, "body has machineId field");
    }

    // testMultipleRequestsQueued
    {
        lumascope::ActivationClient client("http://localhost:8787");
        client.setHttpFactoryForTesting(mockSuccessFactory());

        std::atomic<int> callCount{0};
        client.activate({"key1", "machine"}, [&](lumascope::ActivationResult) { ++callCount; });
        client.activate({"key2", "machine"}, [&](lumascope::ActivationResult) { ++callCount; });
        client.activate({"key3", "machine"}, [&](lumascope::ActivationResult) { ++callCount; });

        int waited = 0;
        while (callCount < 3 && waited < 200) {
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            ++waited;
        }

        expect(callCount == 3, "all 3 queued requests completed");
    }

    if (failures == 0)
        std::cout << "All ActivationClient tests passed.\n";
    else
        std::cerr << failures << " ActivationClient test(s) FAILED.\n";

    return failures;
}
