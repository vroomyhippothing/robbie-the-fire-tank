# 1 "C:\\Users\\Joshua\\AppData\\Local\\Temp\\tmpxsq4cy85"
#include <Arduino.h>
# 1 "C:/Users/Joshua/Desktop/robbie-the-fire-tank/RCMv2/RCMv2.ino"


#include "rcm.h"
#include <ESP32_easy_wifi_data.h>
#include <JMotor.h>

const int dacUnitsPerVolt = 380;
JVoltageCompMeasure<10> voltageComp = JVoltageCompMeasure<10>(batMonitorPin, dacUnitsPerVolt);


JMotorDriverEsp32L293 lMotorDriver = JMotorDriverEsp32L293(portA);
JMotorDriverEsp32L293 rMotorDriver = JMotorDriverEsp32L293(portB);

const float lMotorCompDirectValue = 1;
const float rMotorCompDirectValue = 1;
JMotorCompDirect lMotorCompensator = JMotorCompDirect(lMotorCompDirectValue);
JMotorCompDirect rMotorCompensator = JMotorCompDirect(rMotorCompDirectValue);

JMotorControllerOpen lMotorController = JMotorControllerOpen(lMotorDriver, lMotorCompensator);
JMotorControllerOpen rMotorController = JMotorControllerOpen(rMotorDriver, rMotorCompensator);

JDrivetrainTwoSide drivetrain = JDrivetrainTwoSide(lMotorController, rMotorController, 2);

JDrivetrainControllerBasic drive = JDrivetrainControllerBasic(drivetrain, { INFINITY, 0, INFINITY }, { 2, 0, 2 }, { INFINITY, 0, INFINITY }, false);

float speed = 0;
float turn = 0;
float trim = 0;
void Enabled();
void Enable();
void Disable();
void PowerOn();
void Always();
void configWifi();
void WifiDataToParse();
void WifiDataToSend();
void setup();
void loop();
#line 30 "C:/Users/Joshua/Desktop/robbie-the-fire-tank/RCMv2/RCMv2.ino"
void Enabled()
{
    lMotorCompensator.setMultiplier(1 - trim);
    rMotorCompensator.setMultiplier(1 + trim);
    JTwoDTransform move = { speed, 0, turn };
    move = JDeadzoneRemover::calculate(move, { .4, 0, .4 }, { 1, 1, 1 }, { 0.05, 0, 0.05 });
    drive.moveVel(move);
}

void Enable()
{

    drive.enable();
}

void Disable()
{

    drive.disable();
}

void PowerOn()
{

}

void Always()
{


    drive.run();

    delay(1);
}

void configWifi()
{
    EWD::mode = EWD::Mode::connectToNetwork;
    EWD::routerName = "router";
    EWD::routerPassword = "password";
    EWD::routerPort = 25210;

    EWD::signalLossTimeout = 150;
}

void WifiDataToParse()
{
    enabled = EWD::recvBl();

    speed = EWD::recvFl();
    trim = EWD::recvFl();
    turn = -EWD::recvFl();
}
void WifiDataToSend()
{
    EWD::sendFl(voltageComp.getSupplyVoltage());

}



void setup()
{
    Serial.begin(115200);
    pinMode(ONBOARD_LED, OUTPUT);
    PowerOn();
    Disable();
    configWifi();
    EWD::setupWifi(WifiDataToParse, WifiDataToSend);
}

void loop()
{
    EWD::runWifiCommunication();
    if (!EWD::wifiConnected || EWD::millisSinceMessage() > EWD::signalLossTimeout * 2) {
        enabled = false;
    }
    Always();
    if (enabled && !wasEnabled) {
        Enable();
    }
    if (!enabled && wasEnabled) {
        Disable();
    }
    if (enabled) {
        Enabled();
        digitalWrite(ONBOARD_LED, millis() % 500 < 250);
    } else {
        if (!EWD::wifiConnected)
            digitalWrite(ONBOARD_LED, millis() % 1000 <= 100);
        else if (EWD::timedOut())
            digitalWrite(ONBOARD_LED, millis() % 1000 >= 100);
        else
            digitalWrite(ONBOARD_LED, HIGH);
    }
    wasEnabled = enabled;
}