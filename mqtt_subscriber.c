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


volatile CONNECTED;
MQTTAsync client;



void onDeliveryComplete(void* context, MQTTAsync_token token) {
    printf("Message with token value %d delivery confirmed\n", token);
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connect failed, rc %d\n", response ? response->code : 0);
}

void onConnect(void* context, MQTTAsync_successData* response) {
    CONNECTED=1;
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
                printf("Measurement Unit: %s\n", event_data.unit_of_measurement);
                printf("Last Updated: %s\n", event_data.last_updated);
                //send_json_ha_config_mqtt(client,event_data.entity_id,"homeassistant/sensor/anomaly/config");
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

    CONNECTED=0;
}

void onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribed successfully\n");
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    printf("Subscribe failed, rc %d\n", response ? response->code : 0);
}


void send_json_ha_state_mqtt( const char *unique, int value) {
   send_json_ha_state_mqtt_internal(client,unique,value);
}

void send_json_ha_state_mqtt_internal(MQTTAsync client, const char *unique, int value) {
    // Remove "sensor." from the unique string
    char modified_unique[256];
    char value_char[256];

    snprintf(modified_unique, sizeof(modified_unique), "anomaly_%s", unique+7);

    // Construct the topic
    char topic[300];
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/state", modified_unique);

    // Create JSON object
    cJSON *json = cJSON_CreateObject();
    sprintf(value_char,"%d",value);
    cJSON_AddStringToObject(json, "state", value_char);

    // Convert JSON object to string
    char *json_string = cJSON_PrintUnformatted(json);

    // Create MQTT message
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = value_char;
    pubmsg.payloadlen = strlen(value_char);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

    // Publish the message asynchronously
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to publish message, return code %d\n", rc);
    } else {
        printf("Message '%s' sent to topic '%s'\n", value_char, topic);
    }

    // Clean up
    cJSON_Delete(json);
    free(json_string);
}


void send_json_ha_config_mqtt(const char *unique) {
     send_json_ha_config_mqtt_internal(client,unique);
}

void send_json_ha_config_mqtt_internal(MQTTAsync client, const char *unique) {
    // Replace "sensor" with "anomaly" in the unique string
    char modified_unique[256];
    char tag[256];
    char topic[256];
    char state[256];

    snprintf(tag, 255, "homeassistant/sensor/anomaly_%s", unique+7); 
    snprintf(modified_unique, 255, "anomaly_%s", unique+7);
    snprintf(topic, 255, "homeassistant/sensor/anomaly_%s/config", unique+7);
    snprintf(state, 255, "homeassistant/sensor/anomaly_%s/state", unique+7);

    // Create JSON object
    cJSON *json = cJSON_CreateObject();

#if 0
    cJSON_AddStringToObject(json, "~", tag);
    cJSON_AddStringToObject(json, "name", modified_unique);
    cJSON_AddStringToObject(json, "cmd_t", "~/set");
    cJSON_AddStringToObject(json, "stat_t", "~/state");
    cJSON_AddStringToObject(json, "schema", "json");
#endif

    cJSON_AddStringToObject(json, "name",modified_unique);
    //cJSON_AddStringToObject(json, "command_topic", "homeassistant/switch/irrigation/set");
    cJSON_AddStringToObject(json, "state_topic",state);
    cJSON_AddStringToObject(json, "unique_id",modified_unique);
    cJSON_AddStringToObject(json, "unit_of_measurement","%");

    cJSON *device = cJSON_CreateObject();
    cJSON_AddItemToObject(json, "device", device);

    cJSON *identifiers = cJSON_CreateArray();
    cJSON_AddItemToArray(identifiers, cJSON_CreateString(modified_unique));
    cJSON_AddItemToObject(device, "identifiers", identifiers);
    cJSON_AddStringToObject(device, "name",modified_unique);


    // Convert JSON object to string
    char *json_string = cJSON_PrintUnformatted(json);

    // Create MQTT message
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
    pubmsg.payload = json_string;
    pubmsg.payloadlen = (int)strlen(json_string);
    pubmsg.qos = QOS;
    pubmsg.retained = 1;

    // Publish the message asynchronously
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    //opts.onSuccess = onDeliveryComplete;
    opts.context = client;

    int rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
    if (rc != MQTTASYNC_SUCCESS) {
        printf("Failed to publish message, return code %d\n", rc);
    } else {
    //    delivery_in_progress = 1;
        printf("Message '%s' sent to topic '%s'\n", json_string, topic);
    }

    // Clean up
    cJSON_Delete(json);
    free(json_string);
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

    // Keep the program running to receive messages
    while (1) {

    if (CONNECTED == 0)
     { 
     if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
       
       }
     }

        sleep(10);
    }

    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    MQTTAsync_disconnect(client, &disc_opts);
    MQTTAsync_destroy(&client);
    return rc;
}

