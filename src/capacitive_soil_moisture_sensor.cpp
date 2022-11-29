
#include <Arduino.h>
#include <capacitive_soil_moisture_sensor.h>


//const int dry = 684; // value for dry sensor
//const int wet = 394; // value for wet sensor

capacitive_soil_moisture_sensor::capacitive_soil_moisture_sensor()
{

}

int capacitive_soil_moisture_sensor::value()
{

    return analogRead(A0);
}

int capacitive_soil_moisture_sensor::percentage()
{
    return map(value(), get.wet_val, get.dry_val, 100, 0); 
}