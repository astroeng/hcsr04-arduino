
#include <Arduino.h>
#include "hcsr04.h"
#include <limits.h>


HCSR04::HCSR04(int pinPulse, int pinTrig, unsigned int interval)
{
    trigPin        = pinTrig;
    pulsePin       = pinPulse;
    sensorInterval = interval;

    for (int i = 0; i < HCSR04__SAMPLE_SIZE; i++)
    {
        samples[i].data = 0;
        samples[i].age = 0;
    }
}

void HCSR04::begin(function_ptr handler)
{
    pinMode(trigPin, OUTPUT);
    pinMode(pulsePin, INPUT);
    digitalWrite(trigPin, LOW);

    attachInterrupt(pulsePin, handler, CHANGE);
}

// uS to distance conversion for ultrasonic range sensor.
// (331.3 + 0.606 * Tc) [M/S] / 1000000.0 * 100.0 / 2.54 [inch/uS] / 2.0 [roundTrip Correction]

// For microcontrollers I try and make the code as non blocking
// as practically possible. In the function below the kick off
// off the measurement is the only delay needed to perform the
// measurement of the ultrasonic sensor. The measurement of the
// pulse duration is performed by an ISR.

void HCSR04::loop(double Tc)
{

    // Kickoff a measurement every interval.
    if (millis() > startMeasurementTime)
    {
        // Setting pulseStart and pulseEnd here has the added benefit of
        // resetting the ISR logic if at startup the ISR fires on an extra
        // falling edge. This seems to have occured during certain startup
        // conditions.
        pulseStart = micros();
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);
        pulseEnd = micros();

        DEBUG(Serial.println("pulseRequest," + String(pulseEnd - pulseStart)));

        startMeasurementTime += sensorInterval;
    }

    if (pulseHappened)
    {
        DEBUG(unsigned long start = micros());

        double soundSpeed = (331.3 + 0.606 * Tc) / 1000000.0 * 100.0 / 2.54 / 2.0;
        addElement(pulseDuration);
        pulseHappened = false;
        distanceMedian = getDistance() * soundSpeed;

        DEBUG(Serial.println("pulseHappened," + String(micros() - start) + " " + String(pulseDuration)));
    }
}

unsigned long HCSR04::getLast()
{
    for (int i = 0; i < HCSR04__SAMPLE_SIZE; i++)
    {
        if (samples[i].age == HCSR04__SAMPLE_SIZE-1)
        {
            return samples[i].data;
        }
    }
    return 0;
}

double HCSR04::getDistance()
{
    for (int i = 0; i < HCSR04__SAMPLE_SIZE-1; i++)
    {
        for (int j = i+1; j < HCSR04__SAMPLE_SIZE; j++)
        {
            if (samples[i].data > samples[j].data)
            {
                SampleData_Type temp2 = samples[i];
                samples[i] = samples[j];
                samples[j] = temp2;
            }
        }
    }

    unsigned long value = 0;
    int valueCount = 0;

    for (int i = (HCSR04__SAMPLE_SIZE/2)-(HCSR04__SAMPLE_COUNT/2);
             i < (HCSR04__SAMPLE_SIZE/2)+(HCSR04__SAMPLE_COUNT/2);
             i++)
    {
        value += samples[i].data;
        valueCount++;
    }

    return ((double)value) / ((double)valueCount);
}

String HCSR04::getSensorCSV()
{
    String message = "";

    for (int i = 0; i < HCSR04__SAMPLE_SIZE; i++) {
        message += String(samples[i].data) + ",";
    }

    return message;
}

SampleData_Type HCSR04::getValue(int i)
{
    return samples[i];
}

void HCSR04::addElement(unsigned long value)
{
    bool added = false;

    for (int i = 0; i < HCSR04__SAMPLE_SIZE; i++)
    {
        if (samples[i].age <= 0 && !added)
        {
            samples[i].data = value;
            samples[i].age  = HCSR04__SAMPLE_SIZE-1;
            added = true;
        }
        else
        {
            samples[i].age--;
        }
    }
}

void ICACHE_RAM_ATTR HCSR04::pulseISR()
{
    unsigned long top = micros();

    // Note: micros will not increment during the ISR.
    // Apparently only true on specific Arduino platforms.

    // If pulseStart is less than pulseEnd it means that
    // this is the start of a new pulse. Otherwise, this
    // is the end of a pulse.

    if (pulseStart < pulseEnd)
    {
        pulseStart = top;
    }
    else
    {
        pulseEnd = top;
        pulseDuration = pulseEnd - pulseStart;
        pulseHappened = true;
    }

    DEBUG(Serial.println(String(micros()) + " " + String(top) + " " + String(pulseDuration)));

}

int HCSR04::getPulsePin()
{
    return pulsePin;
}
