#ifndef NVMData_h
#define NVMData_h

#include <Arduino.h>
#include <Preferences.h>

class NVMData
{
private:
    const char* prefDefaultValue = "none";

    const char* prefKeySSID = "ssid";
    const char* prefKeyPSK = "psk";
    const char* prefKeyNamespace = "battery";
    const char* prefKeyOperatingHourCounter = "OHC";
    const char* prefKeyUID = "UID";
    const char* prefKeyIID = "IID";
    const char* prefKeyPID = "PID";
    const char* prefKeyIOffset = "IOFF";
    const int prefKeyPowerSourceSerialDefault = 0;
    const uint32_t prefKeyOffsetDefault = 0;

    String NetName = "";
    bool NetNameChanged = false;
    bool NetNameValid = false;
    String NetPassword = "";
    bool NetPasswordChanged = false;
    bool NetPasswordValid = false;
    int PV_U_ID = 0;
    int PV_I_ID = 0;
    int PV_P_ID = 0;
    int CurrentOffset = 0;
    NVMData(/* args */);
public:
    static NVMData& get()
    {
        static NVMData nonVolatileData;
        return nonVolatileData;
    }
    void Init();
    void StoreNetData();
    void DeleteNetData();
    void SetNetData(String newNetName, String newNetPassword);
    void SetDisplayIP(String newDisplayIP);
    void SetCCUIDs(int U_ID, int I_ID, int P_ID);
    void SetCurrentOffset(int offset);
    String GetNetName();
    String GetNetPassword();
    int GetPvUid();
    int GetPvIid();
    int GetPvPid();
    int GetCurOff();
    
    bool NetDataValid();
};

void NVMData::Init() 
{
    Preferences preferences;
    preferences.begin(prefKeyNamespace, false);
    NetName = preferences.getString(prefKeySSID, prefDefaultValue);
    NetPassword = preferences.getString(prefKeyPSK, prefDefaultValue);
    PV_U_ID = preferences.getInt(prefKeyUID, 0);
    PV_I_ID = preferences.getInt(prefKeyIID, 0);
    PV_P_ID = preferences.getInt(prefKeyPID, 0);
    CurrentOffset = preferences.getInt(prefKeyIOffset, 0);
    preferences.end();
    if (NetName != prefDefaultValue)
    {
        NetNameValid = true;
    }
    if (NetPassword != prefDefaultValue)
    {
        NetPasswordValid = true;
    }
}
bool NVMData::NetDataValid()
{
    bool retVal = true;
    if (NetNameValid == false)
    {
        retVal = false;
    }
    if (NetPasswordValid == false)
    {
        retVal = false;
    }
    return retVal;
}
void NVMData::SetNetData(String newNetName, String newNetPassword)
{
    if (NetName != newNetName)
    {
      NetName = newNetName;
      NetNameChanged = true;
    }
    if (NetPassword != newNetPassword)
    {
      NetPassword = newNetPassword;
      NetPasswordChanged = true;
    }
}
void NVMData::SetCCUIDs(int U_ID, int I_ID, int P_ID) {
    PV_U_ID = U_ID;
    PV_I_ID = I_ID;
    PV_P_ID = P_ID;
    Preferences preferences;
    preferences.begin(prefKeyNamespace, false);
    preferences.putInt(prefKeyUID, PV_U_ID);
    preferences.putInt(prefKeyIID, PV_I_ID);
    preferences.putInt(prefKeyPID, PV_P_ID);
    preferences.end();
}
void NVMData::SetCurrentOffset(int offset) {
    CurrentOffset = offset;
    Preferences preferences;
    preferences.begin(prefKeyNamespace, false);
    preferences.putInt(prefKeyIOffset, CurrentOffset);
    preferences.end();
}
String NVMData::GetNetName()
{
    return NetName;
}
String NVMData::GetNetPassword()
{
    return NetPassword;
}
int NVMData::GetPvUid() {
    return PV_U_ID;
}
int NVMData::GetPvIid() {
    return PV_I_ID;
}
int NVMData::GetPvPid() {
    return PV_P_ID;
}
int NVMData::GetCurOff() {
    return CurrentOffset;
}
void NVMData::StoreNetData() {
    Preferences preferences;
    preferences.begin(prefKeyNamespace, false);
    if (NetNameChanged == true)
    {
        preferences.putString(prefKeySSID, NetName);
        NetNameChanged = false;
    }
    if (NetPasswordChanged == true)
    {
        preferences.putString(prefKeyPSK, NetPassword);
        NetPasswordChanged = false;
    }
    preferences.end();
}
void NVMData::DeleteNetData() {
    Preferences preferences;
    preferences.begin(prefKeyNamespace, false);
    preferences.remove(prefKeyPSK);
    preferences.remove(prefKeySSID);
    preferences.end();
    NetName = "";
    NetPassword ="";
}

NVMData::NVMData(/* args */)
{
}

#endif