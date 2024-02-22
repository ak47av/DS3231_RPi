#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <iomanip>
#include <memory>

#include "rtc.h"

using namespace std;

/**
 * The RTC constructor initializes an instance of the RTC class with the specified bus and device
 * parameters.
 * 
 * @param bus Represents the I2C bus number that the RTC device is connected to, either 0 or 1.
 * @param device Represents the device address of the RTC (Real-Time Clock) module. This address is used to communicate with the
 * RTC module over the I2C bus.
 */
RTC::RTC(unsigned int bus, unsigned int device) : I2CDevice(bus, device)
{
}

/**
 * Converts a BCD (Binary-Coded Decimal) value to its decimal equivalent.
 * 
 * @param BCD_value An 8-bit unsigned integer that represents a binary-coded decimal value.
 * 
 * @return the decimal value of the BCD (Binary-Coded Decimal) value passed as an argument.
 */
uint8_t RTC::BCD_to_decimal(uint8_t BCD_value)
{
    uint8_t res = BCD_value & 0xF;          // Extract the BCD nibble
    res += 10 * (BCD_value >> 4);           // Add the nibbles to consecutive order of 10
    return res;
}

/**
 * Reads time data from registers and stores it in a `user_time_t` pointed to by `user_time_ptr_t`
 * 
 * @return A pointer to a memory sage shared pointer user_time_ptr_t
 */
user_time_ptr_t RTC::getTime()
{
    user_time_ptr_t t (new user_time_t);                // Create a new user_time_t with a pointer declaration
    if (t.get() == nullptr)
    {
        cerr << "RTC: NO MEMORY AVAILABLE to allocate user_time_t* t" << endl;
        return nullptr;
    }
    unsigned char *data = this->readRegisters(7, 0x00); // Read from registers 0x00 through 0x07
    t->seconds          = this->BCD_to_decimal(data[0]);// Extract seconds
    t->minutes          = this->BCD_to_decimal(data[1]);// Extract minutes

    // evalute if 12 hr clock or 24 hr clock
    if(data[2] & 0x40) t->clock_12hr = FORMAT_0_12;
    if(t->clock_12hr)
    {
        if((data[2] & 0x20) >> 5) t->am_pm        = PM; // if 1 then PM, else AM
        else t->am_pm       = AM;
        t->hours        = this->BCD_to_decimal(data[2] & 0x1F); // Extract hours (if 12 hour clock)
    }
    else t->hours       = this->BCD_to_decimal(data[2] & 0x3F); // Extract hours (if 24 hour clock)

    t->day_of_week      = this->BCD_to_decimal(data[3]); // Extract the day of the week
    t->date_of_month    = this->BCD_to_decimal(data[4]); // Extract the date of the month
    t->month            = this->BCD_to_decimal(data[5] & 0x1F); // Extract the month
    t->year             = this->BCD_to_decimal(data[6]); // Extract the year (from 2000, e.g, if 2020 then 20)
    delete []data;
    return t;
}

/**
 * Sets the time and date on a real-time clock module.
 * 
 * @param seconds Represents the seconds value of the time you want to set. It should be an integer value between 0 and 59.
 * @param minutes Represents the minutes component of the time you want to set. It should be a value between 0 and 59.
 * @param clock_12_hr If `clock_12_hr` is `FORMAT_0_12`, the time will be set in 12-hour format.
 * @param am_pm Specifies whether the time is in the AM or PM for a 12-hour clock format. It can be `AM` or `PM`.
 * @param hours Represents the hours component of the time to be set. It can be in 24-hour format or 12-hour format based on the `clock_12_hr` parameter.
 * @param day_of_week Specifies the day of the week. It is expected to be provided as an integer value where 1 represents Sunday, 2 represents
 * Monday, and so on until 7 which represents Saturday.
 * @param date_of_month Represents the day of the month. It is expected to be provided as an integer value between 1 and 31 
 * @param month Represents the month of the year. It is
 * expected to be provided as an integer value ranging from 1 to 12, where 1 represents January and 12
 * represents December.
 * @param year Represents the year value that you want
 * to set. This parameter should be provided as an 8-bit unsigned
 * integer representing the year value (e.g., 2022 would be represented as 22)
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTime(uint8_t seconds, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, uint8_t day_of_week, uint8_t date_of_month, uint8_t month, uint8_t year)
{
    int res = this->writeRegister(REG_TIME_SECONDS, this->decimal_to_BCD(seconds)); // Write to 0x00 the seconds
    if(res) return res;
    res = this->writeRegister(REG_TIME_MINUTES, this->decimal_to_BCD(minutes)); // Write to 0x01 the minutes 
    if(res) return res;
    // Check if the clock format is 12 hour or 24 hour
    if(clock_12_hr)
    {
        uint8_t reg_value = this->decimal_to_BCD(hours) | 0x40; // Set the 12 hour clock bit
        if(am_pm) reg_value |= 0x20;                            // if AM, set the PM bit
        res = this->writeRegister(REG_TIME_HOURS, reg_value);   // Write the hours to 0x02
        if(res) return res;
    }
    // if 24 hour clock
    else {
        res = this->writeRegister(REG_TIME_HOURS, this->decimal_to_BCD(hours));
        if(res) return res;
    }
    res = this->writeRegister(REG_TIME_DAY_OF_WEEK, this->decimal_to_BCD(day_of_week)); // Set the day of the week to 0x03
    if(res) return res;
    res = this->writeRegister(REG_TIME_DATE_OF_MONTH, this->decimal_to_BCD(date_of_month)); // Set the date of the month to 0x04
    if(res) return res;
    res = this->writeRegister(REG_TIME_MONTH, this->decimal_to_BCD(month)); // Set the month to 0x05
    if(res) return res;
    res = this->writeRegister(REG_TIME_YEAR, this->decimal_to_BCD(year)); // Set the year to 0x06
    if(res) return res;
    return res;
}

/**
 * Converts a decimal number to binary-coded decimal (BCD) format.
 * 
 * @param decimal The decimal parameter is an unsigned 8-bit integer that represents a decimal number.
 * 
 * @return a uint8_t value, which is the result of converting a decimal number to BCD (Binary-Coded
 * Decimal) format.
 */
uint8_t RTC::decimal_to_BCD(uint8_t decimal)
{
    uint8_t res = (decimal/10) << 4; // Extract the higher BCD nibble
    res |= decimal%10;              // bitwise AND with the lower nibble
    return res;
}

/**
 * Sets the current system time to the RTC
 * 
 * @param clock_12_hr The `clock_12_hr` parameter is used to determine whether the time should be set
 * in 12-hour format or 24-hour format.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setCurrentTimeToRTC(CLOCK_FORMAT clock_12_hr)
{
    time_t now = time(0);                   // Use the time library to get the current time
    struct tm tstruct;                      // stores the time
    tstruct = *localtime(&now);             // convert to local time representation

    int seconds         = tstruct.tm_sec;   // store the seconds
    int minutes         = tstruct.tm_min;   // store the minutes
    AM_OR_PM am_pm      = AM;               // initialize AM or PM
    int hours;
    // if 24 hour clock
    if(!clock_12_hr)
    {
        hours           = tstruct.tm_hour;  // store the hours 
    } 
    // if 12 hour clock
    else 
    {
        if(tstruct.tm_hour > 12)            // if hours > 12, then PM
        {
            hours   = tstruct.tm_hour - 12; // hours in PM
            am_pm   = PM;
        } else
        {
           hours = tstruct.tm_hour;         // hours in AM
           am_pm = AM;
        }
    }
    int day_of_week     = tstruct.tm_wday+1;    // store day of the week
    int date_of_month   = tstruct.tm_mday;      // store the date of the month
    int month           = tstruct.tm_mon + 1;   // store the month
    int year            = tstruct.tm_year % 100;// store the year mod by 100 (0-99)
    
    // Set the time using the private function
    if(this->setTime(seconds, minutes, clock_12_hr, am_pm, hours, day_of_week, date_of_month, month, year))
    {
        cerr << "RTC: Unable to write system time to module" << endl;
        return 1;
    }
    return 0;
}

/**
 * Reads the temperature from a register and stores it as a floating point number
 * 
 * @return the temperature value as a float.
 */
float RTC::getTemperature()
{
    float temp_msb = static_cast<float>(this->readRegister(REG_TEMPERATURE_MSB));   // store the MSB
    unsigned char temp_lsb = this->readRegister(REG_TEMPERATURE_LSB);               // store the LSB
    unsigned char decimal_bits = (temp_lsb & 0xC0) >> 6;                            // get the LSB

    // The minimum temperature measured is 0.25 degree Celsius
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
 * Sets the time alarm based on specified parameters. It is used by setTimeAlarm1 and setTimeAlarm2 functions
 * 
 * @param alarm_num Specifies which alarm to set, either alarm 1 or alarm 2.
 * @param minutes Represents the minutes value for setting the alarm. It should be an integer value between 0 and 59.
 * @param clock_12_hr Specify whether the clock is in 12-hour format or not.
 * @param am_pm Specify whether the alarm time is in the AM or PM for a 12-hour clock format. It is of type `AM_OR_PM`, which
 * represents an enumeration with two possible values - `AM` and `PM`
 * @param hours Represents the hour at which the alarm should trigger.
 * @param day_or_date Specify whether the alarm should be set based on a day of the week or a specific date in the month.
 * @param day_date Represents either the day of the week or the date of the month, depending on the value of the `day_or_date` parameter.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTimeAlarm(uint8_t alarm_num, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, DAY_OR_DATE day_or_date, uint8_t day_date)
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
    // If mode is Day of the week, then set bit 6 of the 0x0A register
    if(day_or_date == DAY_OF_WEEK)
    {
        day_date_to_set |= MASK_ALARM_DAY_OR_DATEINV;
        if(day_date > 7 || day_date < 1)
        {
            cerr << "Day cannot be greater than 7 or lesser than 1" << endl;
            return 1;
        }
    }
    // If mode is date of the month, then clear the bit 6 of the 0x0A register
    else if (day_or_date == DATE_OF_MONTH)
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
        cerr << "If Day then DAY_OF_WEEK, if Date then DATE_OF_MONTH, no other values permitted" << endl;
        return 1;
    }
    day_date_to_set |= decimal_to_BCD(day_date) & 0x3F; // set the day or date value
    unsigned char day_date_to_set_BCD = static_cast<unsigned char>(day_date_to_set);

    int res = 0;
    // if alarm_num==1, the write to the registers of Alarm 1: 0x08, 0x09, 0x0A
    if(alarm_num == 1)
    {
        res = this->writeRegister(REG_MINUTES_ALARM_1, minutesBCD);
        if(res) return res;
        res = this->writeRegister(REG_HOURS_ALARM_1, hoursBCD);
        if(res) return res;
        res = this->writeRegister(REG_DAYS_ALARM_1, day_date_to_set_BCD);
        if(res) return res;
    } 
    // if alarm_num==2, the write to the registers of Alarm 1: 0x0B, 0x0C, 0x0D
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
 * Sets the alarm time for Alarm 1 and enables the A1IE bit
 * 
 * @param seconds Represents the seconds value for setting the alarm. 
 * @param minutes Represents the minutes value at which the alarm should trigger.
 * @param clock_12_hr Specify whether the time format is in 12-hour or 24-hour format.
 * @param am_pm Specify whether the alarm time is in the AM or PM for a 12-hour clock format. It can have the values `AM` or `PM`.
 * @param hours Represents the hour at which the alarm should trigger. 
 * @param day_or_date Specify whether the alarm should trigger based on the day of the week or the date of the month.
 * @param day_date Specify whether the alarm should trigger on a specific day of the week (1-7) or a specific date of the month (1-31),
 * depending on the value of the `day_or_date` parameter
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::setTimeAlarm1(uint8_t seconds, uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, DAY_OR_DATE day_or_date, uint8_t day_date)
{
    if(seconds > 59 || seconds < 0)
    {
        cerr << "Seconds cannot be greater than 59 or less than 0" << endl;
        return 1;
    }
    int res = 0;
    // set the alarm without the minutes first, using the private function
    res = this->setTimeAlarm(1, minutes, clock_12_hr, am_pm, hours, day_or_date, day_date);
    if(res) return res;
    // write the seconds value to 0x07
    res = this->writeRegister(REG_SECONDS_ALARM_1, decimal_to_BCD(seconds));
    if(res) return res;
    // read the contents of the control register, set the INTCN and A1IE bits and write back to RTC
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_1_INT_ENABLE | MASK_INTERRUPT_CONTROL));
    if(res) return res;
    return res;
}

/**
 * Sets the alarm time for alarm 2 and enables the A2IE bit 
 * 
 * @param minutes Specifies the minutes value at which the alarm should trigger.
 * @param clock_12_hr Specifies whether the time format is 12-hour or 24-hour format.
 * @param am_pm Specifies whether the alarm time is in the AM or PM for a 12-hour clock format. It can have the values `AM` or `PM`.
 * @param hours Specifies the hour at which the alarm should trigger.
 * @param day_or_date Specifies whether the alarm should trigger based on the day of the week or the date of the month. 
 * @param day_date Specify whether the alarm should trigger on a specific day of the week (1-7) or a specific date of the month (1-31),
 * 
 * @return 0 if successful, 1 if unsucessful
 */
int RTC::setTimeAlarm2(uint8_t minutes, CLOCK_FORMAT clock_12_hr, AM_OR_PM am_pm, uint8_t hours, DAY_OR_DATE day_or_date, uint8_t day_date)
{
    int res = 0;
    // Set the alarm using the private function
    res = this->setTimeAlarm(2, minutes, clock_12_hr, am_pm, hours, day_or_date, day_date);
    if(res) return res;
    // read the contents of the control register, set the INTCN and A12E bits and write back to RTC
    uint8_t readControlRegister = this->readRegister(REG_CONTROL);
    res = this->writeRegister(REG_CONTROL, (readControlRegister | MASK_ALARM_2_INT_ENABLE | MASK_INTERRUPT_CONTROL));
    if(res) return res;
    return res;
}

/**
 * Returns the rate at which the alarm should occur based on the values in the A1M4, A1M3, A1M2 and A1M1 bits
 * 
 * @param alarm_1_regs Array of uint8_t integers that represents the registers of the alarm 1.
 * 
 * @return the rate at which the alarm 1 is set to trigger. The possible return values are:
 * - ALARM_1_ONCE_PER_DATE_DAY
 * - ALARM_1_ONCE_PER_DAY
 * - ALARM_1_ONCE_PER_HOUR
 * - ALARM_1_ONCE_PER_MINUTE
 * - ALARM_1_ONCE_PER_SECOND
 */
rate_alarm_1 RTC::getRateAlarm1(uint8_t* alarm_1_regs)  // Get the alarm registers from the calling function
{
    uint8_t A1M4 = alarm_1_regs[3] >> 7;            // Extract A1M4
    if(A1M4 == 0) return ALARM_1_ONCE_PER_DATE_DAY; // if A1M4 is zero, then alarm rings once every date or day
    uint8_t A1M3 = alarm_1_regs[2] >> 7;            // Extract A1M3 
    if(A1M3 == 0) return ALARM_1_ONCE_PER_DAY;      // if A1M3 is zero, then alarm rings when the hours, minutes and seconds match
    uint8_t A1M2 = alarm_1_regs[1] >> 7;            // Extract A1M2
    if(A1M2 == 0) return ALARM_1_ONCE_PER_HOUR;     // if A1M2 is zero, then alarm rings when the seconds and minutes match
    uint8_t A1M1 = alarm_1_regs[0] >> 7;            // Extract A1M4
    if(A1M1 == 0) return ALARM_1_ONCE_PER_MINUTE;   // if A1M1 is zero, then alarm rings when the seconds match
    return ALARM_1_ONCE_PER_SECOND;                 // if all 4 bits are set, the alarm rings every minute
}

/**
 * Returns the rate at which the alarm should occur based on the values in the A2M4, A2M3 and A2M2 bits
 * 
 * @param alarm_2_regs Array of uint8_t integers that represents the registers of the alarm 2.
 * 
 * @return the rate at which Alarm 2 is set to trigger. The possible return values are:
 * - ALARM_2_ONCE_PER_DATE_DAY
 * - ALARM_2_ONCE_PER_DAY
 * - ALARM_2_ONCE_PER_HOUR
 * - ALARM_2_ONCE_PER_MINUTE
 */
rate_alarm_2 RTC::getRateAlarm2(uint8_t* alarm_2_regs)
{
    uint8_t A2M4 = alarm_2_regs[2] >> 7;            // Extract A2M4
    if(A2M4 == 0) return ALARM_2_ONCE_PER_DATE_DAY; // if A2M4 is zero, then alarm rings once every date or day
    uint8_t A2M3 = alarm_2_regs[1] >> 7;            // Extract A2M3
    if(A2M3 == 0) return ALARM_2_ONCE_PER_DAY;      // if A2M3 is zero, then alarm rings when hours and minutes match
    uint8_t A2M2 = alarm_2_regs[0] >> 7;            // Extract A2M2
    if(A2M2 == 0) return ALARM_2_ONCE_PER_HOUR;     // if A2M2 is zero, then alarm rings when minutes match
    return ALARM_2_ONCE_PER_MINUTE;                 // if all 3 bits are set, then the alarm rings every minute
}

/**
 * Returns a pointer to a struct containing information about alarm 1.
 * 
 * @return a memory safe shared pointer `user_alarm_ptr_t` to a `user_alarm_t` struct
 */
user_alarm_ptr_t RTC::getAlarm1()
{
    // get the memory safe pointer to new alarm object to store the information
    user_alarm_ptr_t alarm_1 (new user_alarm_t);
    // get the register values in a single read (0x07 through 0x0A)
    uint8_t* alarm_1_regs = this->readRegisters(4, REG_SECONDS_ALARM_1);
    // get the rate of the alarm by giving the function the registers we got in the previous line
    rate_alarm_1 alarm_rate = this->getRateAlarm1(alarm_1_regs);
    alarm_1->rate_alarm.rate_1 = alarm_rate; // Set the rate of the alarm
    alarm_1->alarm_num = 1;                  // Set the alarm number to 1
    // Set the timing of the alarm by extracting the values from the appropriate registers
    alarm_1->seconds = this->BCD_to_decimal(alarm_1_regs[0] & MASK_ALARM_SECONDS);
    alarm_1->minutes = this->BCD_to_decimal(alarm_1_regs[1] & MASK_ALARM_MINUTES);

    // Check if the 12 hour clock bit (bit 6) is set in 0x09
    if(alarm_1_regs[2] & 0x40) alarm_1->clock_12hr = FORMAT_0_12;
    // if the 12 hour clock bit is set
    if(alarm_1->clock_12hr)
    {
        // if the PM bit (bit 5) is set in 0x09, set to PM
        if((alarm_1_regs[2] & 0x20) >> 5) alarm_1->am_pm = PM;
        // if the AM bit (bit 5) is clear in 0x09, set to AM
        else alarm_1->am_pm = AM;
        alarm_1->hours   = this->BCD_to_decimal(alarm_1_regs[2] & 0x1F); // set the hours
    }
    // if it is a 24 hour clock, write the hours straight to the register
    else alarm_1->hours = this->BCD_to_decimal(alarm_1_regs[2] & 0x3F);
    
    // Set the Day of week if bit 6 is set, else set date of month from regsiter 0x0A
    alarm_1->day_or_date = ((alarm_1_regs[3] & MASK_ALARM_DAY_OR_DATEINV) >> 6) ? DAY_OF_WEEK : DATE_OF_MONTH;
    if(alarm_1->day_or_date) alarm_1->day_date.day_of_week = BCD_to_decimal(alarm_1_regs[3] & MASK_ALARM_DAY_DATE);
    else alarm_1->day_date.date_of_month = BCD_to_decimal(alarm_1_regs[3] & MASK_ALARM_DAY_DATE);

    return alarm_1;
}

/**
 * Returns a pointer to a struct containing information about alarm 2.
 * 
 * @return a memory safe shared pointer `user_alarm_ptr_t` to a `user_alarm_t` struct
 */
user_alarm_ptr_t RTC::getAlarm2()
{
    // get the memory safe pointer to new alarm object to store the information
    user_alarm_ptr_t alarm_2 (new user_alarm_t);
    // get the register values in a single read (0x0B through 0x0D)
    uint8_t* alarm_2_regs = this->readRegisters(3, REG_MINUTES_ALARM_2);
    // get the rate of the alarm by giving the function the registers we got in the previous line
    rate_alarm_2 alarm_rate = this->getRateAlarm2(alarm_2_regs);
    alarm_2->rate_alarm.rate_2 = alarm_rate;    // Set the rate of the alarm
    alarm_2->alarm_num = 2;                     // Set the alarm number to 2
    // Set the timing of the alarm by extracting the values from the appropriate registers
    alarm_2->minutes = this->BCD_to_decimal(alarm_2_regs[0] & MASK_ALARM_MINUTES);

    // Check if the 12 hour clock bit (bit 6) is set in 0x0C
    if(alarm_2->clock_12hr)
    {
        // if the PM bit (bit 5) is set in 0x0C, set to PM
        if((alarm_2_regs[1] & 0x20) >> 5) alarm_2->am_pm = PM;
        // if the AM bit (bit 5) is clear in 0x0C, set to AM
        else alarm_2->am_pm = AM;
        alarm_2->hours   = this->BCD_to_decimal(alarm_2_regs[1] & 0x1F); // set the hours
    }
    // if it is a 24 hour clock, write the hours straight to the register
    else alarm_2->hours = this->BCD_to_decimal(alarm_2_regs[1] & 0x3F);
    
    // Set the Day of week if bit 6 is set, else set date of month from regsiter 0x0D
    alarm_2->day_or_date = ((alarm_2_regs[2] & MASK_ALARM_DAY_OR_DATEINV) >> 6) ? DAY_OF_WEEK : DATE_OF_MONTH;
    if(alarm_2->day_or_date) alarm_2->day_date.day_of_week = BCD_to_decimal(alarm_2_regs[2] & MASK_ALARM_DAY_DATE);
    else alarm_2->day_date.date_of_month = BCD_to_decimal(alarm_2_regs[2] & MASK_ALARM_DAY_DATE);

    return alarm_2;
}

/**
 * Sets the rate of the alarm 1. 
 * 
 * @param rate Is of type "rate_alarm_1" representing the desired alarm rate for alarm 1. The function sets the alarm rate by
 * modifying specific bits in the alarm registers.
 * 
 * @return 0 if successful, 1 if unsuccesful
 */
int RTC::setRateAlarm1(rate_alarm_1 rate)   // get the rate of alarm 1 from the enum
{
    int res = 0;
    // extract the data from the registers in a single read
    uint8_t* alarm_regs = this->readRegisters(4, REG_SECONDS_ALARM_1);
    uint8_t A1M1, A1M2, A1M3, A1M4;

    // Set A1M1
    A1M1 = rate & 0b1;
    if(A1M1) alarm_regs[0] |= 0x80;
    else alarm_regs[0] &= ~(0x80);
    // Write back to 0x07
    res = this->writeRegister(REG_SECONDS_ALARM_1,alarm_regs[0]);
    if(res) return 1;

    // Set A1M2
    A1M2 = rate & 0b10;
    if(A1M2) alarm_regs[1] |= 0x80;
    else alarm_regs[1] &= ~(0x80);
    // Write back to 0x08
    res = this->writeRegister(REG_MINUTES_ALARM_1,alarm_regs[1]);
    if(res) return 1;

    // Set A1M3
    A1M3 = rate & 0b100;
    if(A1M3) alarm_regs[2] |= 0x80;
    else alarm_regs[2] &= ~(0x80);
    // Write back to 0x09
    res = this->writeRegister(REG_HOURS_ALARM_1,alarm_regs[2]);
    if(res) return 1;

    // Set A1M4
    A1M4 = rate & 0b1000;
    if(A1M4) alarm_regs[3] |= 0x80;
    else alarm_regs[3] &= ~(0x80);
    // Write back to 0x0A
    res = this->writeRegister(REG_DAYS_ALARM_1,alarm_regs[3]);
    if(res) return 1;
    return 0;
}

/**
/**
 * Sets the rate of the alarm 2. 
 * 
 * @param rate Is of type "rate_alarm_2" representing the desired alarm rate for alarm 2. The function sets the alarm rate by
 * modifying specific bits in the alarm registers.
 * 
 * @return 0 if successful, 1 if unsuccesful
 */
int RTC::setRateAlarm2(rate_alarm_2 rate)
{
    int res = 0;
    // extract the data from the registers in a single read
    uint8_t* alarm_regs = this->readRegisters(3, REG_MINUTES_ALARM_2);
    uint8_t A2M2, A2M3, A2M4;

    // Set A2M2
    A2M2 = rate & 0b1;
    if(A2M2) alarm_regs[0] |= 0x80;
    else alarm_regs[0] &= ~(0x80);
    // Write back to 0x0B
    res = this->writeRegister(REG_MINUTES_ALARM_2,alarm_regs[0]);
    if(res) return 1;

    // Set A2M3
    A2M3 = rate & 0b10;
    if(A2M3) alarm_regs[1] |= 0x80;
    else alarm_regs[1] &= ~(0x80);
    // Write back to 0x0C
    res = this->writeRegister(REG_HOURS_ALARM_2,alarm_regs[1]);
    if(res) return 1;

    // Set A2M4
    A2M4 = rate & 0b100;
    if(A2M4) alarm_regs[2] |= 0x80;
    else alarm_regs[2] &= ~(0x80);
    // Write back to 0x0D
    res = this->writeRegister(REG_DAYS_ALARM_2,alarm_regs[2]);
    if(res) return 1;

    return 0;
}

/**
 * Snoozes Alarm 1 by clearing the A1F flag in the status register.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::snoozeAlarm1()
{
    uint8_t status_reg = this->readRegister(REG_STATUS);    // Read the status register 0x0F
    status_reg &= ~(MASK_ALARM_1_FLAG);                     // Clear the A1F bit
    int res = this->writeRegister(REG_STATUS, status_reg);  // Write the modified value back to 0x0F
    if(res) cerr << "RTC: Unable to snooze Alarm 1" << endl;
    return res;
}

/**
 * Snoozes Alarm 2 by clearing the A2F flag in the status register.
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::snoozeAlarm2()
{
    uint8_t status_reg = this->readRegister(REG_STATUS);    // Read the status register 0x0F
    status_reg &= ~(MASK_ALARM_2_FLAG);                     // Clear the A2F bit
    int res = this->writeRegister(REG_STATUS, status_reg);  // Write the modified value back to 0x0F
    if(res) cerr << "RTC: Unable to snooze Alarm 1" << endl;
    return res;
}

/**
 * This function enables the interrupt on Alarm 1 by setting the A1IE bit
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::enableInterruptAlarm1()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);  // Read the Control register 0x0E
    control_reg |= (MASK_ALARM_1_INT_ENABLE);               // Set the A1IE bit
    int res = this->writeRegister(REG_CONTROL, control_reg);// Write the modified value back to 0x0E
    if(res) cerr << "RTC: Unable to enable Alarm 1" << endl;
    return res;
}

/**
 * This function disables the interrupt on Alarm 1 by clearing the A1IE bit
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::disableInterruptAlarm1()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);  // Read the Control register 0x0E
    control_reg &= ~(MASK_ALARM_1_INT_ENABLE);              // Clear the A1IE bit
    int res = this->writeRegister(REG_CONTROL, control_reg);// Write the modified value back to 0x0E
    if(res) cerr << "RTC: Unable to disable Alarm 1" << endl;
    return res;
}

/**
 * This function enables the interrupt on Alarm 2 by setting the A2IE bit
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::enableInterruptAlarm2()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);  // Read the Control register 0x0E
    control_reg |= (MASK_ALARM_2_INT_ENABLE);               // Set the A2IE bit
    int res = this->writeRegister(REG_CONTROL, control_reg);// Write the modified value back to 0x0E
    if(res) cerr << "RTC: Unable to enable Alarm 2" << endl;
    return res;
}

/**
 * This function disables the interrupt on Alarm 2 by clearing the A2IE bit
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::disableInterruptAlarm2()
{
    uint8_t control_reg = this->readRegister(REG_CONTROL);  // Read the Control register 0x0E        
    control_reg &= ~(MASK_ALARM_2_INT_ENABLE);              // clear the A2IE bit
    int res = this->writeRegister(REG_CONTROL, control_reg);// Write the modified value back to 0x0E
    if(res) cerr << "RTC: Unable to disable Alarm 2" << endl;
    return res;
}

/**
 * Enables a square wave output at a specified frequency on the INT/SQW. It has not been tested (due to the testing chip being different than the original)
 * 
 * @param freq The `freq` parameter in the `enableSquareWave` function is of type `sqw_frequency`,
 * which is an enum representing different frequencies for the square wave output
 * of the RTC module.
 * possible values of freq:
 * - SQW_1HZ 
 * - SQW_1KHZ
 * - SQW_4KHZ
 * - SQW_8KHZ
 * 
 * @return 0 if successful, 1 if unsuccessful
 */
int RTC::enableSquareWave(sqw_frequency freq)
{
    uint8_t control_reg = this->readRegister(REG_CONTROL); // Read the control register 0x0E
    // Clear A1IE, A2IE, INTCN, RS2 and RS1 bits
    control_reg &= ~(MASK_ALARM_1_INT_ENABLE | MASK_ALARM_2_INT_ENABLE | MASK_INTERRUPT_CONTROL | MASK_RATE_SELECT_1 | MASK_RATE_SELECT_2);
    // set RS1 and RS2 to the frequency specified
    control_reg |= (freq << 3);
    // set the BBSQW bit 
    control_reg |= MASK_BAT_BACKUP_SQW_ENABLE;
    // write the modified value back to 0x0E
    int res = this->writeRegister(REG_CONTROL, control_reg);
    if(res) cerr << "RTC: Unable to enable Square wave" << endl;
    return res;
}

/**
 * The RTC destructor closes the RTC object.
 */
RTC::~RTC()
{
    // close the I2C device file associated with the RTC 
    this->close();
}
