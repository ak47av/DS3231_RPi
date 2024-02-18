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

IPStack ipstack = IPStack();
float version = 0.3;
const char* topic = "DS3231";
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);

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
    printf("Payload:\n%.*s\n", (int)message.payloadlen, (char*)message.payload);
}


/**
 * The function `printUserTime` prints the values of a `user_time_ptr_t` object, which represents a
 * specific date and time, to the console.
 * 
 * @param timePtr timePtr is a pointer to a struct or class object that contains information about a
 * user's time. The struct or class should have the following members:
 * */
void printUserTime(user_time_ptr_t timePtr)
{
    if (timePtr == nullptr)
    {
        cerr << "Error: Null pointer provided." << endl;
        return;
    }
    cout << "Seconds: " << static_cast<int>(timePtr->seconds) << endl;
    cout << "Minutes: " << static_cast<int>(timePtr->minutes) << endl;
    if(timePtr->clock_12hr)
    {
        cout << "Hours: " << static_cast<int>(timePtr->hours);
        if(timePtr->am_pm) cout << " PM" << endl;
        else cout << " AM" << endl;
    }
    else cout << "Hours: " << static_cast<int>(timePtr->hours) << endl;
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
    cout << "Seconds: " << static_cast<int>(alarm_ptr->seconds) << endl;
    cout << "Minutes: " << static_cast<int>(alarm_ptr->minutes) << endl;
    if(alarm_ptr->clock_12hr)
    {
        cout << "Hours: " << static_cast<int>(alarm_ptr->hours);
        if(alarm_ptr->am_pm) cout << " PM" << endl;
        else cout << " AM" << endl;
    } else cout << "Hours: " << static_cast<int>(alarm_ptr->hours) << endl;
    cout << "Day or Date: " << static_cast<int>(alarm_ptr->day_or_date) << endl;

    // Print union member based on the value of 'day_or_date'
    if (alarm_ptr->day_or_date == 0)
        cout << "Date of Month: " << static_cast<int>(alarm_ptr->day_date.date_of_month) << endl;
    else
        cout << "Day of Week: " << static_cast<int>(alarm_ptr->day_date.day_of_week) << endl;

    // Print union member based on the type of rate alarm
    if (alarm_ptr->alarm_num == 1)
    {
        cout << "Rate of alarm 1: ";
        switch (alarm_ptr->rate_alarm.rate_1)
        {
        case ALARM_1_ONCE_PER_DATE_DAY:
            if(alarm_ptr->day_or_date == 0)
                cout << "Once on every date of the month" << endl;
            else cout << "Once on every day of the week" << endl;
            break;
        
        case ALARM_1_ONCE_PER_SECOND:
            cout << "Once every second" << endl;
            break;
        
        case ALARM_1_ONCE_PER_MINUTE:
            cout << "Once every minute when seconds match" << endl;
            break;
        
        case ALARM_1_ONCE_PER_HOUR:
            cout << "Once every hour when minutes and seconds match" << endl;
            break;
        
        case ALARM_1_ONCE_PER_DAY:
            cout << "Once every time hours, minutes and seconds match" << endl;
            break;
        
        default:
            break;
        }
    }
    else
    {
        cout << "Rate of alarm 2: ";
        switch (alarm_ptr->rate_alarm.rate_2)
        {
        case ALARM_2_ONCE_PER_DATE_DAY:
            if(alarm_ptr->day_or_date == 0)
                cout << "Once on every date of the month" << endl;
            else cout << "Once on every day of the week" << endl;
            break;
        
        case ALARM_2_ONCE_PER_MINUTE:
            cout << "Once every minute when seconds match" << endl;
            break;
        
        case ALARM_2_ONCE_PER_HOUR:
            cout << "Once every hour when minutes and seconds match" << endl;
            break;
        
        case ALARM_2_ONCE_PER_DAY:
            cout << "Once every time hours, minutes and seconds match" << endl;
            break;
        
        default:
            break;
        }

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
    cout << "Received signal " << signum << ", exiting..." << endl;
    running = false;
}


/**
 * The function `sendMQTTMessage` sends a MQTT message with QoS 0 using the provided message, buffer,
 * and topic.
 * 
 * @param message The `message` parameter is of type `MQTT::Message` and is used to store the message
 * details such as QoS level, retained flag, duplicate flag, payload, and payload length.
 * @param buf The `buf` parameter in the `sendMQTTMessage` function is a character array that contains
 * the payload data to be sent in the MQTT message.
 * @param topic The `topic` parameter in the `sendMQTTMessage` function is a pointer to a constant
 * character array that represents the topic to which the MQTT message will be published. It specifies
 * the destination or channel to which the message will be sent within the MQTT broker.
 */
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

    rtc.setTimeAlarm2(10, FORMAT_0_23, PM, 10, 0, 1);

    rtc.setRateAlarm2(ALARM_2_ONCE_PER_DATE_DAY);
    user_alarm_ptr_t alarm = rtc.getAlarm2();
    printUserAlarm(alarm);
    
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_MINUTE);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);

    rtc.setRateAlarm2(ALARM_2_ONCE_PER_HOUR);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);

    rtc.setRateAlarm2(ALARM_2_ONCE_PER_DAY);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);

    
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    rtc.setRateAlarm1(ALARM_1_ONCE_PER_MINUTE);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    rtc.setRateAlarm1(ALARM_1_ONCE_PER_HOUR);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DAY);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DATE_DAY);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);
    // SEND MQTT MESSAGES TO THE BROKER ON THE "DS3231" TOPIC

    rtc.enableSquareWave(SQW_8KHZ);
    
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