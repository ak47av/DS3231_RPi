#ifndef RTC_H_
#define RTC_H_

#include "../I2C/I2CDevice.h"
#include <unistd.h>
#include <ctime>
#include <memory>


#define REG_TIME_SECONDS 0x00
#define REG_TIME_MINUTES (REG_TIME_SECONDS + 1)
#define REG_TIME_HOURS   (REG_TIME_SECONDS + 2)
#define REG_TIME_DAY_OF_WEEK (REG_TIME_SECONDS + 3)
#define REG_TIME_DATE_OF_MONTH (REG_TIME_SECONDS + 4)
#define REG_TIME_MONTH (REG_TIME_SECONDS + 5)
#define REG_TIME_YEAR (REG_TIME_SECONDS + 6)

#define REG_TEMPERATURE_MSB 0x11
#define REG_TEMPERATURE_LSB 0x12


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

public:
    RTC(unsigned int bus, unsigned int device);
    user_time_ptr_t readTime();
    int writeTime(user_time_ptr_t t);
    void writeCurrentTimeToRTC();
};

#endif