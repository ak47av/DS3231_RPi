#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <gpiod.hpp>
#include <memory.h>
#include <arpa/inet.h>

#include "linux.cpp"
#include "MQTTClient.h"
#include "RTC/rtc.h"

#define MQTTCLIENT_QOS2 1
#define DEFAULT_STACK_SIZE -1

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

// Global variable to control the loop
volatile bool running = true;

int arrivedcount = 0;

/**
 * The function `messageArrived` prints information about an incoming MQTT message.
 * 
 * @param md In the `messageArrived` function, `md` is of type `MQTT::MessageData&`, which is a
 * reference to a structure containing information about the arrived message. This structure typically
 * includes the message itself along with other metadata related to the message.
 */
void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;

    printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n", 
		++arrivedcount, message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", (int)message.payloadlen, (char*)message.payload);
}


/**
 * The function `printUserTime` prints the values of a `user_time_ptr_t` object, which represents a
 * specific date and time, to the console.
 * 
 * @param timePtr timePtr is a pointer to a struct or class object that contains information about a
 * user's time. The struct or class should have the following members:
 * 
 * @return The function does not return any value. It has a void return type, which means it does not
 * return anything.  */
void printUserTime(user_time_ptr_t timePtr)
{
    if (timePtr == nullptr)
    {
        cerr << "Error: Null pointer provided." << endl;
        return;
    }
    cout << "Seconds: " << static_cast<int>(timePtr->seconds) << endl;
    cout << "Minutes: " << static_cast<int>(timePtr->minutes) << endl;
    cout << "Hours: " << static_cast<int>(timePtr->hours) << endl;
    cout << "Day of Week: " << static_cast<int>(timePtr->day_of_week) << endl;
    cout << "Date of Month: " << static_cast<int>(timePtr->date_of_month) << endl;
    cout << "Month: " << static_cast<int>(timePtr->month) << endl;
    cout << "Year: " << static_cast<int>(2000 + timePtr->year) << endl;
}

/**
 * The function `printUserAlarm` prints the values of a user alarm structure, including the seconds,
 * minutes, hours, day or date, and rate of the alarm.
 * 
 * @param alarm_ptr The parameter `alarm_ptr` is a pointer to a structure of type `user_alarm_ptr_t`.
 */
void printUserAlarm(user_alarm_ptr_t alarm_ptr)
{
    std::cout << "Seconds: " << static_cast<int>(alarm_ptr->seconds) << std::endl;
    std::cout << "Minutes: " << static_cast<int>(alarm_ptr->minutes) << std::endl;
    std::cout << "Hours: " << static_cast<int>(alarm_ptr->hours) << std::endl;
    std::cout << "Day or Date: " << static_cast<int>(alarm_ptr->day_or_date) << std::endl;

    // Print union member based on the value of 'day_or_date'
    if (alarm_ptr->day_or_date == 0)
    {
        std::cout << "Date of Month: " << static_cast<int>(alarm_ptr->day_date.date_of_month) << std::endl;
    }
    else
    {
        std::cout << "Day of Week: " << static_cast<int>(alarm_ptr->day_date.day_of_week) << std::endl;
    }

    // Print union member based on the type of rate alarm
    if (alarm_ptr->alarm_num == 1)
    {
        std::cout << "Rate alarm 1: " << HEX(alarm_ptr->rate_alarm.rate_1) << std::endl;
    }
    else
    {
        std::cout << "Rate alarm 2: " << HEX(alarm_ptr->rate_alarm.rate_2) << std::endl;
    }
}

/**
 * The function "currentDateTime" returns the current date and time in the format
 * "YYYY-MM-DD.HH:MM:SS".
 * 
 * @return a string that represents the current date and time in the format "YYYY-MM-DD.HH:MM:SS".
 */
const string currentDateTime()
{
    time_t now = time(0);
    struct tm tstruct;
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}


/**
 * The function "signal_handler" prints a message indicating the received signal number and sets the
 * "running" variable to false.
 * 
 * @param signum signum is the parameter that represents the signal number that triggered the signal
 * handler function.
 */
void signal_handler(int signum) {
    std::cout << "Received signal " << signum << ", exiting..." << std::endl;
    running = false;
}

IPStack ipstack = IPStack();
float version = 0.3;
const char* topic = "DS3231";
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);
void sendMQTTMessage(MQTT::Message message, char* buf, const char* topic)
{
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*)buf;
    message.payloadlen = strlen(buf)+1;
    int rc = client.publish(topic, message);
	if (rc != 0)
		printf("Error %d from sending QoS 0 message\n", rc);
    // else while (arrivedcount == 0)
    //     client.yield(100);
}

int main() {
    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);


    // //////////////////// MQTT CONNECTION /////////////////////
    printf("Version is %f\n", version);
    const char* hostname = "192.168.1.58";
    int port = 1883;
    printf("Connecting to %s:%d\n", hostname, port);
    int rc = ipstack.connect(hostname, port);
	if (rc != 0)
	    printf("rc from TCP connect is %d\n", rc);
 
	printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*)"magdalenabay";
    rc = client.connect(data);
	if (rc != 0)
	    printf("rc from MQTT connect is %d\n", rc);
	printf("MQTT connected\n");
    
    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);   
    if (rc != 0)
        printf("rc from MQTT subscribe is %d\n", rc);

    MQTT::Message message;
    // //////////////////// MQTT CONNECTION /////////////////////

    // SEND MQTT MESSAGES TO THE BROKER ON THE "DS3231" TOPIC
    char buf[200];
    RTC rtc(1, 0x68);
    user_time_ptr_t t = rtc.readTime();
    sprintf(buf, "Time from RTC\nSeconds:%d \nMinutes:%d \nHours:%d \n", t->seconds, t->minutes, t->hours);
    sendMQTTMessage(message, buf, topic);

    memset(buf,0,strlen(buf));
    float temperature = rtc.getTemperature();
    sprintf(buf, "Temperature from RTC: %f\n", temperature);
    sendMQTTMessage(message, buf, topic);
    // SEND MQTT MESSAGES TO THE BROKER ON THE "DS3231" TOPIC
    
    // //////////////////// Terminate MQTT CONNECTION /////////////////////
    rc = client.unsubscribe(topic);
    if (rc != 0)
        printf("rc from unsubscribe was %d\n", rc);
    
    rc = client.disconnect();
    if (rc != 0)
        printf("rc from disconnect was %d\n", rc);
    
    ipstack.disconnect();
    // //////////////////// Terminate MQTT CONNECTION /////////////////////
    
    // printf("Finishing with %d messages received\n", arrivedcount);

    return 0;
}