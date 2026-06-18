#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mosquitto.h>

#define DEV_PATH    "/dev/parking"

#define TOPIC_SLOT      "parking/rpi1/slot"
#define TOPIC_DISTANCE  "parking/rpi1/distance"
#define TOPIC_LOT       "parking/rpi1/lot"

#define MQTT_QOS    1
#define MQTT_RETAIN 1

static volatile int running = 1;

static void sig_handler(int sig) { (void)sig; running = 0; }

int main(int argc, char *argv[])
{
    const char *broker = (argc >= 2) ? argv[1] : "192.168.4.1";
    int port           = (argc >= 3) ? atoi(argv[2]) : 1883;

    int fd;
    struct mosquitto *mosq;
    char buf[64];
    ssize_t n;
    int s1, s2, d1, d2;
    char payload[128];
    int ret;

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);

    // init mqtt
    mosquitto_lib_init();
    mosq = mosquitto_new("parking_rpi1", true, NULL);
    if (!mosq) {
        fprintf(stderr, "mosquitto_new failed\n");
        return 1;
    }

    ret = mosquitto_connect(mosq, broker, port, 60);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "connect failed: %s\n", mosquitto_strerror(ret));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    printf("parking_mqtt_daemon: connected to %s:%d\n", broker, port);

    // 백그라운드 MQTT 루프 스레드 시작 (자동 Keep Alive 및 재연결 처리)
    ret = mosquitto_loop_start(mosq);
    if (ret != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "mosquitto_loop_start failed: %s\n", mosquitto_strerror(ret));
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    fd = open(DEV_PATH, O_RDONLY);
    if (fd < 0) {
        perror("open " DEV_PATH);
        mosquitto_loop_stop(mosq, true);
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }
    printf("parking_mqtt_daemon: watching %s\n", DEV_PATH);

    while (running) {
        n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        // parse: "Slot1:X Slot2:Y\nDistance1:A Distance2:B\n" */
        if (sscanf(buf, "Slot1:%d Slot2:%d\nDistance1:%d Distance2:%d", &s1, &s2, &d1, &d2) != 4) {
            fprintf(stderr, "bad format: %s\n", buf);
            continue;
        }
        printf("parsed: Slot1=%d Slot2=%d\nDistance1=%dcm Distance2=%dcm\n", s1, s2, d1, d2);

        // --- parking/rpi1/slot ---
        snprintf(payload, sizeof(payload),
                 "{\"slot1\":\"%s\",\"slot2\":\"%s\"}",
                 s1 ? "OCCUPIED" : "EMPTY",
                 s2 ? "OCCUPIED" : "EMPTY");
        mosquitto_publish(mosq, NULL, TOPIC_SLOT,
                          strlen(payload), payload, MQTT_QOS, MQTT_RETAIN);

        // --- parking/rpi1/distance ---
        snprintf(payload, sizeof(payload),
                 "{\"slot1\":%d,\"slot2\":%d,\"unit\":\"cm\"}",
                 d1, d2);
        mosquitto_publish(mosq, NULL, TOPIC_DISTANCE,
                          strlen(payload), payload, MQTT_QOS, MQTT_RETAIN);

        // --- parking/rpi1/lot ---
        snprintf(payload, sizeof(payload),
                 "%s", (s1 && s2) ? "FULL" : "AVAILABLE");
        mosquitto_publish(mosq, NULL, TOPIC_LOT,
                          strlen(payload), payload, MQTT_QOS, MQTT_RETAIN);
    }

    printf("parking_mqtt_daemon: exiting\n");
    close(fd);
    
    // 백그라운드 루프 스레드 중지 및 연결 정리
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
