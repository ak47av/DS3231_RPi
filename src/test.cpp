#include <unistd.h>
#include <iostream>
#include <ctime>

#include "RTC/rtc.h"

using namespace std;

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
    // cout << "currentDateTime()=" << currentDateTime() << endl;

    // rtc.writeCurrentTimeToRTC();
    // user_time_ptr_t read_t = rtc.readTime();
    // printUserTime(read_t);

    cout << rtc.getTemperature() << endl;
    return 0;
}