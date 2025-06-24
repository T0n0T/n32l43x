/**
 * transport.h - Header for embedded transport state machine
 * with inter-transport event mechanism
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Transport status codes
typedef enum {
    TRANSPORT_OK,
    TRANSPORT_ERR_INVALID_ARG,
    TRANSPORT_ERR_MEMORY,
    TRANSPORT_ERR_INVALID_SIZE,
    TRANSPORT_ERR_BUSY,
    TRANSPORT_ERR_OVERSIZE,
    TRANSPORT_ERR_STATE,
    TRANSPORT_ERR_TX,
    TRANSPORT_ERR_RX_VALIDATION,
    TRANSPORT_ERR_TX_ACK,
    TRANSPORT_ERR_ACK_TIMEOUT,
    TRANSPORT_ERR_NOT_SUPPORTED,
    TRANSPORT_ERR_ALREADY_EXISTS
} transport_status_t;

// Transport state codes
typedef enum {
    TRANSPORT_STATE_IDLE,
    TRANSPORT_STATE_TX,
    TRANSPORT_STATE_RX,
    TRANSPORT_STATE_WAIT_ACK,
    TRANSPORT_STATE_ERROR
} transport_state_t;

// Event types
typedef enum {
    TRANSPORT_EVENT_NONE = 0,
    TRANSPORT_EVENT_DATA_RECEIVED,
    TRANSPORT_EVENT_DATA_SENT,
    TRANSPORT_EVENT_ERROR,
    TRANSPORT_EVENT_CONNECTION_ESTABLISHED,
    TRANSPORT_EVENT_CONNECTION_LOST,
    TRANSPORT_EVENT_CUSTOM_BASE = 0x80 // Custom events start here
} transport_event_type_t;

// Forward declarations
typedef struct transport_ctrl  transport_ctrl_t;
typedef struct transport_event transport_event_t;

// Callback types
typedef void (*transport_rx_cb_t)(const uint8_t* data, uint16_t len);
typedef void (*transport_tx_complete_cb_t)(transport_status_t status);
typedef void (*transport_error_cb_t)(transport_status_t error);
typedef transport_status_t (*transport_tx_fn_t)(const uint8_t* data, uint16_t len);
typedef transport_status_t (*transport_validate_rx_fn_t)(const uint8_t* data, uint16_t len);
typedef uint32_t (*transport_get_timestamp_fn_t)(void);
typedef void* (*transport_alloc_fn_t)(size_t size);
typedef void (*transport_free_fn_t)(void* ptr);
typedef void (*transport_event_cb_t)(transport_ctrl_t* receiver, const transport_event_t* event);

// Transport event structure
typedef struct transport_event {
    transport_ctrl_t*      sender;
    transport_event_type_t event_type;
    uint32_t               timestamp;
    uint8_t                data[16]; // Small payload for events
    uint8_t                data_len;
} transport_event_t;

// Event subscription structure
typedef struct {
    transport_event_type_t event_type;
    transport_event_cb_t   callback;
} transport_event_subscription_t;

// Transport configuration structure
typedef struct {
    uint16_t                     tx_buffer_size;
    uint16_t                     rx_buffer_size;
    uint16_t                     tx_chunk_size;
    bool                         ack_enabled;
    uint16_t                     ack_size;
    const uint8_t*               ack_pattern;
    uint32_t                     ack_timeout;
    uint8_t                      retry_count;
    uint8_t                      max_subscriptions; // Max event subscriptions this transport can have
    transport_rx_cb_t            rx_cb;
    transport_tx_complete_cb_t   tx_complete_cb;
    transport_error_cb_t         error_cb;
    transport_tx_fn_t            tx_fn;
    transport_validate_rx_fn_t   validate_rx_fn;
    transport_get_timestamp_fn_t get_timestamp_fn;
    transport_alloc_fn_t         alloc_fn;
    transport_free_fn_t          free_fn;
} transport_cfg_t;

// Transport control structure
// Transport control structure
struct transport_ctrl {
    transport_state_t               state;
    uint8_t*                        tx_buffer;
    uint16_t                        tx_len;
    uint16_t                        tx_pos;
    uint8_t*                        rx_buffer;
    uint16_t                        rx_len;
    uint16_t                        rx_pos;
    transport_cfg_t                 config;
    uint32_t                        timeout;
    uint32_t                        timestamp;
    bool                            ack_received;
    transport_event_subscription_t* subscriptions;
    uint8_t                         num_subscriptions;
    transport_event_t               pending_event;
    bool                            has_pending_event;
};

// Public API functions
transport_status_t transport_event_bus_init(uint8_t max_transports);
transport_status_t transport_init(transport_ctrl_t* ctrl, const transport_cfg_t* config);
void               transport_deinit(transport_ctrl_t* ctrl);
transport_status_t transport_register(transport_ctrl_t* ctrl);
void               transport_unregister(transport_ctrl_t* ctrl);
void               transport_process(transport_ctrl_t* ctrl);
transport_status_t transport_send(transport_ctrl_t* ctrl, const uint8_t* data, uint16_t len);
transport_status_t transport_notify_rx(transport_ctrl_t* ctrl, const uint8_t* data, uint16_t len);
transport_status_t transport_subscribe(transport_ctrl_t*      ctrl,
                                       transport_event_type_t event_type,
                                       transport_event_cb_t   callback);
void               transport_unsubscribe(transport_ctrl_t* ctrl, transport_event_type_t event_type);
transport_status_t transport_publish_event(transport_ctrl_t*      ctrl,
                                           transport_event_type_t event_type,
                                           const uint8_t*         data,
                                           uint16_t               len);
void               transport_reset(transport_ctrl_t* ctrl);

#ifdef __cplusplus
}
#endif

#endif // TRANSPORT_H