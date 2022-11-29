#ifndef capacitive_soil_moisture_sensor_h
#define capacitive_soil_moisture_sensor_h



class capacitive_soil_moisture_sensor
{
public:

    /**
     * @brief get struct holds all the data collected from the CTRL and is populated using the update() API
     */
    struct
    {
        int wet_val = 394;
        int dry_val = 670; //684;
    } get;


    /**
     * @brief Construct a new capacitive_soil_moisture_sensor object
     *
     * 
     */
    capacitive_soil_moisture_sensor();


    /**
     * @brief Get the percentage from the sensor
     */
    int percentage();


    /**
     * @brief Get the value from the sensor
     */
    int value();



private:


    /**
     * @brief Set the Data that are going to be sent
     */




    


};

#endif  // capacitive_soil_moisture_sensor_h
