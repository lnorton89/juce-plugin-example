#pragma once
#include <juce_core/juce_core.h>
#include <functional>
#include <string>

namespace lumascope {

struct ActivationResult {
    enum class Type {
        success,
        error,
        networkError
    };

    Type type = Type::networkError;
    std::string responseBody;
    std::string errorCode;
    std::string errorMessage;
    int statusCode = 0;
    bool isServerError = false;
};

struct ActivationRequestInput {
    std::string licenseKey;
    std::string machineId;
};

class ActivationClient : private juce::TimeSliceClient {
public:
    using Callback = std::function<void(ActivationResult)>;

    explicit ActivationClient(const juce::String& baseUrl);
    ~ActivationClient() override;

    void activate(ActivationRequestInput input, Callback callback);
    void validate(ActivationRequestInput input, Callback callback);
    void deactivate(ActivationRequestInput input, Callback callback);
    void cancelAll();

    using HttpFactory = std::function<std::unique_ptr<juce::InputStream>(
        const juce::URL&, const std::string& body)>;
    void setHttpFactoryForTesting(HttpFactory factory);

private:
    int useTimeSlice() override;

    struct PendingRequest {
        std::string endpoint;
        std::string body;
        Callback callback;
        int retryCount = 0;
    };

    juce::String baseUrl_;
    juce::TimeSliceThread thread_;
    juce::CriticalSection queueLock_;
    std::vector<PendingRequest> queue_;
    HttpFactory httpFactory_;
    bool cancelled_ = false;

    void performRequest(PendingRequest request);
    ActivationResult makeNetworkError(const std::string& code, const std::string& msg);
    void enqueue(const std::string& endpoint, const std::string& body, Callback callback);

    static std::string buildRequestId();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivationClient)
};

} // namespace lumascope
