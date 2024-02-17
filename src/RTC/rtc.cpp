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
 * The function `readTime()` reads the current time from an RTC (Real-Time Clock) and returns it as a
 * `user_time_ptr_t` object.
 * 
 * @return a pointer to a user_time_t object.
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
    t->seconds          = this->BCD_to_decimal(*data);
    t->minutes          = this->BCD_to_decimal(*(data + 1));
    t->hours            = this->BCD_to_decimal(*(data + 2));
    t->day_of_week      = this->BCD_to_decimal(*(data + 3));
    t->date_of_month    = this->BCD_to_decimal(*(data + 4));
    t->month            = this->BCD_to_decimal(*(data + 5));
    t->year             = this->BCD_to_decimal(*(data + 6));
    delete []data;
    return t;
}


/**
 * The function writes the time values from a user-defined structure to the corresponding registers of
 * an RTC module.
 * 
 * @param t The parameter "t" is a pointer to a structure of type "user_time_ptr_t". This structure
 * contains the following fields:
 * 
 * @return 0 if successful, 1 if unsuccessful
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
 * The function "writeCurrentTimeToRTC" writes the current system time to a Real-Time Clock (RTC)
 * module.
 * 
 * @return an integer value. If the writeTime function returns true (indicating an error), the function
 * returns 1. Otherwise, it returns 0.
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
 * The function sets the time and date for an alarm on an RTC (Real-Time Clock) device.
 * 
 * @param alarm_num The alarm number to set. It can be either 1 or 2.
 * @param minutes The "minutes" parameter represents the minutes value for the alarm. It should be an
 * integer between 0 and 59.
 * @param hours The "hours" parameter represents the hour value for setting the alarm. It should be an
 * integer value between 0 and 23, where 0 represents midnight and 23 represents 11 PM.
 * @param day_or_date The parameter "day_or_date" is used to specify whether the alarm is set for a
 * specific day of the week (1) or a specific date of the month (0).
 * @param day_date The "day_date" parameter in the code represents either the day of the week or the
 * date of the month, depending on the value of the "day_or_date" parameter.
 * 
 * @return an integer value.
 */
int RTC::setTimeAlarm(uint8_t alarm_num, uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
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

/**
 * The function sets the time on alarm 1 on an RTC device by validating the input parameters, setting the
 * alarm time, enabling the alarm interrupt, and returning the result.
 * 
 * @param seconds The seconds parameter is an integer that represents the desired value for the seconds
 * field of the alarm time. It should be between 0 and 59.
 * @param minutes The "minutes" parameter is the value of the minutes at which the alarm should
 * trigger.
 * @param hours The "hours" parameter represents the hour value for setting the time alarm. It should
 * be a value between 0 and 23, representing the hours in a 24-hour format.
 * @param day_or_date The "day_or_date" pn is used to specify
 * whether the alarm should trigger on a specific day of the week or on a specific date of the month.
 * @param day_date The "day_date" parameter is used to specify whether
 * the alarm should trigger on a specific day of the week or on a specific date of the month.
 * 
 * @return an integer value.
 */
int RTC::setTimeAlarm1(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    if(seconds > 59 || seconds < 0)
    {
        cerr << "Seconds cannot be greater than 59 or less than 0" << endl;
        return 1;
    }
    int res = 0;
    res = this->setTimeAlarm(1, minutes, hours, day_or_date, day_date);
    if(res) return res;
    res = this->writeRegister(REG_SECONDS_ALARM_1, decimal_to_BCD(seconds));
    if(res) return res;
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_1_INT_ENABLE | MASK_INTERRUPT_CONTROL));
    if(res) return res;
    return res;
}


/**
 * The function sets the time on alarm 1 on an RTC device by validating the input parameters, setting the
 * alarm time, enabling the alarm interrupt, and returning the result.
 * 
 * @param minutes The "minutes" parameter is the value of the minutes at which the alarm should
 * trigger.
 * @param hours The "hours" parameter represents the hour value for setting the time alarm. It should
 * be a value between 0 and 23, representing the hours in a 24-hour format.
 * @param day_or_date The "day_or_date" parameter is used to specify
 * whether the alarm should trigger on a specific day of the week or on a specific date of the month.
 * @param day_date The "day_date" parameter is used to specify whether
 * the alarm should trigger on a specific day of the week or on a specific date of the month.
 * 
 * @return an integer value.
 */
int RTC::setTimeAlarm2(uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date)
{
    int res = 0;
    res = this->setTimeAlarm(2, minutes, hours, day_or_date, day_date);
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
 * @return a pointer to a user_alarm_t object.
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
    alarm_1->hours   = this->BCD_to_decimal(alarm_1_regs[2] & MASK_ALARM_HOURS);
    
    alarm_1->day_or_date = (alarm_1_regs[3] & MASK_ALARM_DAY_OR_DATEINV) >> 6;
    if(alarm_1->day_or_date) alarm_1->day_date.day_of_week = (alarm_1_regs[3] & MASK_ALARM_DAY_DATE);
    else alarm_1->day_date.date_of_month = (alarm_1_regs[3] & MASK_ALARM_DAY_DATE);

    return alarm_1;
}

/**
 * The function "getAlarm2" returns a pointer to a user_alarm_t object that contains information about
 * the second alarm.
 * 
 * @return a pointer to a user_alarm_t object.
 */
user_alarm_ptr_t RTC::getAlarm2()
{
    user_alarm_ptr_t alarm_2 (new user_alarm_t);
    uint8_t* alarm_2_regs = this->readRegisters(3, REG_MINUTES_ALARM_2);
    rate_alarm_2 alarm_rate = this->getRateAlarm2(alarm_2_regs);
    alarm_2->rate_alarm.rate_2 = alarm_rate;
    alarm_2->alarm_num = 2;
    alarm_2->minutes = this->BCD_to_decimal(alarm_2_regs[0] & MASK_ALARM_MINUTES);
    alarm_2->hours   = this->BCD_to_decimal(alarm_2_regs[1] & MASK_ALARM_HOURS);
    
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
 * @return an integer value. If the function is successful, it returns 0. If there is an error during
 * the write operation, it returns 1.
 */
int RTC::setRateAlarm1(rate_alarm_1 rate)
{
    int res = 0;
    uint8_t* alarm_regs = this->readRegisters(4, REG_SECONDS_ALARM_1);
    uint8_t A1M1, A1M2, A1M3, A1M4;

    A1M1 = rate & 0b1;
    cout << endl;
    cout << static_cast<int>(alarm_regs[0]) << endl;
    if(A1M1) alarm_regs[0] |= 0x80;
    else alarm_regs[0] &= ~(0x80);
    cout << static_cast<int>(alarm_regs[0]) << endl;
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
 * @return an integer value. If the function is successful, it returns 0. If there is an error during
 * the write operation, it returns 1.
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
 * @return The `snoozeAlarm1()` function returns an integer value, which is the result of writing to
 * the status register to snooze Alarm 1. The return value indicates the success or failure of the
 * operation. If the operation is successful, it will return 0. If there is an error during the write
 * operation, it will return a non-zero value.
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
 * @return The `snoozeAlarm2()` function returns an integer value, which is the result of writing to
 * the status register to snooze Alarm 1. The return value indicates the success or failure of the
 * operation. If the operation is successful, it will return 0. If there is an error during the write
 * operation, it will return a non-zero value.
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
 * @return The `disableAlarm1()` function returns an integer value. This value represents the result of
 * writing to the control register to disable Alarm 1. If the write operation is successful, the
 * function will return 0. If there is an issue with the write operation, it will return a non-zero
 * value, indicating an error.
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
 * @return The function `disableAlarm2()` is returning an integer value. This integer value represents
 * the result of the operation to disable Alarm 2. If the operation is successful, it will return 0. If
 * there is an error or the operation fails, it will return a non-zero value.
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
 * frequency.
 * 
 * @param freq The `freq` parameter in the `enableSquareWave` function is of type `sqw_frequency`,
 * which is an enum representing different frequencies for the square wave output
 * of the Real-Time Clock (RTC) module.
 * `
 * 
 * @return The function `enableSquareWave` is returning an integer value, which is the result of
 * writing the updated control register value to the RTC device. This integer value indicates the
 * success or failure of the operation. If the operation is successful, it will return 0, otherwise it
 * will return a non-zero value.
 */
int RTC::enableSquareWave(sqw_frequency freq)
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);
    control_reg &= ~(MASK_ALARM_1_INT_ENABLE | MASK_ALARM_2_INT_ENABLE | MASK_INTERRUPT_CONTROL | MASK_RATE_SELECT_1 | MASK_RATE_SELECT_2);
    control_reg |= (freq << 3);
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