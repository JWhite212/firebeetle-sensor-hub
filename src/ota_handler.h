// ota_handler.h — ArduinoOTA wrapper
#pragma once

namespace ota_handler {
    /// Initialise OTA with password from config.h.
    void init();

    /// Handle OTA requests. Call every loop().
    void update();
}
