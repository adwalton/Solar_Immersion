// Solar Immersion 
// 
// Based on Kemo PWM mains controller and Grid Meter LED input signals only.
//
// Includes two relays for driving remote socket switch on via Byron controller
//
// 1/10/2014 - Removed Bargraph and CT Code. Added "meterTest" section to test if Grid Meter LED is really on. It does this by checking if it stays on for > meterLEDOnTime
// test change
//
boolean onboardLED; // define flag for onboard LED status.Onboard LED used to give a 'heartbeat' that toggles each run of main loo
const int piezoPin = 8; // Pin for Piezo Sounder
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
const int minPWMOn = 80; //PWM value that represents lowest point where power actually starts going to immersion heater (minPWMOut is set at a level to ensure heater is definitely OFF when it should be)
const int redLevel = 100; //PWM level to turn on red LED
const int amberLevel = 120; //PWM level to turn on amber LED
const int greenLevel = 140; //PWM level to turn on green LED
const int remotePushTime = 3000; //Time to "hold down button on Remote)
const int PWMDecrementFactor = 11; //used to provide non-linear, accelerated reduction in PWM Output 
int flashCount = 0; // Used in flash function to count down 
boolean meterLEDOn = false; //Flag: 0 if Grid Meter LED off, 1 if on
boolean newOnCycle = true; // Flag to indicate when the EXPORT LED first illuminates
const int meterLEDOnTime = 500; //If LED has been on for this time (ms), we consider it constantly on
int PWMOut = minPWMOut; //PWM output to Kemo; set to min to be sure power is not immediately applied
int latestMaxPWM = minPWMOn; // Initialise maxPWM out variable
//
boolean socketsOn = false; //variable to store the state of the remote sockets - set to false initially
unsigned long minSocketOnTime = 120000; // Turning sockets off is not allowed until they've been on for this long (mS)
unsigned long socketLastOnTime; // Used to store the millis() time when sockets turned on
unsigned long minSocketOffTime = 120000; // Turning sockets on is not allowed until they've been off for this long (mS)
unsigned long socketLastOffTime; // Used to store the millis() time when sockets turned off
//
//  Set up for calculating average of recent PWM maximum values
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
Serial.println("Solar_Immersion 14/02/2016");
}  
void loop()
{
 // Set meterLEDOn Flag (Outside 'stepTime' Loop)
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
// Immediately turn off Immersion if power being exported
//
  if (meterLEDOn)
  {
  // If LED on, do nothing for now, pending timed 'stepTime' loop  
  }
  else
  {    
    PWMOut = minPWMOut;
    newOnCycle = true;
    turnSocketsOff();
    analogWrite(PWMOutPin, PWMOut); //write PWM signal to mains controller
  }
 //
 // Start of Loop that Only Runs Every 'stepTime' mS 
  if((millis()-lastLoopStartTime) > stepTime)
  {
    lastLoopStartTime = millis(); // Reset stored main loop start time
// Toggle Onboard LED each loop
//
    onboardLED = !onboardLED;
    digitalWrite(13,onboardLED);
// 
// Print status to the serial monitor
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
  if (latestMaxPWM > redLevel)
  {
    Serial.print(" ");
    Serial.print("R");
  }
  else
  {
    Serial.print("  ");    
  }
  if (latestMaxPWM > amberLevel)
  {
    Serial.print("A");
  }
  if (latestMaxPWM > greenLevel)
  {
    Serial.print("G");
  }
    Serial.println(" ");
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
              tone(piezoPin, 1400, 30); // Sound piezo at start of each on cycle
              PWMOut = latestMaxPWM - PWMDecrement;
              if (PWMOut < minPWMOn)
              {
                PWMOut = minPWMOn;
              }
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
          if (latestMaxPWM < minPWMOn)
            {
              latestMaxPWM = minPWMOn;
            }
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
  //Set RAG LED 'Traffic Lights' and Turn on Remote Sockets, if appropriate
  //
    if(latestMaxPWM > redLevel)
    {
      digitalWrite(redLEDPin, HIGH);
    }
    else
    {
      digitalWrite(redLEDPin, LOW);
    }
    if(latestMaxPWM > amberLevel) // At PWM level greater than amberLevel turn on amber LED and fire Sockets
    {
      digitalWrite(amberLEDPin, HIGH);
      turnSocketsOn();
    }
    else
    {
      digitalWrite(amberLEDPin, LOW);
    }
    if(latestMaxPWM > greenLevel) 
    {
      digitalWrite(greenLEDPin, HIGH);
    }
    else
    {
      digitalWrite(greenLEDPin, LOW);
    }
// END of RAG LED Update   
//
} // End of main 'stepTime' Loop
  else
  {
    // Come here to do nothing, if stepTime has not yet elapsed    
  }
}  // END of MAIN LOOP
//
//FUNCTIONS
//
void turnSocketsOn() //Checks if already ON and if not sends pulse to remote transmitter
{
  if (socketsOn) 
  {
//    Already ON so take no action 
  }
  else
  {
    if ((millis() - socketLastOffTime) > minSocketOffTime)
    {
      tone(piezoPin, 500, 300);
      delay(300);
      tone(piezoPin, 600, 300);
      delay(300);
      tone(piezoPin, 1400, 300);
      delay(300);
      digitalWrite(remoteOnPin, HIGH);
      delay(remotePushTime);
      digitalWrite(remoteOnPin, LOW);
      socketsOn = true;
      socketLastOnTime = millis(); //Store time of last switch-on
    }
  }
  socketLastOnTime = millis(); // update the time that sockets were last demanded to be ON
}
//
void turnSocketsOff() //Checks aleady ON and if not sends pulse to remote
{
  if (socketsOn) 
  {
    if ((millis() - socketLastOnTime) > minSocketOnTime)
    {
      tone(piezoPin, 1400, 300);
      delay(300);
      tone(piezoPin, 600, 300);
      delay(300);
      tone(piezoPin, 500, 300);
      delay(300);
      digitalWrite(remoteOffPin, HIGH);
      delay(remotePushTime);
      digitalWrite(remoteOffPin, LOW);
      socketsOn = false;
    }
  }
  else
  {
    //  Already OFF so take no action
  }
  socketLastOffTime = millis(); // update the time that sockets were last demanded to be off
}


