//   This program uses  https://github.com/rcmgames/RCMv2
//   for controlling Robbie the fire tank, a medium sized robot with tank treads, code started Dec. 2022
//   for information about the electronics, see the link at the top of this page: https://github.com/RCMgames
#include "rcm.h" //defines pins
#include <ESP32_easy_wifi_data.h> //https://github.com/joshua-8/ESP32_easy_wifi_data >=v1.0.0
#include <JMotor.h> //https://github.com/joshua-8/JMotor

const int dacUnitsPerVolt = 193; // increasing this number decreases the calculated voltage
JVoltageCompMeasure<100> voltageComp = JVoltageCompMeasure<100>(batMonitorPin, dacUnitsPerVolt);
// set up motors and anything else you need here
// https://github.com/joshua-8/JMotor/wiki/How-to-set-up-a-drivetrain
JMotorDriverEsp32L293 lMotorDriver = JMotorDriverEsp32L293(portA, true, true, false, 500);
JMotorDriverEsp32L293 rMotorDriver = JMotorDriverEsp32L293(portB, true, true, false, 500);

JMotorCompStandardConfig lconfig = JMotorCompStandardConfig(0.2, .03, .35, .45, 1, 1, 65);
JMotorCompStandardConfig rconfig = JMotorCompStandardConfig(0.2, .03, .35, .45, 1, 1, 65);

JMotorCompStandard lMotorCompensator = JMotorCompStandard(voltageComp, lconfig);
JMotorCompStandard rMotorCompensator = JMotorCompStandard(voltageComp, rconfig);

JMotorControllerOpen lMotorController = JMotorControllerOpen(lMotorDriver, lMotorCompensator);
JMotorControllerOpen rMotorController = JMotorControllerOpen(rMotorDriver, rMotorCompensator);

JDrivetrainTwoSide drivetrain = JDrivetrainTwoSide(lMotorController, rMotorController, 2);

JDrivetrainControllerBasic drive = JDrivetrainControllerBasic(drivetrain, { INFINITY, 0, INFINITY }, { INFINITY, 0, INFINITY }, { INFINITY, 0, INFINITY }, true);

float speed = 0;
float turn = 0;
float trim = 0;
JTwoDTransform move = { 0, 0, 0 };

boolean a = false;
boolean b = false;
JMotorDriverEsp32Servo s = JMotorDriverEsp32Servo(port1);
const float sOff = -.7;
const float sOn = 1;
uint32_t enabledPacketCount = 0;
boolean f = false;

void Enabled()
{
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
    // drive.XLimiter.setAccelAndDecelLimits(1, 2.5);
    // drive.ThetaLimiter.setAccelAndDecelLimits(3, 4);
    s.enable();
}

void Always()
{
    if (!enabled) {
        enabledPacketCount = 0;
    }
    // always runs if void loop is running, JMotor run() functions should be put here
    // (but only the "top level", for example if you call drivetrainController.run() you shouldn't also call motorController.run())
    lMotorCompensator.setMultiplier(1 - trim);
    rMotorCompensator.setMultiplier(1 + trim);

    move = { speed, 0, turn };
    move = JDeadzoneRemover::calculate(move, { 0, 0, 0 }, { 1, 0, .5 }, { 0.01, 0, 0.01 });
    drive.moveVel(move);

    drive.run();

    if (a && b && enabled && enabledPacketCount > 200) {
        f = true;
        s.set(sOn);
    } else {
        s.set(sOff);
        f = false;
    }
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
    if (enabled) {
        if (enabledPacketCount < (1 << 30)) { // anti overflow
            enabledPacketCount++;
        }
    } else {
        enabledPacketCount = 0;
    }
    speed = EWD::recvFl();
    trim = EWD::recvFl();
    trim = constrain(trim, -1, 1) / 2;
    turn = -EWD::recvFl();
    a = EWD::recvFl() == 1;
    b = EWD::recvFl() == 1;
}
void WifiDataToSend()
{
    EWD::sendFl(voltageComp.getSupplyVoltage());
    // add data to send here: (EWD::sendBl(), EWD::sendBy(), EWD::sendIn(), EWD::sendFl())(boolean, byte, int, float)
    EWD::sendFl(lMotorController.getDriverSetVal());
    EWD::sendFl(rMotorController.getDriverSetVal());
    EWD::sendFl(f);
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
    if (!EWD::wifiConnected || EWD::millisSinceMessage() > EWD::signalLossTimeout * 3) {
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
