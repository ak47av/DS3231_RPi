#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <gpiod.hpp>
#include <memory.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

#include "linux.cpp"    // PAHO MQTT Dependency
#include "MQTTClient.h"
#include "RTC/rtc.h"

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

///////////////// RUN THE TESTS BELOW ONE BY ONE ///////////////////////////

// #define TEST_WITH_MQTT               // REQUIRES A CONNECTION TO AN MQTT BROKER
// #define TEST_TIME_API
#define TEST_ALARM_API
// #define TEST_ALARM_EVERY_SECOND      // Ctrl+C to stop alarm for 5 seconds  
// #define TEST_ALARM_EVERY_MINUTE      // RISING EDGE EVERY MINUTE
// #define TEST_TEMPERATURE
// #define TEST_SQW

///////////////// RUN THE TESTS BELOW ONE BY ONE ///////////////////////////

// MQTT PARAMETERS
#define HOSTNAME    "192.168.1.58"
#define PORT        1883
#define DEVICE_ID   "DS3231_Raspberry_Pi_5"
#define TOPIC       "temperature"

// Global variable to control the loop
volatile bool running = true;

#ifdef TEST_WITH_MQTT

/* Setting up some initial configurations for the MQTT client.*/
IPStack ipstack = IPStack();
float version = 0.3;
const char* topic = "DS3231";
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);

int arrivedcount = 0;
/**
 * Prints information about an incoming MQTT message.
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
 * Sends a MQTT message with QoS 0 using the provided message, buffer, and topic.
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
#endif

/**
 * Prints the values of a shared memory safe `user_time_ptr_t` pointer to a `user_time_t` struct
 * @param timePtr timePtr is the pointer to a struct that contains information about the time.
 * */
void printUserTime(user_time_ptr_t timePtr)
{
    if (timePtr == nullptr)
    {
        cerr << "Error: Null pointer provided." << endl;
        return;
    }
    cout << "Time: ";
    if(timePtr->clock_12hr)
    {
        if(timePtr->am_pm) cout << static_cast<int>(timePtr->hours) << ":" << static_cast<int>(timePtr->minutes) << ":" << static_cast<int>(timePtr->seconds) << " PM" << endl;
        else cout << static_cast<int>(timePtr->hours) << ":" << static_cast<int>(timePtr->minutes) << ":" << static_cast<int>(timePtr->seconds) << " AM" << endl;
    }
    else cout << static_cast<int>(timePtr->hours) << ":" << static_cast<int>(timePtr->minutes) << ":" << static_cast<int>(timePtr->seconds) << endl;
    cout << "Day of Week: " << static_cast<int>(timePtr->day_of_week) << endl;
    cout << "Date of Month: " << static_cast<int>(timePtr->date_of_month) << endl;
    cout << "Month: " << static_cast<int>(timePtr->month) << endl;
    cout << "Year: " << static_cast<int>(2000 + timePtr->year) << endl;
}

/**
 * Prints the values of a user alarm structure, including the seconds,
 * minutes, hours, day or date, and rate of the alarm.
 * 
 * @param alarm_ptr The parameter `alarm_ptr` is a pointer to a structure of type `user_alarm_ptr_t`.
 */
void printUserAlarm(user_alarm_ptr_t alarm_ptr)
{
    // if(alarm_ptr->alarm_num == 1) cout << "Seconds: " << static_cast<int>(alarm_ptr->seconds) << endl;
    // cout << "Minutes: " << static_cast<int>(alarm_ptr->minutes) << endl;
    // if(alarm_ptr->clock_12hr)
    // {
    //     cout << "Hours: " << static_cast<int>(alarm_ptr->hours);
    //     if(alarm_ptr->am_pm) cout << " PM" << endl;
    //     else cout << " AM" << endl;
    // } else cout << "Hours: " << static_cast<int>(alarm_ptr->hours) << endl;

    cout << "Time: ";
    if(alarm_ptr->alarm_num == 1)
    {
        if(alarm_ptr->clock_12hr)
        {
            if(alarm_ptr->am_pm) cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << ":" << static_cast<int>(alarm_ptr->seconds) << " PM" << endl;
            else cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << ":" << static_cast<int>(alarm_ptr->seconds) << " AM" << endl;
        }
        else cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << ":" << static_cast<int>(alarm_ptr->seconds) << endl;
    } 
    else 
    {
       if(alarm_ptr->clock_12hr)
        {
            if(alarm_ptr->am_pm) cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << " PM" << endl;
            else cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << " AM" << endl;
        }
        else cout << static_cast<int>(alarm_ptr->hours) << ":" << static_cast<int>(alarm_ptr->minutes) << endl; 
    }
    // cout << "Day or Date: " << static_cast<int>(alarm_ptr->day_or_date) << endl;

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
 * Returns the current date and time in the format "YYYY-MM-DD.HH:MM:SS".
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
 * Prints a message indicating the received signal number and stops the loop by setting the
 * "running" variable to false.
 * 
 * @param signum signum is the parameter that represents the signal number that triggered the signal
 * handler function.
 */
void signal_handler(int signum) {
    cout << "Received signal " << signum  << endl;
    running = false;
}


int main() {
    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);

    #ifdef TEST_WITH_MQTT
    // //////////////////// MQTT CONNECTION /////////////////////
    printf("Version is %f\n", version);
    const char* hostname = HOSTNAME;
    int port = PORT;
    printf("Connecting to %s:%d\n", hostname, port);
    int rc = ipstack.connect(hostname, port);
	if (rc != 0)
	    printf("rc from TCP connect is %d\n", rc);
 
	printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;       
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*)DEVICE_ID;
    rc = client.connect(data);
	if (rc != 0)
	    printf("rc from MQTT connect is %d\n", rc);
	printf("MQTT connected\n");
    
    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);   
    if (rc != 0)
        printf("rc from MQTT subscribe is %d\n", rc);

    MQTT::Message message;
    char buf[200];
    // //////////////////// MQTT CONNECTION /////////////////////
    #endif

    ////////////////////// DEMONSTRATING THE API ///////////////////////
    RTC rtc(1, 0x68);

    #ifdef TEST_TIME_API
    cout << ">>>> Testing the Time APIs" << endl;
    cout << "\n>>>> Get the time on the RTC" << endl;
    user_time_ptr_t time = rtc.getTime();
    printUserTime(time);

    cout << "\n>>>> Set the time" << endl;
    uint8_t seconds         = 25;
    uint8_t minutes         = 50;
    uint8_t hours           = 19;
    uint8_t day_of_week     = 5;
    uint8_t date            = 12;
    uint8_t month           = 2;
    uint8_t year_from_2000  = 12;
    rtc.setTime(seconds, minutes, FORMAT_0_23, AM, hours, day_of_week, date, month, year_from_2000); // counts from 2000
    time = rtc.getTime();
    printUserTime(time);

    cout << "\n>>>> Set the current system time to the RTC" << endl;
    rtc.setCurrentTimeToRTC(FORMAT_0_12);
    time = rtc.getTime();
    printUserTime(time);
    #endif

    #ifdef TEST_ALARM_API
    cout << "\n\n>>>> Testing Alarm APIs" << endl;
    uint8_t alarm_1_seconds         = 25;
    uint8_t alarm_1_minutes         = 50;
    uint8_t alarm_1_hours           = 19;
    uint8_t alarm_1_day_of_week     = 5;
    uint8_t alarm_1_date            = 12;
    cout << ">>>> Testing Alarm 1" << endl;
    cout << ">>>> Setting Alarm 1" << endl;
    rtc.setTimeAlarm1(alarm_1_seconds, alarm_1_minutes, FORMAT_0_23, PM, alarm_1_hours, DATE_OF_MONTH, alarm_1_date); // AM or PM does not matter in 23 hour format
    user_alarm_ptr_t alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 1 to once per second" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 1 to once per minute" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_MINUTE);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 1 to once per hour" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_HOUR);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 1 to once per day" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DAY);
    alarm = rtc.getAlarm1();
    printUserAlarm(alarm);

    cout << "\n>>>> Testing Alarm 2" << endl;
    uint8_t alarm_2_minutes         = 50;
    uint8_t alarm_2_hours           = 19;
    uint8_t alarm_2_day_of_week     = 5;
    uint8_t alarm_2_date            = 12;
    cout << ">>>> Setting Alarm 2" << endl;
    rtc.setTimeAlarm2(alarm_2_minutes, FORMAT_0_23, PM, alarm_2_hours, DAY_OF_WEEK, alarm_2_day_of_week); // AM or PM does not matter while FORMAT_0_23
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 2 to once per minute" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_MINUTE);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);
    
    cout << "\n>>>> Setting Rate of Alarm 2 to once per hour" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_HOUR);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);

    cout << "\n>>>> Setting Rate of Alarm 2 to once per day" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_DAY);
    alarm = rtc.getAlarm2();
    printUserAlarm(alarm);
    #endif

    #ifdef TEST_ALARM_EVERY_SECOND
    rtc.disableInterruptAlarm2();
    rtc.setTimeAlarm1(0, 0, FORMAT_0_23, AM, 0, DAY_OF_WEEK, 1);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    while(true)
    {
        if(!running)
        {
            cout << "Disable Alarm 1 for 5 seconds" << endl;
            rtc.disableInterruptAlarm1();
            sleep(5);
            rtc.enableInterruptAlarm1();
            running = true;
            cout << "Alarm 1 will resume ringing" << endl;
        }
        rtc.snoozeAlarm1();
        cout << "Snoozed Alarm" << endl;
        sleep(1);
    }
    #endif

    #ifdef TEST_ALARM_EVERY_MINUTE
    rtc.disableInterruptAlarm1();
    rtc.setTimeAlarm2(0, FORMAT_0_23, AM, 0, DAY_OF_WEEK, 1);
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_MINUTE);
    while(running)
    {
        rtc.snoozeAlarm2();
        cout << "Snoozed Alarm" << endl;
        sleep(60);
    }
    #endif

    #ifdef TEST_SQW
    rtc.enableSquareWave(SQW_1HZ); // Only a 1Hz wave is produced (register mismatch for clone chips)
    #endif

    #ifdef TEST_TEMPERATURE
    float temp;
    while(running)
    {
        temp = rtc.getTemperature();
        cout << "Temperature is: " << temp << endl;
        sleep(60);
    }
    #endif

    #ifdef TEST_WITH_MQTT
    float temp;
    char* topic = TOPIC;
    while(running)
    {
        temp = rtc.getTemperature();
        cout << "Temperature is: " << temp << endl;
        sprintf(buf, "Temperature is: %.2f", temp);
        sendMQTTMessage(message, buf, topic);
        sleep(60);
    }
    #endif
    ////////////////////// DEMONSTRATING THE API ///////////////////////
    
    #ifdef TEST_WITH_MQTT
    // //////////////////// Terminate MQTT CONNECTION /////////////////////
    rc = client.unsubscribe(topic);
    if (rc != 0)
        printf("rc from unsubscribe was %d\n", rc);
    
    rc = client.disconnect();
    if (rc != 0)
        printf("rc from disconnect was %d\n", rc);
    
    ipstack.disconnect();
    // //////////////////// Terminate MQTT CONNECTION /////////////////////
    #endif
    
    return 0;
}