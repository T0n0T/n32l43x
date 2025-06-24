/**
 * transport.c - State machine for data transmission in embedded devices
 * with inter-transport event mechanism
 */

#include "transport.h"
#include <string.h>
#include <stdbool.h>

// Event bus structure (singleton)
typedef struct {
    transport_ctrl_t** transports;
    uint8_t            num_transports;
} transport_event_bus_t;

static transport_event_bus_t event_bus = {0};

// Private function prototypes
static void process_tx(transport_ctrl_t* ctrl);
static void process_rx(transport_ctrl_t* ctrl);
static void handle_timeout(transport_ctrl_t* ctrl);
static void reset_state_machine(transport_ctrl_t* ctrl);
static void deliver_event(transport_ctrl_t* receiver, const transport_event_t* event);
static void process_pending_events(transport_ctrl_t* ctrl);

/**
 * @brief Initialize the transport event bus
 *
 * @param max_transports Maximum number of transports that will be registered
 * @return transport_status_t Initialization status
 */
transport_status_t transport_event_bus_init(uint8_t max_transports)
{
    if (max_transports == 0) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    if (event_bus.transports != NULL) {
        // Already initialized
        return TRANSPORT_OK;
    }

    event_bus.transports = malloc(max_transports * sizeof(transport_ctrl_t*));
    if (event_bus.transports == NULL) {
        return TRANSPORT_ERR_MEMORY;
    }

    memset(event_bus.transports, 0, max_transports * sizeof(transport_ctrl_t*));
    event_bus.num_transports = max_transports;

    return TRANSPORT_OK;
}

/**
 * @brief Register a transport with the event bus
 *
 * @param ctrl Transport control structure to register
 * @return transport_status_t Registration status
 */
transport_status_t transport_register(transport_ctrl_t* ctrl)
{
    if (ctrl == NULL || event_bus.transports == NULL) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    for (uint8_t i = 0; i < event_bus.num_transports; i++) {
        if (event_bus.transports[i] == NULL) {
            event_bus.transports[i] = ctrl;
            return TRANSPORT_OK;
        }
    }

    return TRANSPORT_ERR_BUSY;
}

/**
 * @brief Unregister a transport from the event bus
 *
 * @param ctrl Transport control structure to unregister
 */
void transport_unregister(transport_ctrl_t* ctrl)
{
    if (ctrl == NULL || event_bus.transports == NULL) {
        return;
    }

    for (uint8_t i = 0; i < event_bus.num_transports; i++) {
        if (event_bus.transports[i] == ctrl) {
            event_bus.transports[i] = NULL;
            break;
        }
    }
}

/**
 * @brief Initialize the transport state machine
 *
 * @param ctrl Pointer to transport control structure
 * @param config Configuration parameters
 * @return transport_status_t Initialization status
 */
transport_status_t transport_init(transport_ctrl_t* ctrl, const transport_cfg_t* config)
{
    if (ctrl == NULL || config == NULL) {
        return TRANSPORT_ERR_INVALID_ARG;
    }
    memset(ctrl, 0, sizeof(struct transport_ctrl));
    if (config->tx_buffer_size == 0 || config->rx_buffer_size == 0) {
        return TRANSPORT_ERR_INVALID_SIZE;
    }

    // Allocate buffers
    ctrl->tx_buffer = config->alloc_fn(config->tx_buffer_size);
    ctrl->rx_buffer = config->alloc_fn(config->rx_buffer_size);

    if (ctrl->tx_buffer == NULL || ctrl->rx_buffer == NULL) {
        // Free any allocated buffer if one succeeded
        if (ctrl->tx_buffer) config->free_fn(ctrl->tx_buffer);
        if (ctrl->rx_buffer) config->free_fn(ctrl->rx_buffer);
        return TRANSPORT_ERR_MEMORY;
    }

    // Initialize control structure
    ctrl->config = *config;
    reset_state_machine(ctrl);

    // Initialize subscriptions
    if (config->max_subscriptions > 0) {
        ctrl->subscriptions = config->alloc_fn(config->max_subscriptions *
                                               sizeof(transport_event_subscription_t));
        if (ctrl->subscriptions == NULL) {
            config->free_fn(ctrl->tx_buffer);
            config->free_fn(ctrl->rx_buffer);
            return TRANSPORT_ERR_MEMORY;
        }
        memset(ctrl->subscriptions, 0,
               config->max_subscriptions * sizeof(transport_event_subscription_t));
    }

    // Register with event bus
    transport_register(ctrl);

    return TRANSPORT_OK;
}

/**
 * @brief Subscribe to events from another transport
 *
 * @param ctrl Pointer to transport control structure
 * @param event_type Type of event to subscribe to
 * @param callback Callback to invoke when event is received
 * @return transport_status_t Subscription status
 */
transport_status_t transport_subscribe(transport_ctrl_t*      ctrl,
                                       transport_event_type_t event_type,
                                       transport_event_cb_t   callback)
{
    if (ctrl == NULL || callback == NULL) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    if (ctrl->subscriptions == NULL) {
        return TRANSPORT_ERR_NOT_SUPPORTED;
    }

    // Check if already subscribed
    for (uint8_t i = 0; i < ctrl->config.max_subscriptions; i++) {
        if (ctrl->subscriptions[i].event_type == event_type) {
            return TRANSPORT_ERR_ALREADY_EXISTS;
        }
    }

    // Find empty slot
    for (uint8_t i = 0; i < ctrl->config.max_subscriptions; i++) {
        if (ctrl->subscriptions[i].callback == NULL) {
            ctrl->subscriptions[i].event_type = event_type;
            ctrl->subscriptions[i].callback   = callback;
            ctrl->num_subscriptions++;
            return TRANSPORT_OK;
        }
    }

    return TRANSPORT_ERR_BUSY;
}

/**
 * @brief Unsubscribe from events
 *
 * @param ctrl Pointer to transport control structure
 * @param event_type Type of event to unsubscribe from
 */
void transport_unsubscribe(transport_ctrl_t* ctrl, transport_event_type_t event_type)
{
    if (ctrl == NULL || ctrl->subscriptions == NULL) {
        return;
    }

    for (uint8_t i = 0; i < ctrl->config.max_subscriptions; i++) {
        if (ctrl->subscriptions[i].event_type == event_type) {
            ctrl->subscriptions[i].callback   = NULL;
            ctrl->subscriptions[i].event_type = TRANSPORT_EVENT_NONE;
            ctrl->num_subscriptions--;
            break;
        }
    }
}

/**
 * @brief Publish an event to other transports
 *
 * @param ctrl Pointer to transport control structure (sender)
 * @param event_type Type of event to publish
 * @param data Event data (can be NULL)
 * @param len Length of event data (0 if no data)
 * @return transport_status_t Publish status
 */
transport_status_t transport_publish_event(transport_ctrl_t*      ctrl,
                                           transport_event_type_t event_type,
                                           const uint8_t*         data,
                                           uint16_t               len)
{
    if (ctrl == NULL || event_bus.transports == NULL) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    if (len > 0 && data == NULL) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    // Create event
    transport_event_t event = {
        .sender     = ctrl,
        .event_type = event_type,
        .timestamp  = ctrl->config.get_timestamp_fn()};

    // Copy data if provided
    if (len > 0 && len <= sizeof(event.data)) {
        memcpy(event.data, data, len);
        event.data_len = len;
    } else {
        event.data_len = 0;
    }

    // Deliver event to all transports except sender
    for (uint8_t i = 0; i < event_bus.num_transports; i++) {
        if (event_bus.transports[i] != NULL &&
            event_bus.transports[i] != ctrl) {
            deliver_event(event_bus.transports[i], &event);
        }
    }

    return TRANSPORT_OK;
}

/**
 * @brief Process the transport state machine including pending events
 *
 * @param ctrl Pointer to transport control structure
 */
void transport_process(transport_ctrl_t* ctrl)
{
    if (ctrl == NULL) return;

    // First process any pending events
    process_pending_events(ctrl);

    // Check for timeout conditions
    if (ctrl->timeout != 0 &&
        (ctrl->config.get_timestamp_fn() - ctrl->timestamp) > ctrl->timeout) {
        handle_timeout(ctrl);
    }

    // State machine processing
    switch (ctrl->state) {
        case TRANSPORT_STATE_IDLE:
            // Nothing to do in idle state
            break;

        case TRANSPORT_STATE_TX:
            process_tx(ctrl);
            break;

        case TRANSPORT_STATE_RX:
            process_rx(ctrl);
            break;

        case TRANSPORT_STATE_WAIT_ACK:
            // Waiting for ACK - timeout handled above
            break;

        case TRANSPORT_STATE_ERROR:
            // Error state - requires external reset
            break;

        default:
            ctrl->state = TRANSPORT_STATE_ERROR;
            break;
    }
}

/**
 * @brief Send data through the transport layer
 *
 * @param ctrl Pointer to transport control structure
 * @param data Pointer to data to send
 * @param len Length of data to send
 * @return transport_status_t Send operation status
 */
transport_status_t transport_send(transport_ctrl_t* ctrl, const uint8_t* data, uint16_t len)
{
    if (ctrl == NULL || data == NULL || len == 0) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    if (ctrl->state != TRANSPORT_STATE_IDLE) {
        return TRANSPORT_ERR_BUSY;
    }

    if (len > ctrl->config.tx_buffer_size) {
        return TRANSPORT_ERR_OVERSIZE;
    }

    // Copy data to TX buffer
    memcpy(ctrl->tx_buffer, data, len);
    ctrl->tx_len = len;
    ctrl->tx_pos = 0;

    // Start transmission
    ctrl->state     = TRANSPORT_STATE_TX;
    ctrl->timestamp = ctrl->config.get_timestamp_fn();

    return TRANSPORT_OK;
}

/**
 * @brief Notify the transport layer of received data
 *
 * @param ctrl Pointer to transport control structure
 * @param data Pointer to received data
 * @param len Length of received data
 * @return transport_status_t Receive operation status
 */
transport_status_t transport_notify_rx(transport_ctrl_t* ctrl, const uint8_t* data, uint16_t len)
{
    if (ctrl == NULL || data == NULL || len == 0) {
        return TRANSPORT_ERR_INVALID_ARG;
    }

    if (ctrl->state == TRANSPORT_STATE_ERROR) {
        return TRANSPORT_ERR_STATE;
    }

    // Check if we're expecting an ACK
    if (ctrl->state == TRANSPORT_STATE_WAIT_ACK) {
        if (len == ctrl->config.ack_size &&
            memcmp(data, ctrl->config.ack_pattern, ctrl->config.ack_size) == 0) {
            ctrl->ack_received = true;
            ctrl->state        = TRANSPORT_STATE_IDLE;
            if (ctrl->config.tx_complete_cb) {
                ctrl->config.tx_complete_cb(TRANSPORT_OK);
            }
            return TRANSPORT_OK;
        }
    }

    // Handle normal data reception
    if (len > ctrl->config.rx_buffer_size) {
        return TRANSPORT_ERR_OVERSIZE;
    }

    // Copy data to RX buffer
    memcpy(ctrl->rx_buffer, data, len);
    ctrl->rx_len = len;
    ctrl->rx_pos = 0;

    // Process received data
    ctrl->state = TRANSPORT_STATE_RX;

    return TRANSPORT_OK;
}

/**
 * @brief Process transmission state
 *
 * @param ctrl Pointer to transport control structure
 */
static void process_tx(transport_ctrl_t* ctrl)
{
    uint16_t remaining  = ctrl->tx_len - ctrl->tx_pos;
    uint16_t chunk_size = (remaining > ctrl->config.tx_chunk_size) ? ctrl->config.tx_chunk_size : remaining;

    // Send data chunk
    transport_status_t status = ctrl->config.tx_fn(
        &ctrl->tx_buffer[ctrl->tx_pos],
        chunk_size);

    if (status != TRANSPORT_OK) {
        ctrl->state = TRANSPORT_STATE_ERROR;
        if (ctrl->config.error_cb) {
            ctrl->config.error_cb(TRANSPORT_ERR_TX);
        }
        return;
    }

    ctrl->tx_pos += chunk_size;

    // Check if transmission is complete
    if (ctrl->tx_pos >= ctrl->tx_len) {
        if (ctrl->config.ack_enabled) {
            ctrl->state        = TRANSPORT_STATE_WAIT_ACK;
            ctrl->ack_received = false;
            ctrl->timeout      = ctrl->config.ack_timeout;
            ctrl->timestamp    = ctrl->config.get_timestamp_fn();
        } else {
            ctrl->state = TRANSPORT_STATE_IDLE;
            if (ctrl->config.tx_complete_cb) {
                ctrl->config.tx_complete_cb(TRANSPORT_OK);
            }
        }
    }
}

/**
 * @brief Process reception state
 *
 * @param ctrl Pointer to transport control structure
 */
static void process_rx(transport_ctrl_t* ctrl)
{
    // Validate data if validation function is provided
    if (ctrl->config.validate_rx_fn) {
        transport_status_t status = ctrl->config.validate_rx_fn(
            ctrl->rx_buffer,
            ctrl->rx_len);

        if (status != TRANSPORT_OK) {
            ctrl->state = TRANSPORT_STATE_ERROR;
            if (ctrl->config.error_cb) {
                ctrl->config.error_cb(TRANSPORT_ERR_RX_VALIDATION);
            }
            return;
        }
    }

    // Send ACK if required
    if (ctrl->config.ack_enabled) {
        transport_status_t status = ctrl->config.tx_fn(
            ctrl->config.ack_pattern,
            ctrl->config.ack_size);

        if (status != TRANSPORT_OK) {
            ctrl->state = TRANSPORT_STATE_ERROR;
            if (ctrl->config.error_cb) {
                ctrl->config.error_cb(TRANSPORT_ERR_TX_ACK);
            }
            return;
        }
    }

    // Notify application of received data
    if (ctrl->config.rx_cb) {
        ctrl->config.rx_cb(ctrl->rx_buffer, ctrl->rx_len);
    }

    // Return to idle state
    ctrl->state = TRANSPORT_STATE_IDLE;
}

/**
 * @brief Handle timeout conditions
 *
 * @param ctrl Pointer to transport control structure
 */
static void handle_timeout(transport_ctrl_t* ctrl)
{
    ctrl->timeout = 0; // Clear timeout

    if (ctrl->state == TRANSPORT_STATE_WAIT_ACK && !ctrl->ack_received) {
        if (ctrl->config.retry_count > 0) {
            ctrl->config.retry_count--;
            ctrl->state  = TRANSPORT_STATE_TX;
            ctrl->tx_pos = 0; // Reset transmission position for retry
        } else {
            ctrl->state = TRANSPORT_STATE_ERROR;
            if (ctrl->config.error_cb) {
                ctrl->config.error_cb(TRANSPORT_ERR_ACK_TIMEOUT);
            }
        }
    }
}

/**
 * @brief Reset the state machine to idle state
 *
 * @param ctrl Pointer to transport control structure
 */
static void reset_state_machine(transport_ctrl_t* ctrl)
{
    ctrl->state        = TRANSPORT_STATE_IDLE;
    ctrl->tx_len       = 0;
    ctrl->tx_pos       = 0;
    ctrl->rx_len       = 0;
    ctrl->rx_pos       = 0;
    ctrl->timeout      = 0;
    ctrl->ack_received = false;
}

/**
 * @brief Deliver an event to a transport instance
 *
 * @param receiver Transport to receive the event
 * @param event Event to deliver
 */
static void deliver_event(transport_ctrl_t* receiver, const transport_event_t* event)
{
    if (receiver == NULL || event == NULL) {
        return;
    }

    // Check if the receiver has any matching subscriptions
    bool has_subscription = false;
    for (uint8_t i = 0; i < receiver->config.max_subscriptions; i++) {
        if (receiver->subscriptions[i].event_type == event->event_type &&
            receiver->subscriptions[i].callback != NULL) {
            has_subscription = true;
            break;
        }
    }

    if (!has_subscription) {
        return;
    }

    // Store the event for processing in the transport's context
    receiver->pending_event     = *event;
    receiver->has_pending_event = true;
}

/**
 * @brief Process any pending events for a transport
 *
 * @param ctrl Transport control structure
 */
static void process_pending_events(transport_ctrl_t* ctrl)
{
    if (!ctrl->has_pending_event) {
        return;
    }

    // Make local copy and clear pending flag
    transport_event_t event = ctrl->pending_event;
    ctrl->has_pending_event = false;

    // Invoke matching callbacks
    for (uint8_t i = 0; i < ctrl->config.max_subscriptions; i++) {
        if (ctrl->subscriptions[i].event_type == event.event_type &&
            ctrl->subscriptions[i].callback != NULL) {
            ctrl->subscriptions[i].callback(ctrl, &event);
        }
    }
}