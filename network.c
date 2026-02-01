/*
 * network.c - Network interface for GegOS (mock implementation)
 */

#include "network.h"

/* Simple string comparison */
static int streq(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == *b;
}

/* Simple string copy */
static void strcpy_safe(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0;
}

/* Get string length */
static int strlen_safe(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

/* Mock WiFi networks (simulated) */
static wifi_network_t available_networks[] = {
    {"GegOS_Guest", 85, 0},
    {"HomeNetwork", 92, 1},
    {"Coffee_WiFi", 60, 1},
    {"TechCafe", 45, 1}
};
static int num_networks = 4;

/* Network state */
static net_status_t current_status = NET_DISCONNECTED;
static char connected_network[32] = "";
static char error_message[64] = "";
static int scanning = 0;

void network_init(void) {
    current_status = NET_DISCONNECTED;
    connected_network[0] = 0;
    error_message[0] = 0;
}

net_status_t network_get_status(void) {
    return current_status;
}

const char* network_get_status_string(void) {
    switch (current_status) {
        case NET_DISCONNECTED:
            return "No Connection";
        case NET_SCANNING:
            return "Scanning...";
        case NET_CONNECTED:
            return "Connected";
        case NET_ERROR:
            return "Connection Failed";
        default:
            return "Unknown";
    }
}

void network_scan_wifi(void) {
    scanning = 1;
    current_status = NET_SCANNING;
    error_message[0] = 0;
}

wifi_network_t* network_get_networks(int* out_count) {
    *out_count = num_networks;
    return available_networks;
}

void network_connect_wifi(const char* ssid, const char* password) {
    /* Validate password for mock networks */
    int valid = 0;
    
    if (streq(ssid, "HomeNetwork") && streq(password, "home123")) {
        valid = 1;
    } else if (streq(ssid, "Coffee_WiFi") && streq(password, "coffee")) {
        valid = 1;
    } else if (streq(ssid, "TechCafe") && streq(password, "tech2024")) {
        valid = 1;
    } else if (streq(ssid, "GegOS_Guest")) {
        /* No password required */
        valid = 1;
    } else if (strlen_safe(password) > 0) {
        /* Invalid password */
        strcpy_safe(error_message, "Password is incorrect", 64);
        current_status = NET_ERROR;
        return;
    }
    
    if (valid) {
        strcpy_safe(connected_network, ssid, 32);
        current_status = NET_CONNECTED;
        error_message[0] = 0;
    } else {
        strcpy_safe(error_message, "Password is incorrect", 64);
        current_status = NET_ERROR;
    }
}

void network_disconnect(void) {
    connected_network[0] = 0;
    current_status = NET_DISCONNECTED;
    error_message[0] = 0;
}

const char* network_get_connected_name(void) {
    return connected_network;
}

int network_is_connected(void) {
    return current_status == NET_CONNECTED;
}

const char* network_get_error(void) {
    return error_message;
}
