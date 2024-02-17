TARGET=build/test
TARGET_SRC=src/test.cpp

I2C_SRC=src/I2C/I2CDevice.cpp
I2C_INC=src/I2C/I2CDevice.h
I2C_OBJ=build/I2C/I2CDevice

RTC_SRC=src/RTC/rtc.cpp
RTC_INC=src/RTC/rtc.h
RTC_OBJ=build/RTC/rtc

MQTT_CLIENT_DIR = /home/arun/paho.mqtt.embedded-c/MQTTClient/samples/linux
MQTT_INCLUDES = -I $(MQTT_CLIENT_DIR)/../../src/ -I $(MQTT_CLIENT_DIR)/../../src/linux -I $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTPacket.c $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTDeserializePublish.c $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTConnectClient.c $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTSubscribeClient.c $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTSerializePublish.c $(MQTT_CLIENT_DIR)/../../../MQTTPacket/src/MQTTUnsubscribeClient.c

# Determine the operating system
UNAME_S := $(shell uname -s)

# Set the default compiler
ifeq ($(UNAME_S),Darwin)
    CC := arm-unknown-linux-gnueabihf-g++
else
    CC := g++
endif

# Other Makefile rules...
$(TARGET): $(TARGET_SRC) $(I2C_OBJ) $(RTC_OBJ)
	$(CC) -g -o $(TARGET) $(TARGET_SRC) $(I2C_OBJ) $(RTC_OBJ) -II2CDevice -Irtc -lgpiod $(MQTT_INCLUDES)

$(I2C_OBJ): $(I2C_SRC) $(I2C_INC)
	$(CC) -g -c $(I2C_SRC) -o $(I2C_OBJ)

$(RTC_OBJ): $(RTC_SRC) $(RTC_INC) $(I2C_OBJ)
	$(CC) -g -c $(RTC_SRC) -o $(RTC_OBJ)

clean:
	rm $(TARGET)
	rm $(I2C_OBJ)
	rm $(RTC_OBJ)

REMOTE_USER="arun"
REMOTE_HOST="192.168.1.200"
REMOTE_PATH="/home/arun/Documents/assgt1"
directory=/Users/arun/Documents/UNIVERSITY/EE513_Connected_Embedded/assignment_1/DS3231_RPI
password_file=/password_file/password

upload:
	rsync -avz --progress -e ssh $(directory) "arun@${REMOTE_HOST}:${REMOTE_PATH}"