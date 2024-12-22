#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cJSON.h>
#include <MQTTAsync.h>

#include "utils.h"

#define CLIENTID    "Anomolies"
#define TOPIC_EVENTS       "eventstream"
#define QOS         1
#define TIMEOUT     1000L


MQTTAsync client;



void onDeliveryComplete(void* context, MQTTAsync_token token) {
    printf("Message with token value %d delivery confirmed\n", token);
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connect failed, rc %d\n", response ? response->code : 0);
}

void onConnect(void* context, MQTTAsync_successData* response) {
    printf("Connected successfully\n");
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_subscribe(client, "eventstream", QOS, NULL);
}


int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
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
            if (update_event_state(&event_data))
             {
                printf("Entity ID: %s\n", event_data.entity_id);
                printf("State Class: %s\n", event_data.state_class);
                printf("Device Class: %s\n", event_data.device_class);
                printf("State: %s\n", event_data.state);
                printf("Last Updated: %s\n", event_data.last_updated);
                send_json_ha_config_mqtt(client,event_data.entity_id,"homeassistant/sensor/anomaly/config");
             }


        } else {
            //printf("Failed to parse JSON\n");
        }
    }
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}


void onConnectionLost(void* context, char* cause) {
    printf("Connection lost, cause: %s\n", cause);
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

    int rc;
    while ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to reconnect, return code %d\n", rc);
        usleep(1000000L); // Sleep for 1 second before retrying
    }
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribed successfully\n");
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    printf("Subscribe failed, rc %d\n", response ? response->code : 0);
}




void send_json_ha_config_mqtt(MQTTAsync client, const char *unique, const char *topic) {
    // Replace "sensor" with "anomaly" in the unique string
    char modified_unique[256];

    snprintf(modified_unique, 255, "anom_%s", unique+7);
    // Create JSON object
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "~", "homeassistant/sensor/anomaly");
    cJSON_AddStringToObject(json, "name", modified_unique);
    cJSON_AddStringToObject(json, "cmd_t", "~/set");
    cJSON_AddStringToObject(json, "stat_t", "~/state");

    // Convert JSON object to string
    char *json_string = cJSON_PrintUnformatted(json);

#if 1
    // Create MQTT message
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = json_string;
    pubmsg.payloadlen = (int)strlen(json_string);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    // Publish the message asynchronously
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    opts.onSuccess = onDeliveryComplete;
    opts.context = client;

    int rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to publish message, return code %d\n", rc);
    } else {
    //    delivery_in_progress = 1;
        printf("Message '%s' sent to topic '%s'\n", json_string, topic);
    }
#endif

    // Clean up
    cJSON_Delete(json);
    free(json_string);
}







#if 0
void send_json_ha_config_mqtt(MQTTAsync client, const char *unique, const char *topic) {

    // Replace "sensor" with "anomaly" in the unique string
    char modified_unique[256];

     snprintf(modified_unique, 300, "anom_%s", unique+7);

    // Create JSON object
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "~", "homeassistant/sensor/anomaly");
    cJSON_AddStringToObject(json, "name", modified_unique);
    cJSON_AddStringToObject(json, "cmd_t", "~/set");
    cJSON_AddStringToObject(json, "stat_t", "~/state");

    // Convert JSON object to string
    char *json_string = cJSON_PrintUnformatted(json);

    // Create MQTT message
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = json_string;
    pubmsg.payloadlen = (int)strlen(json_string);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    // Publish the message asynchronously
    MQTTClient_deliveryToken token;
    MQTTClient_publishMessage(client, topic, &pubmsg, &token);

    printf("Message '%s' sent to topic '%s'\n", json_string, topic);

    // Clean up
    cJSON_Delete(json);
    free(json_string);
}
#endif

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


    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    MQTTAsync_create(&client, address, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTAsync_setCallbacks(client, client, onConnectionLost, messageArrived, onDeliveryComplete);


    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;
    conn_opts.username = username;
    conn_opts.password = password;

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        return EXIT_FAILURE;
    }
    



    // Keep the program running to receive messages
    while (1) {
        // Add a sleep to reduce CPU usage
        sleep(1);
    }

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_disconnect(client, &disc_opts);
    MQTTAsync_destroy(&client);
    return rc;
}

