#include <stdio.h>
#include <time.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"

void convert_iso8601_to_tm(const char *iso8601, struct tm *result) {
    // Parse the date and time components from the ISO 8601 string
    sscanf(iso8601, "%4d-%2d-%2dT%2d:%2d:%2d", 
           &result->tm_year, &result->tm_mon, &result->tm_mday, 
           &result->tm_hour, &result->tm_min, &result->tm_sec);

    // Adjust the year and month values to match the struct tm format
    result->tm_year -= 1900; // struct tm year is years since 1900
    result->tm_mon -= 1;     // struct tm month is 0-11

    // Note: struct tm does not store microseconds or timezone offset
}


int parse_json(const char *json_str, EventData *event_data) {
    // Parse the JSON string
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return 0;
    }

    // Get the event_data object
    cJSON *event_data_obj = cJSON_GetObjectItemCaseSensitive(json, "event_data");
    if (!cJSON_IsObject(event_data_obj)) {
        fprintf(stderr, "Error: event_data is not an object\n");
        cJSON_Delete(json);
        return 0;
    }

    // Get the entity_id
    cJSON *entity_id = cJSON_GetObjectItemCaseSensitive(event_data_obj, "entity_id");
    if (cJSON_IsString(entity_id) && (entity_id->valuestring != NULL)) {
        strncpy(event_data->entity_id, entity_id->valuestring, sizeof(event_data->entity_id) - 1);
    }

    // Get the state from new_state
    cJSON *new_state = cJSON_GetObjectItemCaseSensitive(event_data_obj, "new_state");
    if (cJSON_IsObject(new_state)) {
        cJSON *state = cJSON_GetObjectItemCaseSensitive(new_state, "state");
        if (cJSON_IsString(state) && (state->valuestring != NULL)) {
            strncpy(event_data->state, state->valuestring, sizeof(event_data->state) - 1);
        }

        // Get the state_class and device_class
        cJSON *attributes = cJSON_GetObjectItemCaseSensitive(new_state, "attributes");
        if (cJSON_IsObject(attributes)) {
            cJSON *state_class = cJSON_GetObjectItemCaseSensitive(attributes, "state_class");
            if (cJSON_IsString(state_class) && (state_class->valuestring != NULL)) {
                strncpy(event_data->state_class, state_class->valuestring, sizeof(event_data->state_class) - 1);
            }

            cJSON *device_class = cJSON_GetObjectItemCaseSensitive(attributes, "device_class");
            if (cJSON_IsString(device_class) && (device_class->valuestring != NULL)) {
                strncpy(event_data->device_class, device_class->valuestring, sizeof(event_data->device_class) - 1);
            }
        }

        // Get the last_updated
        cJSON *last_updated = cJSON_GetObjectItemCaseSensitive(new_state, "last_updated");
        if (cJSON_IsString(last_updated) && (last_updated->valuestring != NULL)) {
            strncpy(event_data->last_updated, last_updated->valuestring, sizeof(event_data->last_updated) - 1);
        }
    }

    cJSON_Delete(json);
    return 1; // Success
}

int check_event_type(const char *json_str) {
    // Parse the JSON string
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        fprintf(stderr, "Error parsing JSON\n");
        return 0;
    }

    // Get the event_type field
    cJSON *event_type = cJSON_GetObjectItemCaseSensitive(json, "event_type");
    if (cJSON_IsString(event_type) && (event_type->valuestring != NULL)) {
        // Check if event_type is "state_changed"
        if (strcmp(event_type->valuestring, "state_changed") == 0) {
            // Get the state_class field from event_data -> new_state -> attributes
            cJSON *event_data = cJSON_GetObjectItemCaseSensitive(json, "event_data");
            if (event_data != NULL) {
                cJSON *new_state = cJSON_GetObjectItemCaseSensitive(event_data, "new_state");
                if (new_state != NULL) {
                    cJSON *attributes = cJSON_GetObjectItemCaseSensitive(new_state, "attributes");
                    if (attributes != NULL) {
                        cJSON *state_class = cJSON_GetObjectItemCaseSensitive(attributes, "state_class");
                        if (cJSON_IsString(state_class) && (state_class->valuestring != NULL)) {
                            // Check if state_class is "measurement"
                            if (strcmp(state_class->valuestring, "measurement") == 0) {
                                cJSON_Delete(json);
                                return 1; // Both conditions are met
                            }
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(json);
    return 0; // Conditions are not met
}



