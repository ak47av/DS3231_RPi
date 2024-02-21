# DS3231_RPi
C++ helper library for DS3231 RTC Module

# API implemented and validated 
- Read Time from RTC
- Set RPi System time to RTC
- Set Alarms 1 and 2 on RTC
- Turn alarms on and off
- Set Rate of Alarms
- Read temperature from RTC
- 1Hz Square wave generation

# API implemented but failed
An attempt was made to generate the square waves of frequencies 1Hz, 1KHz, 4KHz and 8KHz. However only 1Hz was enabled even with the other configurations (the suspicion is that the chip on the RTC module may be a clone that does not conform to the Maxim specifications).

# Tests that can be run
#### TEST_WITH_MQTT
This test connects to an MQTT broker and sends the temperature from the RTC every minute to the broker.

#### TEST_TIME_API
This tests the functions:
- `user_time_ptr_t getTime();`
- `int setTime(uint8_t seconds=0, uint8_t minutes=0, CLOCK_FORMAT, clock_12_hr=FORMAT_0_23, AM_OR_PM am_pm=AM, uint8_t hours=0, uint8_t day_of_week=1, uint8_t date_of_month=1, uint8_t month=1, uint8_t year=0);`
- `int setCurrentTimeToRTC(CLOCK_FORMAT clock_12_hr);`

#### TEST_ALARM_API
This tests the functions:
- `int setTimeAlarm1(uint8_t seconds=0, uint8_t minutes=0, CLOCK_FORMAT clock_12_hr=FORMAT_0_23, AM_OR_PM am_pm=AM, uint8_t hours=0, DAY_OR_DATE day_or_date=DAY_OF_WEEK, uint8_t day_date=1);`
- `int setTimeAlarm2(uint8_t minutes=0, CLOCK_FORMAT clock_12_hr=FORMAT_0_23, AM_OR_PM am_pm=AM, uint8_t hours=0, DAY_OR_DATE day_or_date=DAY_OF_WEEK, uint8_t day_date=1);`
- `user_alarm_ptr_t getAlarm1();`
- `user_alarm_ptr_t getAlarm2();`
- `int setRateAlarm1(rate_alarm_1 rate);`
- `int setRateAlarm2(rate_alarm_2 rate);`

#### TEST_ALARM_EVERY_SECOND
This tests Alarm 1 which can trigger an alarm every second by testing the following APIs:
- `int snoozeAlarm1();`
- `int snoozeAlarm2();`
- `int enableInterruptAlarm1();`
- `int enableInterruptAlarm2();`
- `int disableInterruptAlarm1();`
- `int disableInterruptAlarm2();`

#### TEST_ALARM_EVERY_MINUTE
This tests Alarm 1 which can trigger an alarm every second by testing the following APIs:
- `int snoozeAlarm1();`
- `int snoozeAlarm2();`
- `int enableInterruptAlarm1();`
- `int enableInterruptAlarm2();`
- `int disableInterruptAlarm1();`
- `int disableInterruptAlarm2();`

#### TEST_TEMPERATURE
This tests the temperature reading functionality by printing out the temperature to the console by using the API:
- `float getTemperature();`

#### TEST_SQW
This tests the Square wave functionality by enabling a 1Hz wave on the INT/SQW pin, using the API:
- `int enableSquareWave(sqw_frequency freq);`

# Novel functionality
MQTT implemented to send Temperature values from the test application to the MQTT broker in the LAN