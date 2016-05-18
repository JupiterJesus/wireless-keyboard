
/*
* Getting Started example sketch for nRF24L01+ radios
* This is a very basic example of how to send data from one node to another
* Updated: Dec 2014 by TMRh20
*/

#include <SPI.h>
#include "RF24.h"

/**********************************************************/

/* Keyboard notes
 *  
 *  87 keys
 *  6 x 17 matrix = 102 - 15 cells of the matrix will be blank
 *  6 rows + 17 columns
 *  = 23 switch pins
 *  6 RF pins
 *  3 PWM led pins
 *  total = 32 pins
 *  The teensy 3.2 has over 30 pins, but many are via pads on the bottom - no thanks
 *  That means the Teensy++ 2.0, which has over 40 pins
 *  
 *  I'll either write my own software that manages everything on the ++,
 *  then sends the final keypress to the receiver over RF,
 *  or somehow modify one of the custom keyboard firmwares to 
 *  send the keypress over RF instead of printing it.
 *  
 *  The Receiver could be a Teensy 3.2, or I could go cheaper with an
 *  Arduino pro micro or leonardo, since all I need is usb and RF
 *  support. Literally all it does is have to print the key - simple.
 *  
 *  I'll use the 3.2 for now. If I get a cheaper alternative later I can 
 *  just load the code onto it.
 */
byte addresses[][6] = {"Recvr","Trans"};

const int repeatRate = 200; // 5/sec

// One PWM pin for each LED channel
const int PIN_LED_RED = 24;
const int PIN_LED_GREEN = 25;
const int PIN_LED_BLUE = 26;

const int PIN_RADIO_IRQ = 20;
const int PIN_RADIO_MISO = 23;
const int PIN_RADIO_MOSI = 22;
const int PIN_RADIO_SCLK = 21;
const int PIN_RADIO_CE = 18;
const int PIN_RADIO_CSN = 19;
const int PIN_RADIO_SS = 20;

// 6 rows in the keyboard matrix
const byte MATRIX_ROWS = 6;

// 17 columns in the keyboard matrix
const byte MATRIX_COLS = 17;

// 2 key layers - not really using this atm
int LAYERS = 2;

bool toggleBind = false;
int currLayer = 0;
int prevLayer = 0;
int desiredLayer = 1;

const int KEYBUFFER_SIZE = 7;
const int PIN_ROW_1 = 38;
const int PIN_ROW_2 = 39;
const int PIN_ROW_3 = 40;
const int PIN_ROW_4 = 41;
const int PIN_ROW_5 = 42;
const int PIN_ROW_6 = 43;

const int PIN_COL_1 = 0;
const int PIN_COL_2 = 1;
const int PIN_COL_3 = 2;
const int PIN_COL_4 = 3;
const int PIN_COL_5 = 4;
const int PIN_COL_6 = 5;
const int PIN_COL_7 = 7;
const int PIN_COL_8 = 8;
const int PIN_COL_9 = 9;
const int PIN_COL_10 = 10;
const int PIN_COL_11 = 11;
const int PIN_COL_12 = 12;
const int PIN_COL_13 = 13;
const int PIN_COL_14 = 14;
const int PIN_COL_15 = 15;
const int PIN_COL_16 = 16;
const int PIN_COL_17 = 17;

byte row[MATRIX_ROWS] = {PIN_ROW_1,PIN_ROW_2,PIN_ROW_3,PIN_ROW_4,PIN_ROW_5,PIN_ROW_6};
byte col[MATRIX_COLS] = {PIN_COL_1,PIN_COL_2,PIN_COL_3,PIN_COL_4,PIN_COL_5,PIN_COL_6,
                  PIN_COL_7,PIN_COL_8,PIN_COL_9,PIN_COL_10,PIN_COL_11,PIN_COL_12,
                  PIN_COL_13,PIN_COL_14,PIN_COL_15,PIN_COL_16,PIN_COL_17};

uint8_t keyBuffer[] = {0,0,0,0,0,0,0};
const int KEY_BUFFER_INDEX = 1;
const int MOD_BUFFER_INDEX = 0;

// Constant for no key. Since no keys are connected to this pair of pins, it should not show up
#define KEY_NONE 0

// Constant for the Function key. It isn't a USB HID key, it is a special code just for firmware functions
#define KEY_FN 0xFF
#define KEY_LCTRL 176
#define KEY_RCTRL 177
#define KEY_LALT 178
#define KEY_RALT 179
#define KEY_LSHIFT 180
#define KEY_RSHIFT 181
#define KEY_LGUI 182
#define KEY_RGUI 183

/*
 *  NOTE: the key codes are all ORed with 0x4000 for some reason,
 *  but the keyboard functions work on uint8_t, which implicitly truncates it
 *  Also, there are no key codes dedicated to modifiers - they solely function
 *  as modifiers that can be ORed together, and are ORed themselves with 0x8000,
 *  so the numbers are annoyingly large AND not guaranteed to be unique when truncated
 *  For example, KEY_LEFT_ALT & 0xFF is 4... which is the same as A
 *  So, I made up my own keycodes for the modifiers that are only used
 *  here when checking for pressed keys. When they are detected, I convert them
 *  to a bunch of ORed modifiers. They are truncated (no 0x8000), but that doesn't matter
 *  because the set_modifier function truncates them anyway. No idea why the 0x4000
 *  and 0x8000 - it might have something to do with the sign bit that is too
 *  technical for me to grok.
 */
unsigned int layout[][MATRIX_ROWS][MATRIX_COLS] =
{
  {
    {KEY_ESC, KEY_NONE, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINTSCREEN, KEY_SCROLL_LOCK, KEY_PAUSE},
    {KEY_TILDE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUAL, KEY_BACKSPACE, KEY_INSERT, KEY_HOME, KEY_PAGE_UP},
    {KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_L, KEY_O, KEY_P, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEY_BACKSLASH, KEY_DELETE, KEY_END, KEY_PAGE_DOWN},
    {KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, KEY_NONE, KEY_ENTER, KEY_NONE, KEY_NONE, KEY_NONE},
    {KEY_LSHIFT, KEY_NONE, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_NONE, KEY_RSHIFT, KEY_NONE, KEY_UP_ARROW, KEY_NONE},
    {KEY_LCTRL, KEY_LGUI, KEY_LALT, KEY_NONE, KEY_NONE, KEY_NONE, KEY_SPACE, KEY_NONE, KEY_NONE, KEY_NONE, KEY_RALT, KEY_FN, KEY_MENU, KEY_RCTRL, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_RIGHT_ARROW} 
  }
};

/* Radio configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(PIN_RADIO_CE,PIN_RADIO_CSN);

void setup() 
{
  Serial.begin(115200);
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[0]);
  radio.openReadingPipe(1,addresses[1]);
  
  // initialize the led pins as an output.
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);

  // init all of the columns as outputs
  for (int c = 0; c < MATRIX_COLS; c++)
  {
    pinMode(col[c], OUTPUT);
  }

  // init all of the rows as inputs
  for (int r = 0; r < MATRIX_ROWS; r++)
  {
    pinMode(row[r], INPUT);
    // some of the pins I'm using are analog
    // They appear to start high briefly and go down to low after a few cycles
    // This ensures they start low and there are no false key presses at startup
    analogWrite(row[r], 0); 
  }
/* TEST print of the matrix key codes
  for ( int i = 0; i < MATRIX_ROWS; i++)
  {
    for (int j = 0; j < MATRIX_COLS; j++)
    {
      Serial.print(layout[0][i][j]&0xFF);
      Serial.print(" ");
    }
    Serial.println();
  }
  //delay(50000);
  */
}

void loop() 
{
 
  for (int c = 0; c < MATRIX_COLS; c++) 
  {
    digitalWrite(col[c], HIGH);
    for (int r = 0; r < MATRIX_ROWS; r++)
    {
      if (digitalRead(row[r]))
      {
          // Triggers macro function when '#' is pressed, can be any other char defined in layout[][][]
          if(layout[currLayer][r][c] == '#')
          {
            ctrlAltDel(); // Performs macro function
          }
          else
          {
            uint8_t keypress = layout[currLayer][r][c];
            delay(1000);
            Serial.print("Column ");Serial.print(c);Serial.print(" on pin ");Serial.print(col[c]);Serial.println(" high");
            Serial.print("Row ");Serial.print(r);Serial.print(" on pin ");Serial.print(row[r]);Serial.println(" high");
            Serial.print("Key pressed: ");Serial.print(keypress);Serial.print(" = "); Serial.println(getKeyName(keypress));
            setKey(keypress);
          }
       }
    }
    digitalWrite(col[c], LOW);
  }
  
  holdLayer('^', desiredLayer); // If the fn layer key is held, it changes the layer to the desired layer, when released, returns to previous layer

  // If the FN key is being held down, handle it in the firwmare
  // o/w just send it
  if (!isKeyHeld(KEY_FN) && !handleFunctionKey())
    sendKey();
  delay(5);

} // Loop

// This function will take keypresses passed to it 
// and add them to set of six keys that will be passed to the computer when Keyboard.send_now() is called.

// Basically, this collects the currently pressed keys and stores them until they can be passed to the computer.
void setKey(uint8_t keypress)
{ 
  // Catch Modifiers
  if(keypress == KEY_LCTRL || keypress == KEY_RCTRL)
  {
    setMod(MODIFIERKEY_CTRL);
  }
  else if(keypress == KEY_LALT || keypress == KEY_RALT)
  {
    setMod(MODIFIERKEY_ALT);
  }
  else if(keypress == KEY_LSHIFT || keypress == KEY_RSHIFT)
  {
    setMod(MODIFIERKEY_SHIFT);
  }
  else if(keypress == KEY_LGUI || keypress == KEY_RGUI)
  {
    setMod(MODIFIERKEY_GUI);
  }
  else
  {
    
    // Look for unused keys in the buffer - remember, first key is for mods
    int i;
    for(i = KEY_BUFFER_INDEX; keyBuffer[i] != 0 && i < KEYBUFFER_SIZE; i++)
      ;

    if (i < KEYBUFFER_SIZE)
      keyBuffer[i] = keypress;
  }
  
  if(isKeyHeld('^')) // Prevent setting layer key into set_key or set_modifier
    return;
}

void setMod(uint8_t keypress)
{
  keyBuffer[MOD_BUFFER_INDEX] |= (keypress&0xFF);
}

// Checks for FN key macros and activates them
// Used when the FN key is held down, instead of sending to the PC
bool handleFunctionKey()
{
  if (isKeyHeld(KEY_R))
  {
    if (isModifierHeld(MODIFIERKEY_ALT))
      handleFnAltRed();
    else
      handleFnRed();
    return true; 
  }
  else if (isKeyHeld(KEY_G))
  {
    if (isModifierHeld(MODIFIERKEY_ALT))
      handleFnAltGreen();
    else
      handleFnGreen();
    return true;
  } 
  else if (isKeyHeld(KEY_B))
  {
    if (isModifierHeld(MODIFIERKEY_ALT))
      handleFnAltBlue();
    else
      handleFnBlue();
    return true;
  }
  return false;
}

void handleFnAltRed()
{
  
}
void handleFnRed()
{
  
}
void handleFnAltGreen()
{
  
}
void handleFnGreen()
{
  
}
void handleFnAltBlue()
{
  
}
void handleFnBlue()
{
  
}

// This method sends the depressed keys and clears the buffer.
void sendKey()
{
  // First, stop listening so we can talk.
  radio.stopListening();
  uint8_t transmit[8];
  for (int i = 0; i < KEYBUFFER_SIZE; i++) transmit[i]=keyBuffer[i];
  
  if (!radio.write( transmit, 8*sizeof(uint8_t ) ))
    Serial.println(F("failed"));
  radio.startListening();
  
  unsigned long started_waiting_at = micros();
  boolean timeout = false;
  
  // While nothing is received
  while ( ! radio.available() )
  {  
    // If waited longer than 200ms, indicate timeout and exit while loop
    if (micros() - started_waiting_at > 200000 )
    {   timeout = true;
        break;
    }      
  }
      
  if ( timeout )
  {
    Serial.println(F("Failed, response timed out."));
  }
  else
  {
    // Grab the response, compare, and send to debugging spew
    uint8_t response[8];
    radio.read( response, 8*sizeof(uint8_t) );

    // Spew it
    Serial.print(F("Sent "));
    Serial.print(transmit[0]);Serial.print(" ");
    Serial.print(transmit[1]);Serial.print(" ");
    Serial.print(transmit[2]);Serial.print(" ");
    Serial.print(transmit[3]);Serial.print(" ");
    Serial.print(transmit[4]);Serial.print(" ");
    Serial.print(transmit[5]);Serial.print(" ");
    Serial.print(transmit[6]);Serial.print(" ");
    Serial.print(F(", Got response "));
    Serial.print(response[0]); Serial.print(" "); 
    Serial.print(response[1]); Serial.print(" "); 
    Serial.print(response[2]); Serial.print(" "); 
    Serial.print(response[3]); Serial.print(" "); 
    Serial.print(response[4]); Serial.print(" "); 
    Serial.print(response[5]); Serial.print(" "); 
    Serial.println(response[6]);
  }

  // Try again 1s later
  //    delay(1000);

  clearBuffer();
}

// Helper function to clear the buffer
void clearBuffer()
{
  for(int i = 0; i < KEYBUFFER_SIZE; i++){ keyBuffer[i] = 0; }
}

// Detects when a key is held down, returns true if held down, false if not
bool isKeyHeld(uint8_t keypress)
{
  for (int i = KEY_BUFFER_INDEX; i < KEYBUFFER_SIZE; i++)
    if (keyBuffer[i] == keypress)
      return true;

  return false;
}

bool isModifierHeld(uint8_t modifier)
{
  return keyBuffer[MOD_BUFFER_INDEX] & (modifier&0xFF);
}

// Calling this function will cycle to the next layer
void cycleLayer()
{
  if(currLayer == (LAYERS - 1)) // Reached maximum layer, going back to first layer
    currLayer = 0;
  else
    currLayer++; // Increments to the next layer
}

// Toggles between two layers, the curret layer and desired layer
void toggleLayer(uint8_t keyHeld, int desLayer)
{ 
  if (isKeyHeld(keyHeld))
  {
    prevLayer = currLayer; // Saves previous layer
    currLayer = desLayer; // Desired layer
  }
  else
  {
    currLayer = prevLayer; // Returns to previous layer
  }
}

// Macro sequence
void ctrlAltDel()
{ 
  // Using CTRL+ALT+KEYPAD_0 as example
  setKey(KEYPAD_0);
  setMod(MODIFIERKEY_CTRL);
  setMod(MODIFIERKEY_ALT);
  
  sendKey();
}

// Goes to desired layer when keyHeld is pressed, returns to previous layer when released 
void holdLayer(uint8_t keyHeld, int desLayer)
{ 
  if(isKeyHeld(keyHeld))
  { 
    if(!toggleBind)
    { // Saves the previous layer, using boolean to prevent updating prevLayer more than once
      prevLayer = currLayer;
      toggleBind = 1;
    }
    
    currLayer = desLayer; // Desire layer
  }
  else
  {
    if(toggleBind)
    { 
      toggleBind = !toggleBind; // Resets boolean
    }
    
    currLayer = prevLayer; // Returns to previous layer
  }
}

const char *keyNames[256];
bool keyNamesSetup = false;
void setupKeyNames()
{
  keyNames[(uint8_t)KEY_ESC] = "escape";
  keyNames[(uint8_t)KEY_F1] = "f1";
  keyNames[(uint8_t)KEY_F2] = "f2";
  keyNames[(uint8_t)KEY_F3] = "f3";
  keyNames[(uint8_t)KEY_F4] = "f4";
  keyNames[(uint8_t)KEY_F5] = "f5";
  keyNames[(uint8_t)KEY_F6] = "f6";
  keyNames[(uint8_t)KEY_F7] = "f7";
  keyNames[(uint8_t)KEY_F8] = "f8";
  keyNames[(uint8_t)KEY_F9] = "f9";
  keyNames[(uint8_t)KEY_F10] = "f10";
  keyNames[(uint8_t)KEY_F11] = "f11";
  keyNames[(uint8_t)KEY_F12] = "f12";
  keyNames[(uint8_t)KEY_PRINTSCREEN] = "print screen";
  keyNames[(uint8_t)KEY_SCROLL_LOCK] = "scroll lock";
  keyNames[(uint8_t)KEY_PAUSE] = "pause";
  keyNames[(uint8_t)KEY_TILDE] = "~";
  keyNames[(uint8_t)KEY_1] = "1";
  keyNames[(uint8_t)KEY_2] = "2";
  keyNames[(uint8_t)KEY_3] = "3";
  keyNames[(uint8_t)KEY_4] = "4";
  keyNames[(uint8_t)KEY_5] = "5";
  keyNames[(uint8_t)KEY_6] = "6";
  keyNames[(uint8_t)KEY_7] = "7";
  keyNames[(uint8_t)KEY_8] = "8";
  keyNames[(uint8_t)KEY_9] = "9";
  keyNames[(uint8_t)KEY_0] = "0";
  keyNames[(uint8_t)KEY_MINUS] = "-";
  keyNames[(uint8_t)KEY_EQUAL] = "=";
  keyNames[(uint8_t)KEY_BACKSPACE] = "bksp";
  keyNames[(uint8_t)KEY_INSERT] = "ins";
  keyNames[(uint8_t)KEY_HOME] = "home";
  keyNames[(uint8_t)KEY_PAGE_UP] = "pgup";
  keyNames[(uint8_t)KEY_TAB] = "tab";
  keyNames[(uint8_t)KEY_Q] = "q";
  keyNames[(uint8_t)KEY_W] = "e";
  keyNames[(uint8_t)KEY_E] = "e";
  keyNames[(uint8_t)KEY_R] = "r";
  keyNames[(uint8_t)KEY_T] = "t";
  keyNames[(uint8_t)KEY_Y] = "y";
  keyNames[(uint8_t)KEY_U] = "u";
  keyNames[(uint8_t)KEY_L] = "l";
  keyNames[(uint8_t)KEY_O] = "o";
  keyNames[(uint8_t)KEY_P] = "p";
  keyNames[(uint8_t)KEY_LEFT_BRACE] = "{";
  keyNames[(uint8_t)KEY_RIGHT_BRACE] = "}";
  keyNames[(uint8_t)KEY_BACKSLASH] = "\\";
  keyNames[(uint8_t)KEY_DELETE] = "del";
  keyNames[(uint8_t)KEY_END] = "end";
  keyNames[(uint8_t)KEY_PAGE_DOWN] = "pgdn";
  keyNames[(uint8_t)KEY_CAPS_LOCK] = "caps";
  keyNames[(uint8_t)KEY_A] = "a";
  keyNames[(uint8_t)KEY_S] = "s";
  keyNames[(uint8_t)KEY_D] = "d";
  keyNames[(uint8_t)KEY_F] = "f";
  keyNames[(uint8_t)KEY_G] = "g";
  keyNames[(uint8_t)KEY_H] = "h";
  keyNames[(uint8_t)KEY_J] = "j";
  keyNames[(uint8_t)KEY_K] = "k";
  keyNames[(uint8_t)KEY_L] = "l";
  keyNames[(uint8_t)KEY_SEMICOLON] = ";";
  keyNames[(uint8_t)KEY_QUOTE] = "'";
  keyNames[(uint8_t)KEY_ENTER] = "enter";
  keyNames[(uint8_t)KEY_LSHIFT] = "left shift";
  keyNames[(uint8_t)KEY_Z] = "z";
  keyNames[(uint8_t)KEY_X] = "x";
  keyNames[(uint8_t)KEY_C] = "c";
  keyNames[(uint8_t)KEY_V] = "v";
  keyNames[(uint8_t)KEY_B] = "b";
  keyNames[(uint8_t)KEY_N] = "n";
  keyNames[(uint8_t)KEY_M] = "m";
  keyNames[(uint8_t)KEY_COMMA] = ",";
  keyNames[(uint8_t)KEY_PERIOD] = ".";
  keyNames[(uint8_t)KEY_SLASH] = "/";
  keyNames[(uint8_t)KEY_RSHIFT] = "right shift";
  keyNames[(uint8_t)KEY_UP_ARROW] = "up arrow";
  keyNames[(uint8_t)KEY_LCTRL] = "left ctrl";
  keyNames[(uint8_t)KEY_LGUI] = "windows/gui";
  keyNames[(uint8_t)KEY_LALT] = "left alt";
  keyNames[(uint8_t)KEY_SPACE] = "space";
  keyNames[(uint8_t)KEY_RIGHT_ALT] = "right alt";
  keyNames[(uint8_t)KEY_FN] = "function";
  keyNames[(uint8_t)KEY_MENU] = "context/menu";
  keyNames[(uint8_t)KEY_RCTRL] = "right ctrl";
  keyNames[(uint8_t)KEY_LEFT_ARROW] = "left arrow";
  keyNames[(uint8_t)KEY_DOWN_ARROW] = "down arrow";
  keyNames[(uint8_t)KEY_RIGHT_ARROW] = "right arrow";
  keyNamesSetup = true;
}

const char *getKeyName(uint8_t keypress)
{
  if (!keyNamesSetup)
    setupKeyNames();
  return keyNames[keypress];
}

