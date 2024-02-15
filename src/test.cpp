#include <unistd.h>
#include <iostream>
#include <ctime>
#include <iomanip>

#include "RTC/rtc.h"

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

/**
 * The function to print time from user_time_ptr_t pointer to struct
 * @param timePtr user_time_ptr_t pointer to struct with the time data
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

// Function to print user_alarm_t
void printUserAlarm(user_alarm_ptr_t alarm_ptr) {
    std::cout << "Seconds: " << static_cast<int>(alarm_ptr->seconds) << std::endl;
    std::cout << "Minutes: " << static_cast<int>(alarm_ptr->minutes) << std::endl;
    std::cout << "Hours: " << static_cast<int>(alarm_ptr->hours) << std::endl;
    std::cout << "Day or Date: " << static_cast<int>(alarm_ptr->day_or_date) << std::endl;
    
    // Print union member based on the value of 'day_or_date'
    if (alarm_ptr->day_or_date == 0) {
        std::cout << "Date of Month: " << static_cast<int>(alarm_ptr->day_date.date_of_month) << std::endl;
    } else {
        std::cout << "Day of Week: " << static_cast<int>(alarm_ptr->day_date.day_of_week) << std::endl;
    }

    // Print union member based on the type of rate alarm
    if (alarm_ptr->alarm_num == 1) {
        std::cout << "Rate alarm 1: " << HEX(alarm_ptr->rate_alarm.rate_1) << std::endl;
    } else {
        std::cout << "Rate alarm 2: " << HEX(alarm_ptr->rate_alarm.rate_2) << std::endl;
    }
}

const string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

int main()
{
    RTC rtc(1, 0x68);

    // rtc.writeCurrentTimeToRTC();
    // user_time_ptr_t read_t = rtc.readTime();
    // printUserTime(read_t);

    // cout << rtc.setAlarm1(30, 45, 18) << endl;
    // cout << rtc.setAlarm2(20, 19) << endl;

    // cout << HEX(rtc.getRateAlarm1()) << endl;

    rtc.setTimeAlarm1(45,45,18);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DAY);
    user_alarm_ptr_t alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);
    // rtc.debugDumpRegisters(13);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_HOUR);
    alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);
    // rtc.debugDumpRegisters(13);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_MINUTE);
    alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);
    // rtc.debugDumpRegisters(13);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_DATE_DAY);
    alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);
    // rtc.debugDumpRegisters(13);
    rtc.setRateAlarm1(ALARM_1_ONCE_PER_SECOND);
    alarm_ptr = rtc.getAlarm1();
    // printUserAlarm(alarm_ptr);

    rtc.close();

    return 0;
}