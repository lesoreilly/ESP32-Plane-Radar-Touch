#pragma once

class WiFiManager;

namespace services::ota {

using AdditionalRoutesFn = void (*)();

/**
 * Add an authenticated firmware page to the WiFiManager portal. The optional
 * callback runs while the portal server is being created, before WiFiManager
 * installs its default routes.
 */
void configure(WiFiManager& manager,
               AdditionalRoutesFn additional_routes = nullptr);

/** True while an OTA upload is actively writing flash. */
bool inProgress();

}  // namespace services::ota
