/*
Vive Localization using hardware interrupts on ESP32
~~~Anastasia Dombrowski~~~~

BPV22NF photodiode w/ TLV2462IP op amp (no additional filtering etc)

*/

int vivePin = 32;
int syncCnt = 0;
volatile unsigned long pulseWidth, pulseFall, x, y, distance;  //holds the length of the input pulse.  volatile because it is updated by the ISR
volatile unsigned long pulseRise = 0;  //time the pulse started.  Used in calculation of the pulse length
volatile boolean newPulse = false;  //flag to indicate that a new pulse has been detected
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR pW() //isr for getting pulse width and distance
{
  portENTER_CRITICAL(&mux);
  if (digitalRead(vivePin) == HIGH)  //if the pin is HIGH
  {
    pulseRise = micros();  //save the current time in microseconds
    newPulse = true;
    distance = pulseRise - pulseFall; //get the distance between end of last pulse and start of new pulse
  } else if (newPulse == true)  //otherwise if we are on a new pulse
  {
    pulseFall = micros(); //save time of fall
    pulseWidth = pulseFall - pulseRise;  //calculate the pulse width
    newPulse = false;  //set flag to indicate that we are done with new pulse
  }
  portEXIT_CRITICAL(&mux);
}


void setup() {
  Serial.begin(115200);
  Serial.println("yo yo yo");
  pinMode (vivePin, INPUT);
  //call interrupt function on change in pin value
  attachInterrupt(digitalPinToInterrupt(vivePin), pW, CHANGE); 
}

void loop()
{
  if (pulseWidth != -1) //restricts bc loop is faster than ISR is called
  {
    Serial.print("syncCnt: ");
    Serial.println(syncCnt);
    if (pulseWidth > 60) // threshold for sync pulses, subject to change based on nature's whimsy
    {
      syncCnt++; //increment count for sync pulses
      pulseWidth = -1; //reset 
      if (syncCnt > 3) //make sure synccnt is set up to go into next sweep loop
      {
        syncCnt = 3;
      }
    }
    else
    {
      pulseWidth = -1; 
      if (syncCnt == 3) //if we have counted 3 syncs
      {
        syncCnt = 0; //reset
        x = distance; //use distance calc from ISR to get how far we are on x axis
        Serial.print("x: ");
        Serial.println(x);
      }
      else if (syncCnt == 1) //after sync pulse between x&y
      {
        syncCnt = 0; //reset counter
        y = distance; //get distance along y axis
        Serial.print("y: ");
        Serial.println(y);
      }
    }
  }
}
