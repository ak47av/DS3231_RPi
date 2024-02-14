#ifndef RTC_H_
#define RTC_H_

#include "../I2C/I2CDevice.h"
#include <unistd.h>
#include <ctime>
#include <memory>

// TIME REGISTERS
#define REG_TIME_SECONDS            0x00
#define REG_TIME_MINUTES            0x01 
#define REG_TIME_HOURS              0x02 
#define REG_TIME_DAY_OF_WEEK        0x03 
#define REG_TIME_DATE_OF_MONTH      0x04
#define REG_TIME_MONTH              0x05 
#define REG_TIME_YEAR               0x06 

// ALARM 1 REGISTERS
#define REG_SECONDS_ALARM_1         0x07
#define REG_MINUTES_ALARM_1         0x08
#define REG_HOURS_ALARM_1           0x09
#define REG_DAYS_ALARM_1            0x0A

// ALARM 2 REGISTERS
#define REG_MINUTES_ALARM_2         0x0B
#define REG_HOURS_ALARM_2           0x0C
#define REG_DAYS_ALARM_2            0x0D

// ALARM REGISTER_MASK
#define MASK_ALARM_SECONDS          0x7F
#define MASK_ALARM_MINUTES          0x7F
#define MASK_ALARM_HOURS            0x1F
#define MASK_ALARM_MODE             0x80
#define MASK_ALARM_DAY_OR_DATEINV   0x40
#define MASK_ALARM_DAY_DATE         0x3F          

// CONTROL REGISTERS
#define REG_CONTROL                 0x0E
#define REG_STATUS                  0x0F
#define REG_AGING_OFFSET            0x10

// CONTROL REGISTER MASKS
#define MASK_ENABLE_OSCILLATOR_INV  0x80
#define MASK_BAT_BACKUP_SQW_ENABLE  0x40
#define MASK_CONV_TEMPERATURE       0x20
#define MASK_RATE_SELECT_2          0x10
#define MASK_RATE_SELECT_1          0x08
#define MASK_INTERRUPT_CONTROL      0x04
#define MASK_ALARM_2_INT_ENABLE     0x02
#define MASK_ALARM_1_INT_ENABLE     0x01

// STATUS REGISTER MASKS
#define MASK_OSCILLATOR_STOP_FLAG   0x80
#define MASK_ENABLE_32KHZ_OUT       0x08
#define MASK_BUSY                   0x04
#define MASK_ALARM_2_FLAG           0x02
#define MASK_ALARM_1_FLAG           0x01

// TEMPERATURE REGISTERS
#define REG_TEMPERATURE_MSB         0x11
#define REG_TEMPERATURE_LSB         0x12

typedef struct user_time_t {
    uint8_t seconds;       
    uint8_t minutes;
    uint8_t hours;
    uint8_t day_of_week;
    uint8_t date_of_month;
    uint8_t month;
    uint8_t year;
} user_time_t;

using user_time_ptr_t = std::shared_ptr<user_time_t>;

class RTC: public EE513::I2CDevice {
private:
    uint8_t BCD_to_decimal(uint8_t BCD_value);
    uint8_t decimal_to_BCD(uint8_t decimal);
    int setAlarm(uint8_t alarm_num, uint8_t minutes, uint8_t hours, uint8_t day_or_date, uint8_t day_date);

public:
    RTC(unsigned int bus, unsigned int device);
    user_time_ptr_t readTime();
    int writeTime(user_time_ptr_t t);
    int writeCurrentTimeToRTC();
    float getTemperature();
    int setAlarm1(uint8_t seconds=0, uint8_t minutes=0, uint8_t hours=0, uint8_t day_or_date=0, uint8_t day_date=1);
    int setAlarm2(uint8_t minutes=0, uint8_t hours=0, uint8_t day_or_date=0, uint8_t day_date=1);
};

#endif