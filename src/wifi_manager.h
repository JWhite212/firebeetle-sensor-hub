// wifi_manager.h
#pragma once

namespace wifi_manager {
    /// Initialise WiFi in station mode and begin connection.
    void init();

    /// Handle reconnection logic. Call every loop() iteration.
    void update();

    /// Returns true when WiFi is connected and has an IP address.
    bool is_ready();

    /// Get the current IP address as a string.
    const char* get_ip();

    /// Get WiFi signal strength in dBm.
    int get_rssi();
}
