#include "LumaScope/Licensing/EntitlementToken.h"

namespace lumascope {

static std::string escapeJsonString(const juce::String& s)
{
    std::string result;
    result.reserve(s.length() + 2);
    result += '"';
    for (auto ch : s)
    {
        switch (static_cast<juce::juce_wchar>(ch))
        {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b";  break;
            case '\f': result += "\\f";  break;
            case '\n': result += "\\n";  break;
            case '\r': result += "\\r";  break;
            case '\t': result += "\\t";  break;
            default:
                if (ch < 0x20)
                {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", static_cast<int>(ch));
                    result += buf;
                }
                else
                {
                    result += static_cast<char>(ch);
                }
                break;
        }
    }
    result += '"';
    return result;
}

std::string canonicalJson(const juce::var& value)
{
    if (value.isVoid() || value.isUndefined())
        return "null";

    if (value.isBool())
        return static_cast<bool>(value) ? "true" : "false";

    if (value.isInt())
        return std::to_string(static_cast<int>(value));

    if (value.isInt64())
        return std::to_string(static_cast<int64_t>(value));

    if (value.isDouble())
    {
        double d = static_cast<double>(value);
        if (!std::isfinite(d))
            return "null";
        char buf[64];
        snprintf(buf, sizeof(buf), "%.17g", d);
        std::string s(buf);
        if (s.find('.') == std::string::npos && s.find('e') == std::string::npos && s.find('E') == std::string::npos)
            s += ".0";
        return s;
    }

    if (value.isString())
        return escapeJsonString(value.toString());

    if (auto* arr = value.getArray())
    {
        std::string result = "[";
        for (int i = 0; i < arr->size(); ++i)
        {
            if (i > 0) result += ",";
            result += canonicalJson((*arr)[i]);
        }
        result += "]";
        return result;
    }

    if (auto* obj = value.getDynamicObject())
    {
        auto& props = obj->getProperties();
        std::vector<juce::Identifier> keys;
        for (auto& prop : props)
            keys.push_back(prop.name);
        std::sort(keys.begin(), keys.end(),
            [](const juce::Identifier& a, const juce::Identifier& b) {
                return a.toString() < b.toString();
            });

        std::string result = "{";
        for (size_t i = 0; i < keys.size(); ++i)
        {
            if (i > 0) result += ",";
            result += escapeJsonString(keys[i].toString()) + ":" + canonicalJson(props[keys[i]]);
        }
        result += "}";
        return result;
    }

    return "null";
}

std::optional<SignedEntitlementToken> parseSignedEntitlement(const juce::var& json)
{
    auto* obj = json.getDynamicObject();
    if (!obj) return std::nullopt;

    auto& props = obj->getProperties();

    auto algorithm = props["algorithm"].toString().toStdString();
    if (algorithm != "Ed25519") return std::nullopt;

    auto signature = props["signature"].toString().toStdString();
    if (signature.empty()) return std::nullopt;

    auto kid = props["kid"].toString().toStdString();
    if (kid.empty()) return std::nullopt;

    auto* payloadObj = props["payload"].getDynamicObject();
    if (!payloadObj) return std::nullopt;

    auto& payloadProps = payloadObj->getProperties();

    auto getStr = [&](const juce::Identifier& key) -> std::string {
        return payloadProps[key].toString().toStdString();
    };

    EntitlementClaims claims;
    claims.schemaVersion = static_cast<int>(payloadProps["schemaVersion"]);
    claims.licenseKeyHash = getStr("licenseKeyHash");
    claims.activationId = getStr("activationId");
    claims.machineId = getStr("machineId");
    claims.productId = getStr("productId");
    claims.variantId = getStr("variantId");
    claims.status = getStr("status");
    claims.issuedAt = getStr("issuedAt");
    claims.refreshAfter = getStr("refreshAfter");
    claims.expiresAt = getStr("expiresAt");
    claims.kid = getStr("kid");

    if (claims.licenseKeyHash.empty() || claims.activationId.empty() ||
        claims.machineId.empty() || claims.productId.empty())
        return std::nullopt;

    SignedEntitlementToken token;
    token.payload = std::move(claims);
    token.signature = std::move(signature);
    token.kid = std::move(kid);
    token.algorithm = std::move(algorithm);
    auto canonical = props["canonical"].toString().toStdString();
    if (!canonical.empty())
        token.canonical = std::move(canonical);
    else
        token.canonical = canonicalJson(claimsToVar(token.payload));

    return token;
}

SignedEntitlementToken parseSignedEntitlementOrThrow(const juce::var& json)
{
    auto result = parseSignedEntitlement(json);
    if (!result)
        throw std::runtime_error("Failed to parse signed entitlement token");
    return std::move(*result);
}

juce::var claimsToVar(const EntitlementClaims& claims)
{
    auto obj = new juce::DynamicObject();
    obj->setProperty("schemaVersion", claims.schemaVersion);
    obj->setProperty("licenseKeyHash", juce::String(claims.licenseKeyHash));
    obj->setProperty("activationId", juce::String(claims.activationId));
    obj->setProperty("machineId", juce::String(claims.machineId));
    obj->setProperty("productId", juce::String(claims.productId));
    obj->setProperty("variantId", juce::String(claims.variantId));
    obj->setProperty("status", juce::String(claims.status));
    obj->setProperty("issuedAt", juce::String(claims.issuedAt));
    obj->setProperty("refreshAfter", juce::String(claims.refreshAfter));
    obj->setProperty("expiresAt", juce::String(claims.expiresAt));
    obj->setProperty("kid", juce::String(claims.kid));
    return juce::var(obj);
}

} // namespace lumascope
