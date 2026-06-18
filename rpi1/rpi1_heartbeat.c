#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mosquitto.h>

#define DEFAULT_NODE "rpi1"
#define DEFAULT_BROKER "192.168.4.1"
#define DEFAULT_PORT 1883
#define DEFAULT_INTERVAL_SEC 2
#define MQTT_QOS 1
#define MQTT_RETAIN false

static volatile sig_atomic_t running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[])
{
    const char *node = (argc >= 2) ? argv[1] : DEFAULT_NODE;
    const char *broker = (argc >= 3) ? argv[2] : DEFAULT_BROKER;
    int port = (argc >= 4) ? atoi(argv[3]) : DEFAULT_PORT;
    int interval_sec = (argc >= 5) ? atoi(argv[4]) : DEFAULT_INTERVAL_SEC;
    struct mosquitto *mosq;
    char topic[128];
    char online_payload[128];
    char offline_payload[128];
    int rc;

    if (interval_sec <= 0)
        interval_sec = DEFAULT_INTERVAL_SEC;

    snprintf(topic, sizeof(topic), "parking/%s/heartbeat", node);
    snprintf(online_payload, sizeof(online_payload),
             "{\"node\":\"%s\",\"status\":\"online\"}", node);
    snprintf(offline_payload, sizeof(offline_payload),
             "{\"node\":\"%s\",\"status\":\"offline\"}", node);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        fprintf(stderr, "mosquitto_new failed\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_will_set(mosq, topic, (int)strlen(offline_payload),
                       offline_payload, MQTT_QOS, MQTT_RETAIN);

    rc = mosquitto_connect(mosq, broker, port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "connect failed: %s\n", mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_loop_start(mosq);
    printf("heartbeat: %s -> %s:%d every %d sec\n",
           topic, broker, port, interval_sec);

    while (running) {
        rc = mosquitto_publish(mosq, NULL, topic, (int)strlen(online_payload),
                               online_payload, MQTT_QOS, MQTT_RETAIN);
        if (rc != MOSQ_ERR_SUCCESS)
            fprintf(stderr, "publish failed: %s\n", mosquitto_strerror(rc));

        sleep(interval_sec);
    }

    mosquitto_publish(mosq, NULL, topic, (int)strlen(offline_payload),
                      offline_payload, MQTT_QOS, MQTT_RETAIN);
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
