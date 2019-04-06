#ifndef DS_HCSR04
#define DS_HCSR04

#include <Arduino.h>

#define DEBUG(x) //x

#define HCSR04__SAMPLE_SIZE  16
#define HCSR04__SAMPLE_COUNT 6


typedef struct
{
  unsigned long data;
  int age;
} SampleData_Type;

typedef void (*function_ptr)(void);

class HCSR04
{
public:

    HCSR04(int pinPulse, int pinTrig, unsigned int interval);
    ~HCSR04() {}

    void begin(function_ptr handler);
    void loop(double Tc);
    unsigned long getLast();
    double getDistance();
    String getSensorCSV();
    SampleData_Type getValue(int i);
    void ICACHE_RAM_ATTR pulseISR();
    int getPulsePin();

private:
    void addElement(unsigned long value);

    int trigPin;
    int pulsePin;

    SampleData_Type samples[HCSR04__SAMPLE_SIZE];
    unsigned int sensorInterval;
    double distanceMedian;

    volatile unsigned long pulseStart = 0;
    volatile unsigned long pulseEnd = 1;

    volatile bool pulseHappened = false;

    volatile unsigned  long startMeasurementTime = 30000;

};


#endif
