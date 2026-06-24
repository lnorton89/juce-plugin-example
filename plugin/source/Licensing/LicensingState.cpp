#include "LumaScope/Licensing/LicensingState.h"

namespace lumascope {

const char* toString(LicenseStatus status) noexcept
{
    switch (status)
    {
        case LicenseStatus::uninitialized:          return "uninitialized";
        case LicenseStatus::not_activated:          return "not_activated";
        case LicenseStatus::activating:             return "activating";
        case LicenseStatus::activated:              return "activated";
        case LicenseStatus::offline_grace:          return "offline_grace";
        case LicenseStatus::revalidation_required:  return "revalidation_required";
        case LicenseStatus::revoked:                return "revoked";
        case LicenseStatus::corrupt:                return "corrupt";
        case LicenseStatus::service_unavailable:    return "service_unavailable";
        case LicenseStatus::deactivating:           return "deactivating";
        default:                                    return "unknown";
    }
}

bool isTransitionState(LicenseStatus status) noexcept
{
    return status == LicenseStatus::activating
        || status == LicenseStatus::deactivating;
}

bool isErrorState(LicenseStatus status) noexcept
{
    return status == LicenseStatus::revoked
        || status == LicenseStatus::corrupt;
}

} // namespace lumascope
