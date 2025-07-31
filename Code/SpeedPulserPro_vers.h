/*
Inputs are a 5v/12v square wave input from Can2Cluster or an OEM Hall Sensor and converts it into a PWM signal for a motor.  To get speeds low enough, the motor voltage needs reduced (hence the adjustable LM2596S on the PCB)
from 12v to ~9v.  This allows <10mph readings while still allowing high (160mph) readings.  Clusters supported are 1540 (rotations per mile) =~ (1540*160)/60 = 4100rpm

Default support is for 12v hall sensors from 02J / 02M etc.  According to VW documentation, 1Hz = 1km/h.  Other marques may have different calibrations (adjustable in '_defs.h')

Motor performance plotted with Duty Cycle & Resulting Speed.  Basic Excel located in GitHub for reference - the motor isn't linear so it cannot be assumed that x*y duty = z speed(!)
LED PWM can use various 'bits' for resolution.  8 bit results in a poorer resolution, therefore the speed can be 'jumpy'.  Default is 10 bit which makes it smoother.  Both are available.

Uses 'ESP32_FastPWM' for easier PWM control compared to LEDc
Uses 'RunningMedian' for capturing multiple input pulses to compare against.  Used to ignore 'outliars'

To calibrate or adapt to other models:
> Set 'testSpeed' to 1 & confirm tempSpeed = 0.  This will allow the motor to run through EVERY duty cycle from 0 to 385 (10-bit)
> Monitor Serial Monitor and record in the Excel (under Resulting Speed) the running speed of the cluster at each duty cycle
  > Note: duty cycle is >'100%' due to default 10 bit resolution 
> Copy each resulting speed into 'motorPerformance' 
> Done!

All main adjustable variables are in '_defs.h'.

V1.01 - initial release - using SpeedPulser & C2C as a base
V1.02 - full release - confirmed WiFi and RPM working.  To test CAN
*/