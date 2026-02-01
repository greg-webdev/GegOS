/*
 * network.h - Network interface for GegOS
 * Simplified mock network with WiFi simulation
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>

/* Network status */
typedef enum {
    NET_DISCONNECTED,
    NET_SCANNING,
    NET_CONNECTED,
    NET_ERROR
} net_status_t;

/* WiFi network info */
typedef struct {
    char ssid[32];
    int signal_strength;
    int requires_password;
} wifi_network_t;

/* Initialize network subsystem */
void network_init(void);

/* Get network status */
net_status_t network_get_status(void);

/* Get status string */
const char* network_get_status_string(void);

/* Scan for WiFi networks */
void network_scan_wifi(void);

/* Get available networks */
wifi_network_t* network_get_networks(int* out_count);

/* Connect to WiFi network */
void network_connect_wifi(const char* ssid, const char* password);

/* Disconnect from network */
void network_disconnect(void);

/* Get connected network name */
const char* network_get_connected_name(void);

/* Check if connection succeeded */
int network_is_connected(void);

/* Get error message */
const char* network_get_error(void);

#endif /* NETWORK_H */
