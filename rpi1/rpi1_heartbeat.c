#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <mosquitto.h>

static volatile sig_atomic_t running = 1;

static void handle_signal(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    struct mosquitto *mosq;
    const char *topic = "parking/rpi1/heartbeat";
    const char *payload = "{\"node\":\"rpi1\",\"status\":\"online\"}";

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    mosquitto_lib_init();
    mosq = mosquitto_new(NULL, true, NULL);
    if (!mosq) {
        mosquitto_lib_cleanup();
        return 1;
    }

    if (mosquitto_connect(mosq, "192.168.4.1", 1883, 60) != MOSQ_ERR_SUCCESS) {
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    while (running) {
        mosquitto_publish(mosq, NULL, topic,
                          strlen(payload), payload, 1, false);
        sleep(2);
    }

    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    return 0;
}
