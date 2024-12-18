#include <stdio.h>
#include <time.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"



LIST_INIT(event_list); // create a linked list for the entity_id listing


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

int get_hour(struct tm *time_struct) {
    return time_struct->tm_hour;
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

int add_unique_event_internal(char *entity_id, ll_t *list_head) {
    event_struct_t *current_event;

    // Iterate over the list to check for uniqueness
    list_for_each_entry(current_event, list_head, node) {
        if (strcmp(current_event->pvt->entity_id, entity_id) == 0) {
            // Entity ID is not unique
            return 0;
        }
    }

    // Entity ID is unique, create and add the new event
    event_struct_t *new_event = malloc(sizeof(event_struct_t));
    memset(new_event,0,sizeof(event_struct_t));

    new_event->pvt = malloc(sizeof(EventCatagory));
    strcpy(new_event->pvt->entity_id, entity_id);
    // Initialize other fields as needed
    new_event->pvt->delta_trigger = 100;  // Example initialization
    new_event->pvt->last_state = 0;

    list_add(&new_event->node, list_head);
    return 1;
}


int add_unique_event(char *entity_id) {
   return add_unique_event_internal(entity_id, &event_list);
}

int calculateDifference(int a, int b) {
    int difference = a - b;
    if (difference < 0) {
        difference = -difference;
    }
    return difference;
}


int update_event_state_internal(EventData *event_data, ll_t *list_head) {
    event_struct_t *current_event;
    struct tm current_tm;
    int state;

    // Iterate over the list to find the matching entity_id
    list_for_each_entry(current_event, list_head, node) {
        if (strcmp(current_event->pvt->entity_id, event_data->entity_id) == 0) {
            // Entity ID matches, update the last_state
            state = atoi(event_data->state);
            if (current_event->pvt->last_state == 0)   // Last state didnt have a reading... just set to current
                         current_event->pvt->last_state = state;

	    convert_iso8601_to_tm(event_data->last_updated,&current_tm);
             if (calculateDifference(current_event->pvt->last_state,state) > 100)
               {
                  printf("PING!!! LARGE DELTA (%d) (%d)\r\n",current_event->pvt->last_state,state);
               }




            return 1;  // Update successful
        }
    }

    // Entity ID not found
    return 0;
}


int update_event_state(EventData *event_data) {
   return  update_event_state_internal(event_data,&event_list);
}
