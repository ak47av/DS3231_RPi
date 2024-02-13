#include <iostream>

#include "I2C/I2CDevice.h"

using namespace std;

int main()
{
    EE513::I2CDevice RTC(1, 0x68);
    return 0;
}