#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>  // Required for size_t
#include <sysrepo.h>

// Callback function for handling temperature changes
static int oven_temperature_change_cb(sr_session_ctx_t *session, uint32_t sub_id, const char *module_name,
                                      const char *xpath, sr_event_t event, uint32_t request_id, void *private_ctx) {
    sr_change_iter_t *iter = NULL;
    sr_change_oper_t op;
    sr_val_t *old_val = NULL, *new_val = NULL;
    int rc;

    printf("\n ^=^t  [EVENT] Temperature Change Detected in %s  ^=^t \n", module_name);

    // Get changes for the specified xpath
    rc = sr_get_changes_iter(session, "/oven:oven/temperature", &iter);
    if (rc != SR_ERR_OK) {
        printf("Error: Failed to get changes iterator\n");
        return SR_ERR_OPERATION_FAILED;
    }

    // Iterate through changes
    while ((rc = sr_get_change_next(session, iter, &op, &old_val, &new_val)) == SR_ERR_OK) {
        if (new_val) {
            printf(" ^=^t^d Path: %s\n", new_val->xpath);
            printf("    ^~   ^o New Value: %d  C\n", new_val->data.int32_val);

            // Trigger an alert if temperature is above 300
            if (strstr(new_val->xpath, "temperature") && new_val->data.int32_val > 300) {
                printf(" ^z   ^o WARNING: Oven temperature is too high! Cooling system activated.  ^z   ^o\n");
            }
        }

        // Free old and new values
        sr_free_val(old_val);
        sr_free_val(new_val);
    }


    sr_free_change_iter(iter);
    return SR_ERR_OK;
}

int main() {
    sr_conn_ctx_t *conn = NULL;
    sr_session_ctx_t *session = NULL;
    sr_subscription_ctx_t *subscription = NULL;
    int rc;

    printf(" ^=^t^m Attempting to connect to Sysrepo...\n");

    // Connect to Sysrepo
    rc = sr_connect(0, &conn);
    if (rc != SR_ERR_OK) {
        printf(" ^}^l Error: Failed to connect to Sysrepo (Error Code: %d)\n", rc);
        return rc;
    }

    // Start a session
    rc = sr_session_start(conn, SR_DS_RUNNING, &session);
    if (rc != SR_ERR_OK) {
        printf(" ^}^l Error: Failed to start Sysrepo session\n");
        sr_disconnect(conn);
        return rc;
    }

    // Subscribe to temperature changes
    printf(" ^=^t^m Subscribing to /oven:oven/temperature changes...\n");
    rc = sr_module_change_subscribe(session, "oven", NULL, oven_temperature_change_cb, NULL, 0, SR_SUBSCR_DEFAULT, &subscription);
    if (rc != SR_ERR_OK) {
        printf(" ^}^l Error: Failed to subscribe to changes\n");
        sr_session_stop(session);
        sr_disconnect(conn);
        return rc;
    }

    printf(" ^|^e Waiting for temperature change events... (Press Ctrl+C to exit)\n");

    // Keep running to listen for events
    while (1) {
        sleep(1);
    }
// Cleanup (never reaches here in normal execution)
    sr_unsubscribe(subscription);
    sr_session_stop(session);
    sr_disconnect(conn);

    return 0;
}


