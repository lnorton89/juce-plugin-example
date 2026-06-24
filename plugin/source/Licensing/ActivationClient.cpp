#include "LumaScope/Licensing/ActivationClient.h"

namespace lumascope {

ActivationClient::ActivationClient(const juce::String& baseUrl)
    : baseUrl_(baseUrl),
      thread_("LumaScope Activation Thread")
{
    thread_.startThread(juce::Thread::Priority::normal);
    thread_.addTimeSliceClient(this, 50);
}

ActivationClient::~ActivationClient()
{
    cancelAll();
    thread_.stopThread(1000);
}

void ActivationClient::setHttpFactoryForTesting(HttpFactory factory)
{
    httpFactory_ = std::move(factory);
}

void ActivationClient::activate(ActivationRequestInput input, Callback callback)
{
    juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
    bodyObj->setProperty("licenseKey", juce::String(input.licenseKey).substring(0, 256));
    bodyObj->setProperty("machineId", juce::String(input.machineId).substring(0, 128));
    bodyObj->setProperty("requestId", juce::String(buildRequestId()));
    bodyObj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    bodyObj->setProperty("appVersion", JucePlugin_VersionString);

    const auto body = juce::JSON::toString(juce::var(bodyObj.get()), false).toStdString();
    enqueue("/api/v1/activate", body, std::move(callback));
}

void ActivationClient::validate(ActivationRequestInput input, Callback callback)
{
    juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
    bodyObj->setProperty("licenseKey", juce::String(input.licenseKey).substring(0, 256));
    bodyObj->setProperty("machineId", juce::String(input.machineId).substring(0, 128));
    bodyObj->setProperty("requestId", juce::String(buildRequestId()));
    bodyObj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    bodyObj->setProperty("appVersion", JucePlugin_VersionString);

    const auto body = juce::JSON::toString(juce::var(bodyObj.get()), false).toStdString();
    enqueue("/api/v1/validate", body, std::move(callback));
}

void ActivationClient::deactivate(ActivationRequestInput input, Callback callback)
{
    juce::DynamicObject::Ptr bodyObj = new juce::DynamicObject();
    bodyObj->setProperty("licenseKey", juce::String(input.licenseKey).substring(0, 256));
    bodyObj->setProperty("machineId", juce::String(input.machineId).substring(0, 128));
    bodyObj->setProperty("requestId", juce::String(buildRequestId()));
    bodyObj->setProperty("timestamp", juce::Time::getCurrentTime().toISO8601(true));
    bodyObj->setProperty("appVersion", JucePlugin_VersionString);

    const auto body = juce::JSON::toString(juce::var(bodyObj.get()), false).toStdString();
    enqueue("/api/v1/deactivate", body, std::move(callback));
}

void ActivationClient::enqueue(const std::string& endpoint, const std::string& body, Callback callback)
{
    const juce::ScopedLock lock(queueLock_);
    queue_.push_back({endpoint, body, std::move(callback), 0});
    thread_.addTimeSliceClient(this, 50);
}

void ActivationClient::cancelAll()
{
    const juce::ScopedLock lock(queueLock_);
    cancelled_ = true;
    queue_.clear();
}

int ActivationClient::useTimeSlice()
{
    PendingRequest request;
    {
        const juce::ScopedLock lock(queueLock_);
        if (cancelled_ || queue_.empty())
            return 500;
        request = std::move(queue_.front());
        queue_.erase(queue_.begin());
    }

    performRequest(std::move(request));

    {
        const juce::ScopedLock lock(queueLock_);
        if (queue_.empty())
            return 500;
    }
    return 5;
}

void ActivationClient::performRequest(PendingRequest request)
{
    {
        const juce::ScopedLock lock(queueLock_);
        if (cancelled_)
            return;
    }

    if (httpFactory_)
    {
        auto url = juce::URL(baseUrl_ + juce::String(request.endpoint));
        auto stream = httpFactory_(url, request.body);
        if (!stream)
        {
            request.callback(makeNetworkError("request_failed", "Mock HTTP request failed"));
            return;
        }

        ActivationResult result;
        result.type = ActivationResult::Type::success;
        result.statusCode = 200;
        const auto data = stream->readEntireStreamAsString();
        result.responseBody = data.toStdString();
        request.callback(result);
        return;
    }

    auto url = juce::URL(baseUrl_ + juce::String(request.endpoint))
                   .withPOSTData(juce::String(request.body));
    juce::StringPairArray responseHeaders;
    int statusCode = 0;

    std::unique_ptr<juce::InputStream> stream(url.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders("Content-Type: application/json\r\n")
            .withResponseHeaders(&responseHeaders)
            .withStatusCode(&statusCode)
            .withConnectionTimeoutMs(10000)
            .withNumRedirectsToFollow(3)));

    if (!stream)
    {
        if (statusCode > 0)
        {
            ActivationResult result;
            result.type = statusCode >= 500 ? ActivationResult::Type::error : ActivationResult::Type::error;
            result.statusCode = statusCode;
            result.isServerError = statusCode >= 500;
            result.errorCode = "http_error";
            result.errorMessage = "HTTP " + std::to_string(statusCode);
            request.callback(result);
        }
        else
        {
            request.callback(makeNetworkError("connection_failed", "Could not connect to activation server"));
        }
        return;
    }

    const auto data = stream->readEntireStreamAsString().toStdString();

    ActivationResult result;
    result.type = (statusCode >= 200 && statusCode < 300) ? ActivationResult::Type::success : ActivationResult::Type::error;
    result.statusCode = statusCode;
    result.responseBody = data;
    result.isServerError = statusCode >= 500;

    if (result.type == ActivationResult::Type::error)
    {
        auto json = juce::JSON::parse(juce::String(data));
        auto* obj = json.getDynamicObject();
        if (obj)
        {
            result.errorCode = obj->getProperty("code").toString().toStdString();
            result.errorMessage = obj->getProperty("message").toString().toStdString();
        }
    }

    request.callback(result);
}

ActivationResult ActivationClient::makeNetworkError(const std::string& code, const std::string& msg)
{
    ActivationResult result;
    result.type = ActivationResult::Type::networkError;
    result.errorCode = code;
    result.errorMessage = msg;
    return result;
}

std::string ActivationClient::buildRequestId()
{
    return juce::Uuid().toString().toStdString();
}

} // namespace lumascope
