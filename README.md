HomeAssistant Energy Anomaly detect

Uses HA EVENTSTREAM and MQTT

The aim was to decect how many times the energy for a device has changed by 100Watts from last reading.  It logs this counter. It then compares to the week before (if present) and tells the percentage of delta. This 
should show up if devices on thermostats or frequent power cycling start running more often than reference.   The Alarm will reset every hour as its hourly based.. if latching is needed, use a HA helper

