#include <stdio.h>
#include <time.h>
#include <cJSON.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"



LIST_INIT(event_list); // create a linked list for the entity_id listing

void print_event_category(const EventCatagory* event) {
    if (event == NULL) {
        printf("Invalid event category.\n");
        return;
    }

    printf("Entity ID: %s -> ", event->entity_id);
    printf("(%d) ", event->delta_trigger);
    printf("Last State: %d\n", event->last_state);
    //printf("Last Timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
    //       event->last_timestamp.tm_year + 1900,
    //       event->last_timestamp.tm_mon + 1,
    //       event->last_timestamp.tm_mday,
    //       event->last_timestamp.tm_hour,
    //       event->last_timestamp.tm_min,
    //       event->last_timestamp.tm_sec);

    printf("\nWatts Integral (hourly):\n");
    for (int i = 0; i < 24; i++) {
        printf("[%.2f]", event->watts_integral[i]);
    }

    printf("\nCounter (hourly):\n");
    for (int i = 0; i < 24; i++) {
        printf("[%d]", event->counter[i]);
    }
   printf("\n\n");
}


void convert_iso8601_to_tm(const char* datetime_str, struct tm* tm_struct) {
    if (tm_struct == NULL) {
        perror("Invalid struct tm pointer");
        return;
    }

    // Initialize the struct to zero
    memset(tm_struct, 0, sizeof(struct tm));

    // Variables to hold the timezone offset
    int tz_hour = 0, tz_min = 0;
    char tz_sign = '+';

    // Parse the datetime string
    sscanf(datetime_str, "%4d-%2d-%2dT%2d:%2d:%2d.%*d%*d%*d%c%2d:%2d",
           &tm_struct->tm_year, &tm_struct->tm_mon, &tm_struct->tm_mday,
           &tm_struct->tm_hour, &tm_struct->tm_min, &tm_struct->tm_sec,
           &tz_sign, &tz_hour, &tz_min);

    // Adjust year and month to match struct tm expectations
    tm_struct->tm_year -= 1900; // tm_year is years since 1900
    tm_struct->tm_mon -= 1;     // tm_mon is 0-based

    // Calculate the timezone offset in seconds
    int tz_offset = (tz_hour * 3600) + (tz_min * 60);
    if (tz_sign == '-') {
        tz_offset = -tz_offset;
    }

    // Set the timezone offset in the tm struct (GNU extension)
    tm_struct->tm_gmtoff = tz_offset;
}

int get_hour(struct tm *time_struct) {
    return time_struct->tm_hour;
}


int parse_json(const char *json_str, EventData *event_data) {
    // Parse the JSON string
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        //fprintf(stderr, "Error parsing JSON\n");
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
        //fprintf(stderr, "Error parsing JSON\n");
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

    int i;
    for (int i = 0; i < 24; i++) {
        new_event->pvt->watts_integral[i]=0;
        new_event->pvt->counter[i]=0;
    }

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
                  current_event->pvt->counter[get_hour(&current_tm)]++;
                  print_event_category( current_event->pvt);
                  printf("PING!!! LARGE DELTA  %s -> (%d) (%d)\r\n",current_event->pvt->entity_id,current_event->pvt->last_state,state);
               }
             current_event->pvt->last_state = state;



            return 1;  // Update successful
        }
    }

    // Entity ID not found
    return 0;
}


int update_event_state(EventData *event_data) {
   return  update_event_state_internal(event_data,&event_list);
}
