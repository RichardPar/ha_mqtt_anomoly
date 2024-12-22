#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cJSON.h>
#include "MQTTClient.h"

#include "utils.h"

#define CLIENTID    "Anomolies"
#define TOPIC       "#"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;



void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    if (check_event_type((char*)message->payload)) {
//        printf("Message arrived\n");
//        printf("     topic: %s\n", topicName);
//        printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);
        EventData event_data;
        if (parse_json(message->payload, &event_data)) {
            //printf("Entity ID: %s\n", event_data.entity_id);
            //printf("State Class: %s\n", event_data.state_class);
            //printf("Device Class: %s\n", event_data.device_class);
            //printf("State: %s\n", event_data.state);
            //printf("Last Updated: %s\n", event_data.last_updated);
            if (add_unique_event(event_data.entity_id))
             {
               
             }
            if (update_event_state(&event_data));
             {
                printf("Entity ID: %s\n", event_data.entity_id);
                printf("State Class: %s\n", event_data.state_class);
                printf("Device Class: %s\n", event_data.device_class);
                printf("State: %s\n", event_data.state);
                printf("Last Updated: %s\n", event_data.last_updated);
    
             }





        } else {
            //printf("Failed to parse JSON\n");
        }
    }
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
    MQTTClient *client = (MQTTClient *)context;
    int rc;

    while ((rc = MQTTClient_connect(*client, NULL)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to reconnect, return code %d\n", rc);
        sleep(1);
    }
    printf("Reconnected successfully\n");
}

void connlost_2(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    char *address = NULL;
    char *username = NULL;
    char *password = NULL;
    int opt;

    while ((opt = getopt(argc, argv, "u:p:m:")) != -1) {
        switch (opt) {
            case 'u':
                username = optarg;
                break;
            case 'p':
                password = optarg;
                break;
            case 'm':
                address = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -u username -p password -m mqttserver\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!address || !username || !password) {
        fprintf(stderr, "Usage: %s -u username -p password -m mqttserver\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;

    char full_address[256];
    snprintf(full_address, sizeof(full_address), "tcp://%s:1883", address);

    MQTTClient_create(&client, full_address, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.username = username;
    conn_opts.password = password;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    // Keep the program running to receive messages
    while (1) {
        // Add a sleep to reduce CPU usage
        sleep(1);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

