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
boolean onboardLED; // define flag for onboard LED status.Onboard LED used to give a 'heartbeat' that toggles each run of main loo
const int meterLEDPin = 30; //Digital pin used to monitor Grid Meter LED pulses
const int PWMOutPin = 12; //Immersion heater controller output (PWM)
const int remoteOnPin = 40; //Digital output to turn remote sockets ON
const int remoteOffPin = 46; //Digital output to turn remote sockets OFF
const int redLEDPin = 50; // Red LED (Low power)
const int amberLEDPin = 53; //Amber LED (mid power)
const int greenLEDPin = 52; //Green LED (hign power)
const int PWMIncrement = 1; //Amount to increment PWM setting by when exporting
const int PWMDecrement = 10; //Amount to decrement PWM setting by when importing
//const int loopTime = 10; //Duration of meter test loop() in ms
unsigned long stepTime = 3000; //Duration of time between incremental PWM adjustments (mS)
unsigned long lastLoopStartTime = millis(); // Used to store start time of last loop 
const int maxPWMOut = 240; //Used to scale PWM output - set to just over 90% of Arduino range (90% is supposed to give Kemo full ON
const int minPWMOut = 0; //Used to set minimum PWM level - Kemo OFF at less than 10% mark-space ratio
const int minPWMOn = 77; //PWM value that represents lowest point where power actually starts going to immersion heater (minPWMOut is set at a level to ensure heater is definitely OFF when it should be)
const int redLevel = 120; //PWM level to turn on red LED
const int amberLevel = 160; //PWM level to turn on amber LED
const int greenLevel = 200; //PWM level to turn on green LED
const int remotePushTime = 1500; //Time to "hold down button on Remote)
const int PWMDecrementFactor = 11; //used to provide non-linear, accelerated reduction in PWM Output 
int flashCount = 0; // Used in flash function to count down 
boolean meterLEDOn = false; //Flag: 0 if Grid Meter LED off, 1 if on
boolean newOnCycle = true; // Flag to indicate when the EXPORT LED first illuminates
const int meterLEDOnTime = 500; //If LED has been on for this time (ms), we consider it constantly on
int PWMOut = minPWMOut; //PWM output to Kemo; set to min to be sure power is not immediately applied
int latestMaxPWM = minPWMOn; // Initialise maxPWM out variable
//
int socketsOn = 0; //variable to store the state of the remote sockets - set to OFF initially
unsigned long minSocketOnTime = 120000; // Turning sockets off is not allowed until they've been on for this long (mS)
unsigned long socketLastOnTime; // Used to store the millis() time when sockets turned on
unsigned long minSocketOffTime = 120000; // Turning sockets on is not allowed until they've been off for this long (mS)
unsigned long socketLastOffTime; // Used to store the millis() time when sockets turned off
//
//  Set up for calculating avaerage of recent PWM maximum values
//
const int nRecentPWMs = 4; //Number of recent energy values to store. MUST BE EVEN.
int recentPWMs[nRecentPWMs]; // Create array to store PWM readings
int recentPWMsIndex = 0; // initialise pointer to PWM array
float newAveragePWM = minPWMOn; // variable to store calculated value of most recent PWM average - initialise to minimim PWM value
float oldAveragePWM = minPWMOn; // variable to store calculated value of earlier PWM average - initialise to minimim PWM value
//
// Functions
//
void turnSocketsOn(); //Function to check status of sockets and turn ON if OFF
void turnSocketsOff(); //Function to check status of sockets and turn OFF if ON
//void flashPWM(); //Function to flash PWM level to onboard LED
//int meterTest(int export); //Function to check if Meter LED is really on (based on meterLEDOnTime setting)
//
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
//
// Prime Rolling Average Array with values
//
  for(unsigned i = 0; i < nRecentPWMs; ++i) // Initiatise array for storing energy history (used in rolling average calculation)
  {
    recentPWMs[i] = minPWMOn;
  }
Serial.println("Solar_Immersion 13/02/2016");
}  
void loop()
{
  if((millis()-lastLoopStartTime) > stepTime)
  {
    lastLoopStartTime = millis(); // Reset stored main loop start time
// Toggle Onboard LED each loop
//
    onboardLED = !onboardLED;
    digitalWrite(13,onboardLED);
// 
// print the results to the serial monitor: in CSV Format for cut and paste into Excel
// Sequence is: PWMOut, meterLEDOn, socketsOn
//      
  Serial.print("PWMOut = ");
  Serial.print(PWMOut);
  Serial.print(", meterLEDOn = ");
  Serial.print(meterLEDOn);
  Serial.print(", newOnCycle = ");
  Serial.print(newOnCycle);
  Serial.print(", latestMaxPWM = ");
  Serial.print(latestMaxPWM);
  Serial.print(", socketsOn = ");
  Serial.print(socketsOn);
  if (PWMOut > redLevel)
  {
    Serial.print(" ");
    Serial.print("R");
  }
  else
  {
    Serial.print("  ");    
  }
if (PWMOut > amberLevel)
  {
    Serial.print("A");
  }
if (PWMOut > greenLevel)
  {
    Serial.print("G");
  }
  Serial.println(" ");
 //
 // Set meterLEDOn Flag
 //
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
 //
 //
 //
 if (meterLEDOn)
    {
      if (PWMOut >= maxPWMOut)
        {
          PWMOut = maxPWMOut;
        }
      else
        {
          if(newOnCycle)
            {
              PWMOut = latestMaxPWM - PWMDecrement;
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
          latestMaxPWM = PWMOut;
          newOnCycle = false;
    }
  else
    {
      PWMOut = minPWMOut;
      newOnCycle = true;
      turnSocketsOff();
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
//  flashPWM();
  delay(stepTime);
  }
  else
  {
    // Come here to do nothing, if stepTime has not yet elapsed    
  }
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
    socketLastOnTime = millis(); // update the time that sockets were last demanded to be on
  }
  else
  {
    if((millis() - socketLastOffTime) > minSocketOffTime)
    {
    digitalWrite(remoteOnPin, HIGH);
    delay(remotePushTime);
    digitalWrite(remoteOnPin, LOW);
    socketsOn = 1;
    socketLastOnTime = millis(); // store the time that sockets were turned on
    }
  }
}
//
void turnSocketsOff() //Checks aleady ON and if not sends pulse to remote
{
  if(socketsOn < 1) 
  {
    socketLastOffTime = millis(); // update the time that sockets were last demanded to be off
  }
  else
  {
    if ((millis() - socketLastOnTime) > minSocketOnTime)
    {
    digitalWrite(remoteOffPin, HIGH);
    delay(remotePushTime);
    digitalWrite(remoteOffPin, LOW);
    socketsOn = 0;
    socketLastOffTime = millis(); // store the time that sockets were turned off
    }
  }
}
//
//void flashPWM()
//{
// flashCount = PWMOut + 1;
// while (flashCount > minPWMOut)
//  {
//    digitalWrite(13, HIGH);
//    delay(150);
//    digitalWrite(13,LOW);
//    delay(100);
//    flashCount = flashCount - 10;
//  }
//}

