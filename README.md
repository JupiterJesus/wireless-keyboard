# wireless-keyboard
Some hastily thrown together code for reading a keyboard matrix from a Teensy++ 2.0 and sending the keystrokes to a 32u4-based mc wirelessly via the RF24 library. Almost has hastily thrown together as the title. WIP.

## Software Requirements
The RF24 library, for use with an nRF24L01+ radio transceiver. There is more than one version. I'm using the original, RF24-master.

The Arduino IDE, obvs.

If using Teensy's, the Teensyduino software and the Teensy loader.

## Hardware Requirements
2 microcontrollers, a transmitter and a receiver. The transmitter should have enough available digital pins to handle your keyboard matrix, plus 5 pins (3 SPI plus 2 more) for the radio, plus an optional PWM pin for each LED color channel you want to control.

2 nRF24L01+ transceivers. They are extremely cheap on Amazon, or anywhere else. You can get an optional voltage regulator breakout board that makes it a bit easier to use.

Everything else that's needed you should already know about if you're making your own keyboard. Diodes, LEDs, etc.

## Information

###Intro
This code is a WIP. I don't know too much about writing keyboard firmware, so this is a trial and error work in progress. I'm using a Teensy++ to power a gutted 87-key Razer TE Chroma with a 6x17 matrix, the transceiver and 3 PWM pins for the RGB LEDs. Each channel is controlled separately through an NPN transistor. All of the common anode RGB LEDs are wired in parallel, and connected to resistors to limit their current to a max of around 5 mA. 

Besides supporting basic key functionality, the firmware (will) support custom Function key shortcuts, including media keys and LED control. Some basic layer code is implemented, but I doubt I'll do much more with that. The initial version isn't too concerned with providing a general platform for others to customize - it's mostly for me, and if anyone sees this and uses it, that's cool too. Neither do I care about supporting every feature or piece of hardware under the sun - there are other firmwares for that. This is specifically designed from the ground up as a simple, wireless mechanical keyboard.

### Matrix

I set up a 6x17 matrix, because it most closely matched the layout of my keyboard and would be the easiest to wire. A tighter matrix will use fewer pins. Changing the matrix is straightforward - just find the layout variable and modify the first entry. Each key code is a uint8_t, which is eventually written by the Teensy keyboard library as a USB HID code.

Changing which pins each row and column use is as simple as changing the pin definitions - they look like const int PIN_ROW_1 = ?. PIN_ROW_1 is the first (or 0th) row, and PIN_COL_1 is the first column.

### The Key Buffer & Key Definitions

The keyboard library for 32U4 mcs sends up to 6 separate keys and 4 modifier keys. Each key is send as a separate byte, while each modifier is actually a single bit (stored in a byte). To send the current state of the keyboard, you set each of the 6 keys using the library definitions for that key - Keyboard.set_key(KEY_A), for example. To send modifiers, you OR all of the modifiers together and send them all at once - Keyboard.set_modifier(MODIFIERKEY_CTRL | MODIFIERKEY_SHIFT), for example.

I've taken the approach of ORing all of the modifiers together on the transmitter side. The transmitter sends the 6 key bytes, plus a single modifier byte. All seven bytes are directly set by the receiver, as-is, without artistic license.

One note about the key and modifier definitions, and the set_key methods. set_key and set_modifier expect uint8_t, and yet the key definitions are NOT uint8_t. This is because they are created via #define (not a constant declaration) and have the number 0x4000 inexplicably ORed into them. The modifier keys actually have 0x8000 ORed with them. This makes the actual numbers much larger than you'd expect. The compiler implicitly casts these large numbers to unit8_t, cutting off the extraneous 0x4000 and 0x8000 in the process. I don't know what the extra bit is for, but it gave me headaches until I saw the key definitions.

The keyboard library doesn't have definitions for modifier keys, at least not in the same way as regular keys. The modifiers are just bits rather than unique IDs. I had to create my own definitions to handle them, so you'll see those definitions in the code, like this - #define KEY_LCTRL 177. Once these are detected during the strobing period, they are converted to modifiers by ORing, just as usual.

This wouldn't have been a problem exception that the modifier keys aren't unique. MODIFIERKEY_ALT&0xFF is 4. KEY_A&0xFF is 4. Oops.

No, the definition for KEY_LEFT_CTRL isn't what I want. It is just an alias for MODIFIERKEY_CTRL.

For the key and modifier definitions, see <Arduino Library Install>\hardware\teensy\avr\cores\teensy3\keylayouts.h. That file wasn't easy to find. Teensy doesn't give much actual documentation of the internals, just usage. See https://www.pjrc.com/teensy/td_keyboard.html.

## Bugs and Errata
###Note about LEDs
You can only send so much current through a PWM pin. You'll need to amplify/switch the LED current through a transistor, according to the advice here: http://electronics.stackexchange.com/questions/64608/hooking-up-multiple-rgb-leds-while-using-a-minimal-number-of-pwm-pins-on-an-ardu

###nRF24L01+
For more info about the nRF24L01+, most importantly about using capacitors to solve connection issues: https://arduino-info.wikispaces.com/Nrf24L01-2.4GHz-HowTo

###RF24 library shenanigans
There's a potential bug in the RF24-master library, I can't explain it otherwise. If I send exactly 7 bytes of data via radio using the radio::write method, the receiver gets garbage. It is consistent garbage, the same every time.

If I send a byte (uint8_t data = 0; radio.write(&data, sizeof(uint8_t)), the receiver gets 0.

If I send an array of one byte (uint8_t data[] = {0}; radio.write(data, sizeof(uint8_t)), the receiver gets 0.

If I send an array of two bytes (uint8_t data[] = {0,0}; radio.write(data, 2*sizeof(uint8_t)), the receiver gets two 0s.

If I send 3, 4, 5 or 6 bytes, they get the right numbers of 0s.

If I send 7 bytes, the receiver gets 7 non-zero characters. I didn't write them down.

If I send 8 bytes, the receiver gets 8 0s.

I ended up working around the problem by sending an array of 8 bytes and ignoring the 8th byte.

It also seems to break if I don't have the receiver respond, and have the transmitter receive the response. I'm bewildered, but until I figure out the cause of the problem (if ever), I'm leaving in the call and response code. 
