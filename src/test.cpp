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

#include "linux.cpp" // PAHO MQTT Dependency
#include "MQTTClient.h"
#include "RTC/rtc.h"

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

///////////////// RUN THE TESTS BELOW ONE BY ONE ///////////////////////////

// #define TEST_TIME_API                // Runs once
// #define TEST_ALARM_API               // Runs once
// #define TEST_ALARM_EVERY_SECOND      // Runs indefinitely, Ctrl+C to stop alarm for 5 seconds
// #define TEST_ALARM_EVERY_MINUTE      // Runs indefinitely, RISING EDGE EVERY MINUTE
// #define TEST_TEMPERATURE             // Runs indefinitely
// #define TEST_SQW                     // Runs once
// #define TEST_WITH_MQTT               // Runs indefinitely, REQUIRES A CONNECTION TO AN MQTT BROKER
// #define TEST_32kHz                   // Runs indefinitely

///////////////// RUN THE TESTS BELOW ONE BY ONE ///////////////////////////

// MQTT PARAMETERS
#define HOSTNAME "broker.emqx.io"
#define PORT 1883
#define DEVICE_ID "DS3231_Raspberry_Pi_5"
#define TOPIC "temperature"

// Global variable to control the loop
volatile bool running = true;

#ifdef TEST_WITH_MQTT
/* Setting up some initial configurations for the MQTT client.*/
IPStack ipstack = IPStack();
float version = 0.3;
MQTT::Client<IPStack, Countdown> client = MQTT::Client<IPStack, Countdown>(ipstack);

int arrivedcount = 0;
/**
 * Prints information about an incoming MQTT message. For Debugging purposes only.
 *
 * @param md In the `messageArrived` function, `md` is of type `MQTT::MessageData&`, which is a
 * reference to a structure containing information about the arrived message. This structure typically
 * includes the message itself along with other metadata related to the message.
 */
void messageArrived(MQTT::MessageData &md)
{
    MQTT::Message &message = md.message;

    printf("Message %d arrived: qos %d, retained %d, dup %d, packetid %d\n",
           ++arrivedcount, message.qos, message.retained, message.dup, message.id);
    printf("Payload:\n%.*s\n", (int)message.payloadlen, (char *)message.payload);
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
void sendMQTTMessage(MQTT::Message message, char *buf, const char *topic)
{
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *)buf;
    message.payloadlen = strlen(buf) + 1;
    int rc = client.publish(topic, message);
    if (rc != 0)
        printf("Error %d from sending QoS 0 message\n", rc);
}
#endif

/**
 * Prints a message indicating the received signal number and stops the loop by setting the
 * "running" variable to false.
 *
 * @param signum signum is the parameter that represents the signal number that triggered the signal
 * handler function.
 */
void signal_handler(int signum)
{
    cout << "Received signal " << signum << endl;
    running = false;
}

int main()
{
    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);

#ifdef TEST_WITH_MQTT
    // //////////////////// MQTT CONNECTION /////////////////////
    // Print the MQTT version
    printf("Version is %f\n", version);

    const char *hostname = HOSTNAME;
    int port = PORT;
    printf("Connecting to %s:%d\n", hostname, port);
    // connect to the Host on the specified port
    int rc = ipstack.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\n", rc);

    // Initialize the MQTT parameters
    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)DEVICE_ID;
    rc = client.connect(data);
    if (rc != 0)
        printf("rc from MQTT connect is %d\nUnable to connect to MQTT broker\n", rc);
    else printf("MQTT connected\n");

    MQTT::Message message;
    char buf[200];
// //////////////////// MQTT CONNECTION /////////////////////
#endif

    ////////////////////// DEMONSTRATING THE API ///////////////////////
    RTC rtc(1, 0x68);

#ifdef TEST_TIME_API
    // Test the functionality of setting the time, setting the system time and getting the time
    cout << ">>>> Testing the Time APIs" << endl;
    cout << "\n>>>> Get the time on the RTC" << endl;
    rtc.displayTime();

    cout << "\n>>>> Set the time" << endl;
    uint8_t seconds = 25;
    uint8_t minutes = 50;
    uint8_t hours = 19;
    uint8_t day_of_week = 5;
    uint8_t date = 12;
    uint8_t month = 2;
    uint8_t year_from_2000 = 12;
    rtc.setTime(seconds, minutes, FORMAT_0_23, AM, hours, day_of_week, date, month, year_from_2000); // counts from 2000
    rtc.displayTime();

    cout << "\n>>>> Set the current system time to the RTC" << endl;
    rtc.setCurrentTimeToRTC(FORMAT_0_12);
    rtc.displayTime();
#endif

#ifdef TEST_ALARM_API
    // Test the functionality of setting time, rate and getting the details of the alarms
    cout << "\n\n>>>> Testing Alarm APIs" << endl;
    uint8_t alarm_1_seconds = 25;
    uint8_t alarm_1_minutes = 50;
    CLOCK_FORMAT alarm_1_format = FORMAT_0_12;
    AM_OR_PM alarm_1_AM_PM = PM;
    uint8_t alarm_1_hours = 7;
    DAY_OR_DATE alarm_1_dayordate = DATE_OF_MONTH;
    uint8_t alarm_1_day_of_week = 5;
    uint8_t alarm_1_date = 12;
    cout << ">>>> Testing Alarm 1" << endl;
    cout << ">>>> Setting Alarm 1" << endl;
    rtc.setTimeAlarm1(alarm_1_seconds, alarm_1_minutes, alarm_1_format, alarm_1_AM_PM, alarm_1_hours, alarm_1_dayordate, alarm_1_date); // AM or PM does not matter in 23 hour format
    rtc.displayAlarm1();

    cout << "\n>>>> Setting Rate of Alarm 1 to once per second" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    rtc.displayAlarm1();

    cout << "\n>>>> Setting Rate of Alarm 1 to once per minute" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_MINUTE);
    rtc.displayAlarm1();

    cout << "\n>>>> Setting Rate of Alarm 1 to once per hour" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_HOUR);
    rtc.displayAlarm1();

    cout << "\n>>>> Setting Rate of Alarm 1 to once per day" << endl;
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DAY);
    rtc.displayAlarm1();

    cout << "\n>>>> Testing Alarm 2" << endl;
    uint8_t alarm_2_minutes = 50;
    CLOCK_FORMAT alarm_2_format = FORMAT_0_23;
    AM_OR_PM alarm_2_AM_PM = PM;
    uint8_t alarm_2_hours = 17;
    DAY_OR_DATE alarm_2_dayordate = DAY_OF_WEEK;
    uint8_t alarm_2_day_of_week = 5;
    uint8_t alarm_2_date = 12;
    cout << ">>>> Setting Alarm 2" << endl;
    rtc.setTimeAlarm2(alarm_2_minutes, alarm_2_format, alarm_2_AM_PM, alarm_2_hours, alarm_2_dayordate, alarm_2_day_of_week); // AM or PM does not matter while FORMAT_0_23
    rtc.displayAlarm2();

    cout << "\n>>>> Setting Rate of Alarm 2 to once per minute" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_MINUTE);
    rtc.displayAlarm2();

    cout << "\n>>>> Setting Rate of Alarm 2 to once per hour" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_HOUR);
    rtc.displayAlarm2();

    cout << "\n>>>> Setting Rate of Alarm 2 to once per day" << endl;
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_DAY);
    rtc.displayAlarm2();
#endif

#ifdef TEST_ALARM_EVERY_SECOND
    // Hit Ctrl + C to pause the alarm ringin for 5 seconds by disabling the interrupt
    // Hit Ctrl + Z to exit the program
    rtc.disableInterruptAlarm2();
    rtc.setTimeAlarm1(0, 0, FORMAT_0_23, AM, 0, DAY_OF_WEEK, 1);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    while (true)
    {
        if (!running)
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
    // test alarm 2 ringing every minute
    rtc.disableInterruptAlarm1();
    rtc.setTimeAlarm2(0, FORMAT_0_23, AM, 0, DAY_OF_WEEK, 1);
    rtc.setRateAlarm2(ALARM_2_ONCE_PER_MINUTE);
    while (running)
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
    // prints temperature every 60 seconds
    float temp;
    while (running)
    {
        temp = rtc.getTemperature();
        cout << "Temperature is: " << temp << endl;
        sleep(60);
    }
#endif

#ifdef TEST_WITH_MQTT
    // publishes a string with temperature every 60 seconds to the MQTT broker configured
    float temp;
    char *topic = TOPIC;
    while (running && (rc==0))
    {
        temp = rtc.getTemperature();
        cout << "Temperature is: " << temp << endl;
        sprintf(buf, "Temperature is: %.2f", temp);
        sendMQTTMessage(message, buf, topic);
        sleep(60);
    }
#endif

#ifdef TEST_32kHz
    while (running)
    {
        rtc.setState32kHz(HIGH_IMPEDANCE);
        cout << "Set to HIGH_IMPEDANCE" << endl;
        sleep(5);
        rtc.setState32kHz(ON);
        cout << "Set to ON" << endl;
        sleep(5);
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
