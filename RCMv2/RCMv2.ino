//   This program uses  https://github.com/rcmgames/RCMv2
//   for information about the electronics, see the link at the top of this page: https://github.com/RCMgames
#include "rcm.h" //defines pins
#include <ESP32_easy_wifi_data.h> //https://github.com/joshua-8/ESP32_easy_wifi_data >=v1.0.0
#include <JMotor.h> //https://github.com/joshua-8/JMotor

const int dacUnitsPerVolt = 380; // increasing this number decreases the calculated voltage
JVoltageCompMeasure<10> voltageComp = JVoltageCompMeasure<10>(batMonitorPin, dacUnitsPerVolt);
// set up motors and anything else you need here
// https://github.com/joshua-8/JMotor/wiki/How-to-set-up-a-drivetrain
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
    // turn on outputs
    drive.enable();
}

void Disable()
{
    // shut off all outputs
    drive.disable();
}

void PowerOn()
{
    // runs once on robot startup, set pin modes and use begin() if applicable here
}

void Always()
{
    // always runs if void loop is running, JMotor run() functions should be put here
    // (but only the "top level", for example if you call drivetrainController.run() you shouldn't also call motorController.run())
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
    // add data to read here: (EWD::recvBl, EWD::recvBy, EWD::recvIn, EWD::recvFl)(boolean, byte, int, float)
    speed = EWD::recvFl();
    trim = EWD::recvFl();
    turn = -EWD::recvFl();
}
void WifiDataToSend()
{
    EWD::sendFl(voltageComp.getSupplyVoltage());
    // add data to send here: (EWD::sendBl(), EWD::sendBy(), EWD::sendIn(), EWD::sendFl())(boolean, byte, int, float)
}

////////////////////////////// you don't need to edit below this line ////////////////////

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
        digitalWrite(ONBOARD_LED, millis() % 500 < 250); // flash, enabled
    } else {
        if (!EWD::wifiConnected)
            digitalWrite(ONBOARD_LED, millis() % 1000 <= 100); // short flash, wifi connection fail
        else if (EWD::timedOut())
            digitalWrite(ONBOARD_LED, millis() % 1000 >= 100); // long flash, no driver station connected
        else
            digitalWrite(ONBOARD_LED, HIGH); // on, disabled
    }
    wasEnabled = enabled;
}
