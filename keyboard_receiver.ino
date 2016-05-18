
/*
* Getting Started example sketch for nRF24L01+ radios
* This is a very basic example of how to send data from one node to another
* Updated: Dec 2014 by TMRh20
*/

#include <SPI.h>
#include "RF24.h"

const int PIN_RADIO_CE = 9;
const int PIN_RADIO_CSN = 10;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 9 & 10 */
RF24 radio(PIN_RADIO_CE, PIN_RADIO_CSN);

/**********************************************************/

byte addresses[][6] = {"Recvr","Trans"};
const int KEYBUFFER_SIZE = 7;
uint8_t keys[KEYBUFFER_SIZE];
  

void setup() 
{
  Serial.begin(115200);
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1,addresses[0]);
  
  // Start the radio listening for data
  radio.startListening();
}

void loop() 
{
    uint8_t response[8];
    
    if( radio.available()){
                                                                    // Variable for the received timestamp
      while (radio.available()) {                                   // While there is data ready
        radio.read( response, 8*sizeof(uint8_t) );             // Get the payload
      }
     
      radio.stopListening();                                        // First, stop listening so we can talk   
      radio.write( response, 8*sizeof(uint8_t) );              // Send the final one back.      
      radio.startListening();                                       // Now, resume listening so we catch the next packets.     
      Serial.print(F("Sent response "));
      Serial.print(response[0]); Serial.print(" ");
      Serial.print(response[1]); Serial.print(" ");
      Serial.print(response[2]); Serial.print(" ");
      Serial.print(response[3]); Serial.print(" ");
      Serial.print(response[4]); Serial.print(" ");
      Serial.print(response[5]); Serial.print(" ");
      Serial.print(response[6]); Serial.print(" ");
      Serial.println(response[7]); 
    }
 /* 
  // While there is data ready
    for (int i = 0; i < KEYBUFFER_SIZE; i++)
    {
        while (!radio.available());
        uint8_t datum;
        radio.read( &datum, sizeof(datum) ); // Get the payload
        Serial.print(F("Received  ")); Serial.println(datum);
        keys[i] = datum;
    }
    delay(5000);
  */
  //    Keyboard.set_modifier(receiveKeys[0]);
  //    Keyboard.set_key1(receiveKeys[1]);
  //    Keyboard.set_key2(receiveKeys[2]);
  //    Keyboard.set_key3(receiveKeys[3]);
  //    Keyboard.set_key4(receiveKeys[4]);
  //    Keyboard.set_key5(receiveKeys[5]);
  //    Keyboard.set_key6(receiveKeys[6]);
  //
  //    Keyboard.send_now(); 

} // Loop

