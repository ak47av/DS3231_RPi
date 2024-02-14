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
        cerr << "RTC: NO MEMORY AVAILABLE to allocate user_time_t* t" << endl;
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
    if(res) return res;
    res = this->writeRegister(REG_TIME_MINUTES, t->minutes);
    if(res) return res;
    res = this->writeRegister(REG_TIME_HOURS, t->hours);
    if(res) return res;
    res = this->writeRegister(REG_TIME_DAY_OF_WEEK, t->day_of_week);
    if(res) return res;
    res = this->writeRegister(REG_TIME_DATE_OF_MONTH, t->date_of_month);
    if(res) return res;
    res = this->writeRegister(REG_TIME_MONTH, t->month);
    if(res) return res;
    res = this->writeRegister(REG_TIME_YEAR, t->year);
    if(res) return res;
    if(res) cerr << "RTC: Unable to write time to module" << endl;
    return res;
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
int RTC::writeCurrentTimeToRTC()
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
    if(this->writeTime(t))
    {
        cerr << "RTC: Unable to write system time to module" << endl;
        return 1;
    }
    return 0;
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


int RTC::setAlarm(uint8_t alarm_num, uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    // Set alarm minutes
    if(minutes > 59 || minutes < 0)
    {
        cerr << "Minutes can't be more than 59 or less than 0" << endl;
        return 1;
    }
    unsigned char minutesBCD = (decimal_to_BCD(minutes));

    // Set alarm hours
    if(hours > 23 || hours < 0)
    {
        cerr << "Hours cannot be more than 23 or less than 0" << endl;
        return 1;
    }
    unsigned char hoursBCD = (decimal_to_BCD(hours));

    // Set Alarm day or date
    uint8_t day_date_to_set = 0;
    if(day_or_date == 1)
    {
        day_date_to_set |= MASK_ALARM_DAY_OR_DATEINV;
        if(day_date > 7 || day_date < 1)
        {
            cerr << "Day cannot be greater than 7 or lesser than 1" << endl;
            return 1;
        }
    }
    else if (day_or_date == 0)
    {
        day_date_to_set &= ~(MASK_ALARM_DAY_OR_DATEINV);
        if(day_date > 31 || day_date < 1)
        {
            cerr << "Date cannot be greater than 31 or lesser than 1" << endl;
            return 1;
        }
    }
    else
    {
        cerr << "If Day then 1, if Date then 0, no other values permitted" << endl;
        return 1;
    }
    day_date_to_set |= decimal_to_BCD(day_date);
    unsigned char day_date_to_set_BCD = static_cast<unsigned char>(day_date_to_set);

    int res = 0;
    if(alarm_num == 1)
    {
        res = this->writeRegister(REG_MINUTES_ALARM_1, minutesBCD);
        if(res) return res;
        res = this->writeRegister(REG_HOURS_ALARM_1, hoursBCD);
        if(res) return res;
        res = this->writeRegister(REG_DAYS_ALARM_1, day_date_to_set_BCD);
        if(res) return res;
    } 
    else if (alarm_num == 2)
    {
        res = this->writeRegister(REG_MINUTES_ALARM_2, minutesBCD);
        if(res) return res;
        res = this->writeRegister(REG_HOURS_ALARM_2, hoursBCD);
        if(res) return res;
        res = this->writeRegister(REG_DAYS_ALARM_2, day_date_to_set_BCD);
        if(res) return res;
    }
    if(res) cerr << "RTC: Unable to set the Alarm" << endl;
    return res;
}

int RTC::setAlarm1(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    if(seconds > 59 || seconds < 0)
    {
        cerr << "Seconds cannot be greater than 59 or less than 0" << endl;
        return 1;
    }
    int res = 0;
    res = this->setAlarm(1, minutes, hours, day_or_date, day_date);
    if(res) return res;
    res = this->writeRegister(REG_SECONDS_ALARM_1, decimal_to_BCD(seconds));
    if(res) return res;
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_1_INT_ENABLE));
    if(res) return res;
    return res;
}

int RTC::setAlarm2(uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    int res = 0;
    res = this->setAlarm(2, minutes, hours, day_or_date, day_date);
    if(res) return res;
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_2_INT_ENABLE));
    if(res) return res;
    return res;
}
