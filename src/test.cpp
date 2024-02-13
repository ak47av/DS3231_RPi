#include <unistd.h>
#include <iostream>

#include "RTC/rtc.h"

using namespace std;

void printUserTime(const user_time_t *timePtr)
{
    if (timePtr == nullptr)
    {
        std::cerr << "Error: Null pointer provided." << std::endl;
        return;
    }

    std::cout << "Seconds: " << static_cast<int>(timePtr->seconds) << std::endl;
    std::cout << "Minutes: " << static_cast<int>(timePtr->minutes) << std::endl;
    std::cout << "Hours: " << static_cast<int>(timePtr->hours) << std::endl;
    std::cout << "Day of Week: " << static_cast<int>(timePtr->day_of_week) << std::endl;
    std::cout << "Date of Month: " << static_cast<int>(timePtr->date_of_month) << std::endl;
    std::cout << "Month: " << static_cast<int>(timePtr->month) << std::endl;
    std::cout << "Year: " << static_cast<int>(timePtr->year) << std::endl;
}

int main()
{
    RTC rtc(1, 0x68);
    user_time_t* t = rtc.readTime();
    printUserTime(t);
    delete t;
    return 0;
}