#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sysrepo.h>

static int module_change_cb(sr_session_ctx_t *session, uint32_t sub_id,
                            const char *module_name, const char *xpath,
                            sr_event_t event, uint32_t request_id,
                            void *private_data) {
    printf("Hello World! Change detected in module: %s\n", module_name);
    return SR_ERR_OK;
}

int main() {
    sr_conn_ctx_t *connection = NULL;
    sr_session_ctx_t *session = NULL;
    sr_subscription_ctx_t *subscription = NULL;

    /* Connect to Sysrepo */
    if (sr_connect(0, &connection) != SR_ERR_OK) {
        fprintf(stderr, "Failed to connect to Sysrepo.\n");
        return EXIT_FAILURE;
    }

    /* Start a session */
    if (sr_session_start(connection, SR_DS_RUNNING, &session) != SR_ERR_OK) {
        fprintf(stderr, "Failed to start Sysrepo session.\n");
        sr_disconnect(connection);
        return EXIT_FAILURE;
    }

    /* Subscribe to changes in example-module */
    if (sr_module_change_subscribe(session, "example-module", NULL,
                                   module_change_cb, NULL, 0,
                                   SR_SUBSCR_DEFAULT, &subscription) != SR_ERR_OK) {
        fprintf(stderr, "Failed to subscribe to module changes.\n");
        sr_session_stop(session);
        sr_disconnect(connection);
        return EXIT_FAILURE;
        }

    printf("Subscribed to changes in example-module. Waiting for events...\n");
    while (1) {
        sleep(1);
    }

    /* Cleanup */
    sr_disconnect(connection);
    return EXIT_SUCCESS;
}
