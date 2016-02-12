// 
// 2014-04-15 - increased max PWM output level 'cos volts on immersion were not reaching 230V 
// 2014-03-08
// Changes:
// Increased remote transmit time to 1.5 seconds to try to improve reliability
// Corrected error stopping Traffic Lights working in sequence
// Added minPWMOn parameter to enable PWM output to jump straight from it's absolutely OFF value (minPWMOut) to a level that delivers power to the heater as soon as solar export is detected
// Confirmed that sockets come on at Amber level and off when solar import detected
//
// Based on Kemo PWM mains controller and Grid Meter LED input signals only.
//
// Now includes two relays for driving remote socket switch on via Byron controller
//
// 1/10/2014 - Removed Bargraph and CT Code. Added "meterTest" section to test if Grid Meter LED is really on. It does this by checking if it stays on for > meterLEDOnTime
// test change
//
const int meterLEDPin = 30; //Digital pin used to monitor Grid Meter LED pulses
const int PWMOutPin = 12; //Immersion heater controller output (PWM)
const int remoteOnPin = 40; //Digital output to turn remote sockets ON
const int remoteOffPin = 46; //Digital output to turn remote sockets OFF
const int redLEDPin = 50; // Red LED (Low power)
const int amberLEDPin = 53; //Amber LED (mid power)
const int greenLEDPin = 52; //Green LED (hign power)
const int PWMIncrement = 1; //Amount to increment PWM setting by when exporting
const int PWMDecrement = 2; //Amount to decrement PWM setting by when importing
//const int loopTime = 10; //Duration of meter test loop() in ms
const int stepTime = 15000; //Duration of time between incremental PWM adjustments
const int maxPWMOut = 255; //Used to scale PWM output - set to just over 90% of Arduino range (90% is supposed to give Kemo full ON
const int minPWMOut = 44; //Used to set minimum PWM level - Kemo OFF at less than 10% mark-space ratio
const int minPWMOn = 82; //PWM value that represents lowest point where power actually starts going to immersion heater (minPWMOut is set at a level to ensure heater is definitely OFF when it should be)
const int redLevel = 82; //PWM level to turn on red LED
const int amberLevel = 120; //PWM level to turn on amber LED
const int greenLevel = 184; //PWM level to turn on green LED
const int remotePushTime = 1500; //Time to "hold down button on Remote)
const int PWMDecrementFactor = 11; //used to provide non-linear, accelerated reduction in PWM Output 
int flashCount = 0; // Used in flash function to count down 
boolean meterLEDOn = false; //Flag: 0 if Grid Meter LED off, 1 if on
const int meterLEDOnTime = 800; //If LED has been on for this time (ms), we consider it constantly on
int PWMOut = minPWMOut; //PWM output to Kemo; set to min to be sure power is not immediately applied
int socketsOn = 0; //variable to store the state of the remote sockets - set to OFF initially
// Functions
void turnSocketsOn(); //Function to check status of sockets and turn ON if OFF
void turnSocketsOff(); //Function to check status of sockets and turn OFF if ON
void flashPWM(); //Function to flash PWM level to onboard LED
//int meterTest(int export); //Function to check if Meter LED is really on (based on meterLEDOnTime setting)
void setup()
{
  Serial.begin(9600); //start serial comms
  pinMode(meterLEDPin, INPUT);
  pinMode(PWMOutPin, OUTPUT);
  pinMode(remoteOnPin, OUTPUT);
  pinMode(remoteOffPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(amberLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(13, OUTPUT); // Set for use as Heart Beat & flash
}
void loop()
{
   // print the results to the serial monitor: in CSV Format for cut and paste into Excel
   // Sequence is: PWMOut, meterLEDOn, socketsOn
  //      
  Serial.print(PWMOut);
  Serial.print(", ");
  Serial.print(meterLEDOn);
  Serial.print(", ");
  Serial.println(socketsOn);
 // meterLEDOn = false; //Reset meterLEDOn to 'Importing' before re-testing it
    if (digitalRead(meterLEDPin)) 
    {
        delay(meterLEDOnTime);
        if (digitalRead(meterLEDPin))
        {
          meterLEDOn = true;
        }
        else
        {
          meterLEDOn = false;
        }
    }
    else
    {
      meterLEDOn = false;
    }
    if (meterLEDOn)
    {
     if (PWMOut > maxPWMOut)
     {
       PWMOut = maxPWMOut;
     }
     else
     {
        if (PWMOut < minPWMOn)
        {
          PWMOut = minPWMOn;        
        }
        else  
        {
          PWMOut = PWMOut + PWMIncrement;
        }
      }
   }
   else
   {
      if(PWMOut < minPWMOut)
      {
        PWMOut = minPWMOut;
        turnSocketsOff();
      }
      else
      {
          PWMOut = PWMOut - PWMDecrement;
//        PWMOut = PWMOut - (((PWMOut - minPWMOut)/ PWMDecrementFactor)+1); // Decrement rate, based on linear distance above zero power
      }
    }
  analogWrite(PWMOutPin, PWMOut); //write PWM signal to mains controller
  //
  //Set LED 'Traffic Lights' and Turn on Remote Sockets, if appropriate
  if(PWMOut > redLevel)
   {
     digitalWrite(redLEDPin, HIGH);
   }
  else
   {
    digitalWrite(redLEDPin, LOW);
   }
  if(PWMOut > amberLevel) // At PWM level greater than amberLevel turn on amber LED and fire Sockets
   {
     digitalWrite(amberLEDPin, HIGH);
     turnSocketsOn();
   }
   else
   {
     digitalWrite(amberLEDPin, LOW);
   }
   if(PWMOut > greenLevel) 
   {
     digitalWrite(greenLEDPin, HIGH);
   }
   else
   {
     digitalWrite(greenLEDPin, LOW);
   }
  flashPWM();
  delay(stepTime);
}
//
//End of main loop
//
//FUNCTIONS
//
void turnSocketsOn() //Checks if already ON and if not sends pulse to remote
{
  if(socketsOn > 0) 
  {
    // Do Nothing
  }
  else
  {
    digitalWrite(remoteOnPin, HIGH);
    delay(remotePushTime);
    digitalWrite(remoteOnPin, LOW);
    socketsOn = 1;
  }
}
//
void turnSocketsOff() //Checks aleady ON and if not sends pulse to remote
{
  if(socketsOn < 1) 
  {
    // Do Nothing
  }
  else
  {
    digitalWrite(remoteOffPin, HIGH);
    delay(remotePushTime);
    digitalWrite(remoteOffPin, LOW);
    socketsOn = 0;
  }
}
//
void flashPWM()
{
 flashCount = PWMOut + 1;
 
 while (flashCount > minPWMOut)
  {
    digitalWrite(13, HIGH);
    delay(150);
    digitalWrite(13,LOW);
    delay(100);
    flashCount = flashCount - 10;
  }
}

