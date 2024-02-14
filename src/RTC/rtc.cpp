#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <iomanip>
#include <memory>

#include "rtc.h"

using namespace std;

#define HEX(x) setw(2) << setfill('0') << hex << (int)(x)


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

/**
 * The function to convert an 8 bit BCD value into an 8 bit decimal value into  
 * @param BCD 8 bit BCD value
 * @return 8 bit decimal value
 */
uint8_t RTC::BCD_to_decimal(uint8_t BCD_value)
{
    uint8_t res = BCD_value & 0xF;
    res += 10 * (BCD_value >> 4);
    return res;
}

/**
 * The function to read the time from the RTC Module.
 * @return user_time_ptr_t pointer to struct containing the time and date data in decimal format
 */
user_time_ptr_t RTC::readTime()
{
    user_time_ptr_t t (new user_time_t);
    if (t.get() == nullptr)
    {
        perror("RTC: NO MEMORY AVAILABLE to allocate user_time_t* t");
        return nullptr;
    }
    unsigned char *data = this->readRegisters(7, 0x00);
    t->seconds = this->BCD_to_decimal(*data);
    t->minutes = this->BCD_to_decimal(*(data + 1));
    t->hours = this->BCD_to_decimal(*(data + 2));
    t->day_of_week = this->BCD_to_decimal(*(data + 3));
    t->date_of_month = this->BCD_to_decimal(*(data + 4));
    t->month = this->BCD_to_decimal(*(data + 5));
    t->year = this->BCD_to_decimal(*(data + 6));
    delete []data;
    return t;
}

/**
 * The function to write time onto the RTC Module
 * @param t is a user_time_ptr_t pointer to struct containing the time and date
 * @return 0 if successful, 1 if not
 */
int RTC::writeTime(user_time_ptr_t t)
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

/**
 * The function to convert an 8 bit decimal value into an 8 bit BCD value
 * @param decimal decimal value
 * @return 8 bit BCD value
 */
uint8_t RTC::decimal_to_BCD(uint8_t decimal)
{
    uint8_t res = (decimal/10) << 4;
    res |= decimal%10;
    return res;
}

/**
 * The function to write the system time to the RTC Module
 */
void RTC::writeCurrentTimeToRTC()
{
    time_t now = time(0);
    struct tm tstruct;
    tstruct = *localtime(&now);

    user_time_ptr_t t (new user_time_t);
    t->seconds = decimal_to_BCD(tstruct.tm_sec);
    t->minutes = decimal_to_BCD(tstruct.tm_min);
    t->hours = decimal_to_BCD(tstruct.tm_hour);
    t->day_of_week = decimal_to_BCD(tstruct.tm_wday+1);
    t->date_of_month = decimal_to_BCD(tstruct.tm_mday);
    t->month = decimal_to_BCD(tstruct.tm_mon + 1);

    int yearMod100 = tstruct.tm_year % 100;
    t->year = decimal_to_BCD(yearMod100);
    this->writeTime(t);
}

/**
 * The function to read the Temperature from the RTC Module
 * @return float representation of Temperature
 */
float RTC::getTemperature()
{
    float temp_msb = static_cast<float>(this->readRegister(REG_TEMPERATURE_MSB));
    unsigned char temp_lsb = this->readRegister(REG_TEMPERATURE_LSB);
    unsigned char decimal_bits = (temp_lsb & 0xC0) >> 6;
    switch (decimal_bits)
    {
    case 0b01:
        temp_msb += 0.25;
        break;

    case 0b10:
        temp_msb += 0.50;
        break;

    case 0b11:
        temp_msb += 0.75;
        break;
    
    default:
        break;
    }
    return temp_msb;
}