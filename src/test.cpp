#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <gpiod.hpp>

#include "MQTTClient.h"
#include "RTC/rtc.h"

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

// Global variable to control the loop
volatile bool running = true;

/**
 * The function `printUserTime` prints the values of a `user_time_ptr_t` object, which represents a
 * specific date and time, to the console.
 * 
 * @param timePtr timePtr is a pointer to a struct or class object that contains information about a
 * user's time. The struct or class should have the following members:
 * 
 * @return The function does not return any value. It has a void return type, which means it does not
 * return anything.
 */
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

int main() {
    // Register signal handler for SIGINT
    signal(SIGINT, signal_handler);

    RTC rtc(1, 0x68);
    // user_alarm_ptr_t alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);

    // alarm_ptr = rtc.getAlarm2();
    // printUserAlarm(alarm_ptr);
    rtc.setTimeAlarm1();
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_MINUTE);
    
    rtc.enableSquareWave(SQW_8KHZ);

    while (running) {
        // rtc.readRegister(0x00);
        // rtc.enableSquareWave(SQW_1KHZ);
        // rtc.snoozeAlarm1();
        // sleep(1);
    }

    return 0;
}