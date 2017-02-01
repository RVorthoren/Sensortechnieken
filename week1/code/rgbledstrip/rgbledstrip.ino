/*
 * TI-1.4 - Week 2, Arduino LED Strip
 *
 * Demonstratie, lees commando om PWM waarden om de ledstrip aan te sturen. Commando
 * komt binnen via seriele poort, karakter voor karakter
 * 
 * Keywords: Events, Seriele communicatie, Libraries
 */

// Global var's
char cmd[200];  
unsigned int index;

// PWM output's voor RGB channels
int ledR = 6;
int ledG = 5;
int ledB = 3;

// Initial pwm value
int pwmValue = 0;

/*
 * Initialisatie
 */
void setup()
{
  // Init seriele poort.
  Serial.begin(9600);
  index = 0;
  
  // Alle leds uit
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  
  // Boodschap naar user
  Serial.println("hello you out there\n");
}

/*
 * Run loop
 */
void loop()
{
    Serial.println("hello kitty!");

    delay(500);
}

/*
 * Events, dit zijn de stubs rond de ISR's van de AVR
 */
void serialEvent()
{
  while( Serial.available() )
  {
    char ch = Serial.read(); 
    
    if( ch == '\n' || ch == '\r' )
    {
      // Add '\0' terminator
      cmd[index] = '\0';
      dispatch(cmd);
      index = 0;  
    }
    else
    {
      cmd[index] = ch;
      index++;
    }
  }
}

/*
 * Dispatch cmd's
 */
void dispatch(char *cmd)
{
  Serial.print("\tDebug:[cmd == "); Serial.print(cmd); Serial.print("]\n");
  char *strValue;
  
  // Handle reset
  if( strcmp(cmd, "reset")  == 0 ||
      strcmp(cmd, "rst")  == 0  )
  {
      Serial.println("Reset OK");
  } 

  // Handle get
  if( strcmp(cmd, "get")  == 0 ||
      strcmp(cmd, "g")  == 0  )
  {
      Serial.print("pwm:");
      Serial.println(pwmValue);
  }  
    
  // Handle set
  if( strncmp(cmd, "set:", strlen("set:"))  == 0 ||
      strncmp(cmd, "s:", strlen("s:"))  == 0  )
  {
      // Split strings op ':'
      strValue = strtok(cmd,":");  // strValue = "set" of "s"
      strValue = strtok(NULL,":"); // strValue = numerical value;
      
      pwmValue = atoi(strValue);
      if( pwmValue <= 0xFF )
      {
        analogWrite(ledR, pwmValue);
        analogWrite(ledG, pwmValue);
        analogWrite(ledB, pwmValue);
      }
  } 
}



