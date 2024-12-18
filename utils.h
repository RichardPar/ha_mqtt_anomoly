#include <time.h>

typedef struct {
    char entity_id[256];
    char state_class[256];
    char device_class[256];
    char state[256];
    char last_updated[256];
} EventData;

typedef struct {
  char   entity_id[256];
  int    delta_trigger;
  int    last_state;
  struct tm last_timestamp; 
  float  watts_integral[24];   // one her hour
  int    counter[24]; // one per hour
  struct EventCatagory *next;
} EventCatagory;


void convert_iso8601_to_tm(const char *iso8601, struct tm *result);
int check_event_type(const char *json_str);
int parse_json(const char *json_str, EventData *event_data);
