#ifndef DynamicData_h
#define DynamicData_h

#include <Arduino.h>
#include "NVMData.h"

class DynamicData
{
public:
    static const int numberOfErrorMessageHist = 4;
private:
    DynamicData(/* args */);
    unsigned int errorCounter = 0;
    String errorMessageHist[numberOfErrorMessageHist];
    int errorMessagePointer = 0;
public:
    static DynamicData& get()
    {
        static DynamicData globalDynamicData;
        return globalDynamicData;
    }
    void Init();
    // static definitions
    const int MAX_NO_UPDATE = 600;

    String ipaddress = "";
    
    unsigned long epochTime = 0;
    unsigned int uptimeHours = 0;
    int connections = 0;
    String RSSIText = "";
    bool setNewNetwork = false;
    float busVoltage = 0.0;
    float current = 0.0;
    float power = 0.0;
    String lastSendVoltage = "";
    String lastSendCurrent = "";
    String lastSendPower = "";
    float Voltage = 0.0;
    float Current = 0.0;
    float Power = 0.0;
    int MeasureCount = 0;
    
    unsigned int getErrorCounter();
    void incErrorCounter(String message);
    String getErrorHist(int pos);
};

void DynamicData::Init() {
    for (int i = 0; i < numberOfErrorMessageHist; i++)
    {
        errorMessageHist[i] = "";
    }
}
unsigned int DynamicData::getErrorCounter()
{
    return errorCounter;
    this->errorCounter;
}
void DynamicData::incErrorCounter(String message)
{
    errorMessageHist[errorMessagePointer] = message;
    errorMessagePointer++;
    if (errorMessagePointer >= numberOfErrorMessageHist)
    {
        errorMessagePointer = 0;
    }
    errorCounter++;
}
String DynamicData::getErrorHist(int pos)
{
    int arrayPos = pos + errorMessagePointer;
    if (arrayPos >= numberOfErrorMessageHist)
    {
        arrayPos -= numberOfErrorMessageHist;
    }
    if (arrayPos >= numberOfErrorMessageHist)
    {
        return "end";
    }
    return errorMessageHist[arrayPos];
}

DynamicData::DynamicData(/* args */)
{
}

#endif
