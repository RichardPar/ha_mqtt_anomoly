CC = gcc
CFLAGS = -I/usr/include -I/usr/include/cjson
LDFLAGS = -L/usr/lib -lpaho-mqtt3c -lcjson

TARGET = mqtt_subscriber
SRC = mqtt_subscriber.c utils.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
