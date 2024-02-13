TARGET=build/rtc
TARGET_SRC=rtc.cpp

I2C_SRC=I2C/I2CDevice.cpp
I2C_INC=I2C/I2CDevice.h
I2C_OBJ=build/I2C/I2CDevice

CC = arm-unknown-linux-gnueabihf-g++

$(TARGET): $(TARGET_SRC) $(I2C_OBJ)
	$(CC) -o $(TARGET) $(TARGET_SRC) $(I2C_OBJ) -II2CDevice

$(I2C_OBJ): $(I2C_SRC) $(I2C_INC)
	$(CC) -c $(I2C_SRC) -o $(I2C_OBJ) 

clean:
	rm ./build/* $(TARGET)

REMOTE_USER="arun"
REMOTE_HOST="192.168.1.200"
REMOTE_PATH="/home/arun/Documents/assgt1"
directory=/Users/arun/Documents/UNIVERSITY/EE513_Connected_Embedded/assignment_1/DS3231_RPI

upload:
	rsync -avz --progress $(directory) "arun@${REMOTE_HOST}:${REMOTE_PATH}"