#include <Arduino.h>

#ifndef _LPP_
#define _LPP_

// Devicess
#define GATEWAY     0
#define STATION     1
#define DATALOGGER  2
#define ALL         3


// Data Size
#define LPP_INT_SIZE       1       // 1 byte
#define LPP_FLOAT_SIZE     2       // 2 byte


class DataEncDec {
    public:
        DataEncDec(uint8_t size);
        ~DataEncDec();
        
        void reset(void);
        uint8_t getSize(void);
        char* getBuffer(void);
        uint8_t copy(uint8_t* buffer);

        float newPrecision(float n, int i);
        
        uint8_t addHeader(uint8_t from, uint8_t to);
        uint8_t addHeader(uint8_t from, uint8_t to, uint8_t ack);
        uint8_t addHeader(uint8_t from, uint8_t to, uint8_t ack, uint8_t settings);
        uint8_t addDate(long value);

        uint8_t addTemp(float value);
        uint8_t addHumi(int value);
        uint8_t addIrrad(float value);
        uint8_t addWindSpeed(float value);
        uint8_t addWindDirection(int value);
        uint8_t addRain(float value);
        uint8_t addVoltage(float value);
        uint8_t addCurrent(float value);
        uint8_t addPower(float value);

        uint8_t getTo(char header);
        uint8_t getFrom(char header);
        uint8_t getACK(char header);
        uint8_t getSettings(char header);

        long getDate(char byte_h, char byte_hm, char byte_lm, char byte_l);

        float getTemp(char byte_h, char byte_l);
        int getHumi(char byte);
        float getIrrad(char byte_h, char byte_l);
        float getWindSpeed(char byte);
        int getWindDirection(char byte);
        float getRain(char byte);
        float getVoltage(char byte_h, char byte_l);
        float getCurrent(char byte);
        float getPower(char byte_h, char byte_m, char byte_l);
    
    private:
        char *buffer;
        uint8_t maxsize;
        uint8_t cursor;
        
        
};


#endif