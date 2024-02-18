#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <iomanip>
#include <memory>

#include "rtc.h"

using namespace std;

#define HEX(x) setw(1) << setfill('0') << hex << (int)(x)

/**
 * The RTC constructor initializes an instance of the RTC class with the specified bus and device
 * parameters.
 * 
 * @param bus The "bus" parameter represents the I2C bus number that the RTC device is connected to.
 * This is typically a number that identifies a specific I2C bus on the system, such as 0 or 1.
 * @param device The "device" parameter in the RTC constructor is an unsigned integer that represents
 * the device address of the RTC (Real-Time Clock) module. This address is used to communicate with the
 * RTC module over the I2C bus.
 */
RTC::RTC(unsigned int bus, unsigned int device) : I2CDevice(bus, device)
{
}


/**
 * The function converts a BCD (Binary-Coded Decimal) value to its decimal equivalent.
 * 
 * @param BCD_value The BCD_value parameter is an 8-bit unsigned integer that represents a binary-coded
 * decimal value.
 * 
 * @return the decimal value of the BCD (Binary-Coded Decimal) value passed as an argument.
 */
uint8_t RTC::BCD_to_decimal(uint8_t BCD_value)
{
    uint8_t res = BCD_value & 0xF;
    res += 10 * (BCD_value >> 4);
    return res;
}


/**
 * The function `RTC::getTime()` reads time data from registers and converts it to a user-friendly
 * format.
 * 
 * @return A pointer to a memory sage shared pointer user_time_ptr_t
 */
user_time_ptr_t RTC::getTime()
{
    user_time_ptr_t t (new user_time_t);
    if (t.get() == nullptr)
    {
        cerr << "RTC: NO MEMORY AVAILABLE to allocate user_time_t* t" << endl;
        return nullptr;
    }
    unsigned char *data = this->readRegisters(7, 0x00);
    t->seconds          = this->BCD_to_decimal(data[0]);
    t->minutes          = this->BCD_to_decimal(data[1]);

    // evalute if 12 hr clock or 24 hr clock
    if(data[2] & 0x40) t->clock_12hr = FORMAT_0_12;
    if(t->clock_12hr)
    {
        if((data[2] & 0x20) >> 5) t->am_pm        = PM; // if 1 then PM, else AM
        else t->am_pm       = AM;
        t->hours        = this->BCD_to_decimal(data[2] & 0x1F);
    }
    else t->hours       = this->BCD_to_decimal(data[2] & 0x3F);

    t->day_of_week      = this->BCD_to_decimal(data[3]);
    t->date_of_month    = this->BCD_to_decimal(data[4]);
    t->month            = this->BCD_to_decimal(data[5] & 0x1F);
    t->year             = this->BCD_to_decimal(data[6]);
    delete []data;
    return t;
}


/**
 * The function `setTime` in C++ sets the time and date on a real-time clock module.
 * 
 * @param seconds The `seconds` parameter represents the seconds value of the time you want to set. It
 * should be an integer value between 0 and 59.
 * @param minutes The `minutes` parameter in the `setTime` function represents the minutes component of
 * the time you want to set. It should be a value between 0 and 59 to represent the minutes of the
 * hour.
 * @param clock_12_hr The `clock_12_hr` parameter is a flag indicating whether the time should be set
 * in 12-hour format. If `clock_12_hr` is true, the time will be set in 12-hour format with an
 * additional bit for AM/PM indication. If `clock_12_hr
 * @param am_pm The `am_pm` parameter in the `setTime` function is used to specify whether the time is
 * in the AM or PM for a 12-hour clock format. It is of type `AM_OR_PM`, which likely is an enum or a
 * typedef defining the values for AM and PM. When
 * @param hours The `hours` parameter in the `setTime` function represents the hours component of the
 * time to be set. It can be in 24-hour format or 12-hour format based on the `clock_12_hr` parameter.
 * @param day_of_week The `day_of_week` parameter in the `setTime` function represents the day of the
 * week. It is expected to be provided as an integer value where 1 represents Sunday, 2 represents
 * Monday, and so on until 7 which represents Saturday.
 * @param date_of_month The `date_of_month` parameter in the `setTime` function represents the day of
 * the month. It is used to set the specific day within the month for the real-time clock.
 * @param month The `month` parameter in the `setTime` function represents the month of the year. It is
 * expected to be provided as an integer value ranging from 1 to 12, where 1 represents January and 12
 * represents December. The function converts this integer value into Binary-Coded Decimal (
 * @param year The `year` parameter in the `setTime` function represents the year value that you want
 * to set in the Real-Time Clock (RTC) module. This parameter should be provided as an 8-bit unsigned
 * integer representing the year value (e.g., 2022 would be represented as 22
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTime(uint8_t seconds, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, uint8_t day_of_week, uint8_t date_of_month, uint8_t month, uint8_t year)
{
    int res = this->writeRegister(REG_TIME_SECONDS, this->decimal_to_BCD(seconds));
    if(res) return res;
    res = this->writeRegister(REG_TIME_MINUTES, this->decimal_to_BCD(minutes));
    if(res) return res;
    if(clock_12_hr)
    {
        uint8_t reg_value = this->decimal_to_BCD(hours) | 0x40;
        if(am_pm) reg_value |= 0x20;
        res = this->writeRegister(REG_TIME_HOURS, reg_value);
        if(res) return res;
    }
    else {
        res = this->writeRegister(REG_TIME_HOURS, this->decimal_to_BCD(hours));
        if(res) return res;
    }
    res = this->writeRegister(REG_TIME_DAY_OF_WEEK, this->decimal_to_BCD(day_of_week));
    if(res) return res;
    res = this->writeRegister(REG_TIME_DATE_OF_MONTH, this->decimal_to_BCD(date_of_month));
    if(res) return res;
    res = this->writeRegister(REG_TIME_MONTH, this->decimal_to_BCD(month));
    if(res) return res;
    res = this->writeRegister(REG_TIME_YEAR, this->decimal_to_BCD(year));
    if(res) return res;
    return res;
}


/**
 * The function converts a decimal number to binary-coded decimal (BCD) format.
 * 
 * @param decimal The decimal parameter is an unsigned 8-bit integer that represents a decimal number.
 * 
 * @return a uint8_t value, which is the result of converting a decimal number to BCD (Binary-Coded
 * Decimal) format.
 */
uint8_t RTC::decimal_to_BCD(uint8_t decimal)
{
    uint8_t res = (decimal/10) << 4;
    res |= decimal%10;
    return res;
}

/**
 * The function `setCurrentTimeToRTC` sets the current system time to a real-time clock module in C++,
 * handling 12-hour clock format and error checking.
 * 
 * @param clock_12_hr The `clock_12_hr` parameter is used to determine whether the time should be set
 * in 12-hour format or 24-hour format. If `clock_12_hr` is false, the time will be set in 24-hour
 * format. If `clock_12_hr` is true,
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setCurrentTimeToRTC(CLOCK_FORMAT clock_12_hr)
{
    time_t now = time(0);
    struct tm tstruct;
    tstruct = *localtime(&now);

    int seconds         = tstruct.tm_sec;
    int minutes         = tstruct.tm_min;
    AM_OR_PM am_pm      = AM;
    int hours;
    if(!clock_12_hr)
    {
        hours           = tstruct.tm_hour;
    } else {
        if(tstruct.tm_hour > 12)
        {
            hours   = tstruct.tm_hour - 12;
            am_pm   = PM;
        } else
        {
           hours = tstruct.tm_hour;
           am_pm = AM;
        }
    }
    int day_of_week     = tstruct.tm_wday+1;
    int date_of_month   = tstruct.tm_mday;
    int month           = tstruct.tm_mon + 1;

    int yearMod100      = tstruct.tm_year % 100;
    int year            = yearMod100;
    if(this->setTime(seconds, minutes, clock_12_hr, am_pm, hours, day_of_week, date_of_month, month, year))
    {
        cerr << "RTC: Unable to write system time to module" << endl;
        return 1;
    }
    return 0;
}


/**
 * The function "getTemperature" reads the temperature from a register and adjusts it based on the
 * decimal bits.
 * 
 * @return the temperature value as a float.
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

/**
 * The function `setTimeAlarm` in C++ sets the time alarm based on specified parameters such as
 * minutes, hours, day or date, and alarm number.
 * 
 * @param alarm_num The `alarm_num` parameter in the `setTimeAlarm` function is used to specify which
 * alarm to set, either alarm 1 or alarm 2. It is an integer parameter that should be either 1 or 2 to
 * indicate the alarm number.
 * @param minutes The `minutes` parameter in the `setTimeAlarm` function represents the minutes value
 * for setting the alarm. It should be an integer value between 0 and 59.
 * @param clock_12_hr The parameter `clock_12_hr` in the `setTimeAlarm` function is used to specify
 * whether the clock is in 12-hour format or not.
 * @param am_pm The `am_pm` parameter in the `setTimeAlarm` function is used to specify whether the
 * alarm time is in the AM or PM for a 12-hour clock format. It is of type `AM_OR_PM`, which
 * represents an enumeration with two possible values - `AM` and `PM`
 * @param hours The `hours` parameter in the `setTimeAlarm` function represents the hour at which the
 * alarm should trigger.
 * @param day_or_date The `day_or_date` parameter in the `setTimeAlarm` function is used to specify
 * whether the alarm should be set based on a day of the week or a specific date in the month.
 * @param day_date The `day_date` parameter in the `setTimeAlarm` function represents either the day of
 * the week or the date of the month, depending on the value of the `day_or_date` parameter.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTimeAlarm(uint8_t alarm_num, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    // Set alarm minutes
    if(minutes > 59 || minutes < 0)
    {
        cerr << "Minutes can't be more than 59 or less than 0" << endl;
        return 1;
    }
    unsigned char minutesBCD = decimal_to_BCD(minutes);

    // Set alarm hours
    if(hours > 23 || hours < 0)
    {
        cerr << "Hours cannot be more than 23 or less than 0" << endl;
        return 1;
    }
    unsigned char hoursBCD;
    if(clock_12_hr)
    {
        hoursBCD = this->decimal_to_BCD(hours) | 0x40;
        if(am_pm) hoursBCD |= 0x20;
    } else {
        hoursBCD = this->decimal_to_BCD(hours);
    }

    // Set Alarm day or date
    unsigned char day_date_to_set = 0;
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


/**
 * The function `setTimeAlarm1` sets the alarm time for Alarm 1 in a real-time clock module in C++,
 * including validation checks and enabling the alarm interrupt.
 * 
 * @param seconds The `seconds` parameter in the `setTimeAlarm1` function represents the seconds value
 * for setting the alarm. 
 * @param minutes The `minutes` parameter in the `setTimeAlarm1` function represents the minutes value
 * at which the alarm should trigger.
 * @param clock_12_hr The parameter `clock_12_hr` is used to specify whether the time format is in
 * 12-hour or 24-hour format.
 * @param am_pm The `am_pm` parameter in the `setTimeAlarm1` function is used to specify whether the
 * alarm time is in the AM or PM for a 12-hour clock format.
 * @param hours The `hours` parameter in the `setTimeAlarm1` function represents the hour at which the
 * alarm should trigger. 
 * @param day_or_date The `day_or_date` parameter in the `setTimeAlarm1` function is used to specify
 * whether the alarm should trigger based on the day of the week or the date of the month. 
 * @param day_date The `day_date` parameter in the `setTimeAlarm1` function is used to specify whether
 * the alarm should trigger on a specific day of the week (1-7) or a specific date of the month (1-31),
 * depending on the value of the `day_or_date` parameter
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTimeAlarm1(uint8_t seconds, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    if(seconds > 59 || seconds < 0)
    {
        cerr << "Seconds cannot be greater than 59 or less than 0" << endl;
        return 1;
    }
    int res = 0;
    res = this->setTimeAlarm(1, minutes, clock_12_hr, am_pm, hours, day_or_date, day_date);
    if(res) return res;
    res = this->writeRegister(REG_SECONDS_ALARM_1, decimal_to_BCD(seconds));
    if(res) return res;
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_1_INT_ENABLE | MASK_INTERRUPT_CONTROL));
    if(res) return res;
    return res;
}


/**
 * The function `setTimeAlarm2` sets the alarm time for alarm 2 and enables the corresponding
 * interrupt.
 * 
 * @param minutes The `minutes` parameter in the `setTimeAlarm2` function represents the minutes value
 * at which the alarm should trigger.
 * @param clock_12_hr The parameter `clock_12_hr` in the `setTimeAlarm2` function is used to specify
 * whether the time format is 12-hour or 24-hour format.
 * @param am_pm The `am_pm` parameter in the `setTimeAlarm2` function is used to specify whether the
 * alarm time is in the AM or PM for a 12-hour clock format. It can have the values `AM` or `PM`.
 * @param hours The `hours` parameter in the `setTimeAlarm2` function represents the hour at which the
 * alarm should trigger.
 * @param day_or_date The `day_or_date` parameter in the `setTimeAlarm2` function is used to specify
 * whether the alarm should trigger based on the day of the week or the date of the month. 
 * @param day_date The `day_date` parameter in the `setTimeAlarm2` function represents the day of the
 * month or the date of the month for which the alarm is being set. 
 * 
 * @return 0 if successful, 1 if unsucessful
 */
int RTC::setTimeAlarm2(uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    int res = 0;
    res = this->setTimeAlarm(2, minutes,clock_12_hr, am_pm, hours, day_or_date, day_date);
    if(res) return res;
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_2_INT_ENABLE | MASK_INTERRUPT_CONTROL));
    if(res) return res;
    return res;
}

/**
 * The function `getRateAlarm1` returns the rate at which the alarm should occur based on the values in
 * the `alarm_1_regs` array.
 * 
 * @param alarm_1_regs The parameter "alarm_1_regs" is an array of uint8_t (unsigned 8-bit integers)
 * that represents the registers of the alarm 1 in a real-time clock (RTC) device.
 * 
 * @return the rate at which the alarm 1 is set to trigger. The possible return values are:
 * - ALARM_1_ONCE_PER_DATE_DAY
 * - ALARM_1_ONCE_PER_DAY
 * - ALARM_1_ONCE_PER_HOUR
 * - ALARM_1_ONCE_PER_MINUTE
 * - ALARM_1_ONCE_PER_SECOND
 */
rate_alarm_1 RTC::getRateAlarm1(uint8_t* alarm_1_regs)
{
    uint8_t A1M4 = alarm_1_regs[3] >> 7;
    if(A1M4 == 0)
    {
        uint8_t dy_dt = (alarm_1_regs[3] & MASK_ALARM_DAY_OR_DATEINV) >> 6;
        return ALARM_1_ONCE_PER_DATE_DAY;
    }
    uint8_t A1M3 = alarm_1_regs[2] >> 7;
    if(A1M3 == 0) return ALARM_1_ONCE_PER_DAY;
    uint8_t A1M2 = alarm_1_regs[1] >> 7;
    if(A1M2 == 0) return ALARM_1_ONCE_PER_HOUR;
    uint8_t A1M1 = alarm_1_regs[0] >> 7;
    if(A1M1 == 0) return ALARM_1_ONCE_PER_MINUTE;
    return ALARM_1_ONCE_PER_SECOND;
}

/**
 * The function "getRateAlarm2" returns the rate at which the alarm should occur based on the values in
 * the "alarm_2_regs" array.
 * 
 * @param alarm_2_regs The parameter "alarm_2_regs" is an array of uint8_t (unsigned 8-bit integers)
 * that represents the registers of the alarm 2 in a real-time clock (RTC) device.
 * 
 * @return the rate at which Alarm 2 is set to trigger. The possible return values are:
 * - ALARM_2_ONCE_PER_DATE_DAY
 * - ALARM_2_ONCE_PER_DAY
 * - ALARM_2_ONCE_PER_HOUR
 * - ALARM_2_ONCE_PER_MINUTE
 */
rate_alarm_2 RTC::getRateAlarm2(uint8_t* alarm_2_regs)
{
    uint8_t A2M4 = alarm_2_regs[2] >> 7;
    if(A2M4 == 0)
    {
        uint8_t dy_dt = (alarm_2_regs[2] & MASK_ALARM_DAY_OR_DATEINV) >> 6;
        return ALARM_2_ONCE_PER_DATE_DAY;
    }
    uint8_t A2M3 = alarm_2_regs[1] >> 7;
    if(A2M3 == 0) return ALARM_2_ONCE_PER_DAY;
    uint8_t A2M2 = alarm_2_regs[0] >> 7;
    if(A2M2 == 0) return ALARM_2_ONCE_PER_HOUR;
    return ALARM_2_ONCE_PER_MINUTE;
}

/**
 * The function `getAlarm1` returns a pointer to a struct containing information about alarm 1.
 * 
 * @return a memory safe shared pointer to a user_alarm_t struct
 */
user_alarm_ptr_t RTC::getAlarm1()
{
    user_alarm_ptr_t alarm_1 (new user_alarm_t);
    uint8_t* alarm_1_regs = this->readRegisters(4, REG_SECONDS_ALARM_1);
    rate_alarm_1 alarm_rate = this->getRateAlarm1(alarm_1_regs);
    alarm_1->rate_alarm.rate_1 = alarm_rate;
    alarm_1->alarm_num = 1;
    alarm_1->seconds = this->BCD_to_decimal(alarm_1_regs[0] & MASK_ALARM_SECONDS);
    alarm_1->minutes = this->BCD_to_decimal(alarm_1_regs[1] & MASK_ALARM_MINUTES);

    if(alarm_1_regs[2] & 0x40) alarm_1->clock_12hr = FORMAT_0_12;
    if(alarm_1->clock_12hr)
    {
        if((alarm_1_regs[2] & 0x20) >> 5) alarm_1->am_pm = PM;
        else alarm_1->am_pm = AM;
        alarm_1->hours   = this->BCD_to_decimal(alarm_1_regs[2] & 0x1F);
    }
    else alarm_1->hours = this->BCD_to_decimal(alarm_1_regs[2] & 0x3F);
    
    alarm_1->day_or_date = (alarm_1_regs[3] & MASK_ALARM_DAY_OR_DATEINV) >> 6;
    if(alarm_1->day_or_date) alarm_1->day_date.day_of_week = (alarm_1_regs[3] & MASK_ALARM_DAY_DATE);
    else alarm_1->day_date.date_of_month = (alarm_1_regs[3] & MASK_ALARM_DAY_DATE);

    return alarm_1;
}

/**
 * The function "getAlarm2" returns a pointer to a user_alarm_t object that contains information about
 * the second alarm.
 * 
 * @return a memory safe shared pointer to a user_alarm_t struct.
 */
user_alarm_ptr_t RTC::getAlarm2()
{
    user_alarm_ptr_t alarm_2 (new user_alarm_t);
    uint8_t* alarm_2_regs = this->readRegisters(3, REG_MINUTES_ALARM_2);
    rate_alarm_2 alarm_rate = this->getRateAlarm2(alarm_2_regs);
    alarm_2->rate_alarm.rate_2 = alarm_rate;
    alarm_2->alarm_num = 2;
    alarm_2->minutes = this->BCD_to_decimal(alarm_2_regs[0] & MASK_ALARM_MINUTES);
    if(alarm_2->clock_12hr)
    {
        if((alarm_2_regs[1] & 0x20) >> 5) alarm_2->am_pm = PM;
        else alarm_2->am_pm = AM;
        alarm_2->hours   = this->BCD_to_decimal(alarm_2_regs[1] & 0x1F);
    }
    else alarm_2->hours = this->BCD_to_decimal(alarm_2_regs[2] & 0x3F);
    
    alarm_2->day_or_date = (alarm_2_regs[2] & MASK_ALARM_DAY_OR_DATEINV) >> 6;
    if(alarm_2->day_or_date) alarm_2->day_date.day_of_week = (alarm_2_regs[2] & MASK_ALARM_DAY_DATE);
    else alarm_2->day_date.date_of_month = (alarm_2_regs[2] & MASK_ALARM_DAY_DATE);

    return alarm_2;
}

/**
 * The function `setRateAlarm1` sets the rate alarm for a real-time clock (RTC) based on the given
 * rate.
 * 
 * @param rate The "rate" parameter is of type "rate_alarm_1", 
 * representing the desired alarm rate for alarm 1. The function sets the alarm rate by
 * modifying specific bits in the alarm registers.
 * 
 * @return 0 if successful, 1 if unsuccesful
 */
int RTC::setRateAlarm1(rate_alarm_1 rate)
{
    int res = 0;
    uint8_t* alarm_regs = this->readRegisters(4, REG_SECONDS_ALARM_1);
    uint8_t A1M1, A1M2, A1M3, A1M4;

    A1M1 = rate & 0b1;
    cout << endl;
    if(A1M1) alarm_regs[0] |= 0x80;
    else alarm_regs[0] &= ~(0x80);
    res = this->writeRegister(REG_SECONDS_ALARM_1,alarm_regs[0]);
    if(res) return 1;

    A1M2 = rate & 0b10;
    if(A1M2) alarm_regs[1] |= 0x80;
    else alarm_regs[1] &= ~(0x80);
    res = this->writeRegister(REG_MINUTES_ALARM_1,alarm_regs[1]);
    if(res) return 1;

    A1M3 = rate & 0b100;
    if(A1M3) alarm_regs[2] |= 0x80;
    else alarm_regs[2] &= ~(0x80);
    res = this->writeRegister(REG_HOURS_ALARM_1,alarm_regs[2]);
    if(res) return 1;

    A1M4 = rate & 0b1000;
    if(A1M4) alarm_regs[3] |= 0x80;
    else alarm_regs[3] &= ~(0x80);
    res = this->writeRegister(REG_DAYS_ALARM_1,alarm_regs[3]);
    if(res) return 1;
    return 0;
}

/**
 * The function "setRateAlarm2" sets the rate for alarm 2 by modifying the appropriate bits in the
 * alarm registers.
 * 
 * @param rate The "rate" parameter is of type "rate_alarm_2", 
 * representing different rate options for the alarm 2.
 * 
 * @return 0 if successful, 1 if unsucessful
 */
int RTC::setRateAlarm2(rate_alarm_2 rate)
{
    int res = 0;
    uint8_t* alarm_regs = this->readRegisters(3, REG_MINUTES_ALARM_2);
    uint8_t A2M2, A2M3, A2M4;

    A2M2 = rate & 0b1;
    if(A2M2) alarm_regs[0] |= 0x80;
    else alarm_regs[0] &= ~(0x80);
    res = this->writeRegister(REG_MINUTES_ALARM_2,alarm_regs[0]);
    if(res) return 1;

    A2M3 = rate & 0b10;
    if(A2M3) alarm_regs[1] |= 0x80;
    else alarm_regs[1] &= ~(0x80);
    res = this->writeRegister(REG_HOURS_ALARM_2,alarm_regs[1]);
    if(res) return 1;

    A2M4 = rate & 0b100;
    if(A2M4) alarm_regs[2] |= 0x80;
    else alarm_regs[2] &= ~(0x80);
    res = this->writeRegister(REG_DAYS_ALARM_2,alarm_regs[2]);
    if(res) return 1;

    return 0;
}

/**
 * The function snoozeAlarm1 in C++ snoozes Alarm 1 by clearing the alarm flag in the status register.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::snoozeAlarm1()
{
    uint8_t status_reg = this->readRegister(REG_STATUS);
    status_reg &= ~(MASK_ALARM_1_FLAG);
    int res = this->writeRegister(REG_STATUS, status_reg);
    if(res) cerr << "RTC: Unable to snooze Alarm 1" << endl;
    return res;
}

/**
 * The function snoozeAlarm2 in C++ snoozes Alarm 2 by clearing the alarm flag in the status register.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::snoozeAlarm2()
{
    uint8_t status_reg = this->readRegister(REG_STATUS);
    status_reg &= ~(MASK_ALARM_2_FLAG);
    int res = this->writeRegister(REG_STATUS, status_reg);
    if(res) cerr << "RTC: Unable to snooze Alarm 1" << endl;
    return res;
}


/**
 * The function `disableAlarm1` in C++ disables Alarm 1 on an RTC device.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::disableAlarm1()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);
    control_reg &= ~(MASK_ALARM_1_INT_ENABLE);
    int res = this->writeRegister(REG_CONTROL, control_reg);
    if(res) cerr << "RTC: Unable to disable Alarm 1" << endl;
    return res;
}

/**
 * This function disables Alarm 2 in an RTC module by updating the control register.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::disableAlarm2()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);
    control_reg &= ~(MASK_ALARM_2_INT_ENABLE);
    int res = this->writeRegister(REG_CONTROL, control_reg);
    if(res) cerr << "RTC: Unable to disable Alarm 2" << endl;
    return res;
}

/**
 * The function `enableSquareWave` in the RTC class enables a square wave output at a specified
 * frequency. It has not been tested (due to the testing chip being different than the original)
 * 
 * @param freq The `freq` parameter in the `enableSquareWave` function is of type `sqw_frequency`,
 * which is an enum representing different frequencies for the square wave output
 * of the RTC module.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::enableSquareWave(sqw_frequency freq)
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);
    control_reg &= ~(MASK_ALARM_1_INT_ENABLE | MASK_ALARM_2_INT_ENABLE | MASK_INTERRUPT_CONTROL | MASK_RATE_SELECT_1 | MASK_RATE_SELECT_2);
    control_reg |= (freq << 3);
    control_reg |= MASK_BAT_BACKUP_SQW_ENABLE;
    int res = this->writeRegister(REG_CONTROL, control_reg);
    if(res) cerr << "RTC: Unable to enable Square wave" << endl;
    return res;
}

/**
 * The RTC destructor closes the RTC object.
 */
RTC::~RTC()
{
    this->close();
}