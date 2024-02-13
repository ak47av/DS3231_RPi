#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <iomanip>
#include <memory>

#include "rtc.h"

using namespace std;

#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)

uint8_t BCD_to_decimal(uint8_t BCD_value)
{
    uint8_t res;
    res = BCD_value & 0xF;
    res += 10 * (BCD_value >> 4);
    return res;
}

/**
 * Constructor for the RTC class. It requires the bus number and device number.
 * The constructor opens a file handle to the I2C device, which is destroyed when
 * the destructor is called
 * @param bus The bus number.
 * @param device The device ID on the bus.
 */
RTC::RTC(unsigned int bus, unsigned int device) : I2CDevice(bus, device)
{
}

user_time_ptr_t RTC::readTime()
{
    user_time_ptr_t t (new user_time_t);
    if (t.get() == nullptr)
    {
        perror("RTC: NO MEMORY AVAILABLE to allocate user_time_t* t");
        return nullptr;
    }
    unsigned char *data = this->readRegisters(6, 0x00);
    t->seconds = BCD_to_decimal(*data);
    t->minutes = BCD_to_decimal(*(data + 1));
    t->hours = BCD_to_decimal(*(data + 2));
    t->day_of_week = BCD_to_decimal(*(data + 3));
    t->date_of_month = BCD_to_decimal(*(data + 4));
    t->month = BCD_to_decimal(*(data + 5));
    t->year = BCD_to_decimal(*(data + 6));
    delete [] data;
    return t;
}

int RTC::writeTime(user_time_t* t)
{
    int res = this->writeRegister(REG_TIME_SECONDS, t->seconds);
    res = this->writeRegister(REG_TIME_MINUTES, t->minutes);
    res = this->writeRegister(REG_TIME_HOURS, t->hours);
    res = this->writeRegister(REG_TIME_DAY_OF_WEEK, t->day_of_week);
    res = this->writeRegister(REG_TIME_DATE_OF_MONTH, t->date_of_month);
    res = this->writeRegister(REG_TIME_MONTH, t->month);
    res = this->writeRegister(REG_TIME_YEAR, t->year);
    if(res == 1)
    {
        perror("RTC: Unable to write time\n");
        return 1;
    }
    return 0;
}
