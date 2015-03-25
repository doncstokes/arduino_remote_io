/*
 * File:   arduino_remote_io.ino
 * Author: Don Stokes <don@donstokes.com>
 * Purpose:
 *  Remote I/O (serial) for Arduino UNO.
 *  Protocol Command Syntax:
 *   DIGITAL READ:
 *    Command:  "R;"
 *    Response: (example) "R111110;\r\n"
 *   DIGITAL WRITE:
 *    Command:  (example: turn on output 0 thru 5) "W011121314151;"
 *    Response: "W;\r\n"
 *   ANALOG_READ:
 *    Command:  (example: read pin A0) "A0;"
 *    Response: "Api;\r\n" (where p is the pin and i is a decimal integer such that 0 >= i >= 1023)
 *              Reading 1023 is 5.0V on the analog pin.
 *   VERSION:
 *    Command: "V;"
 *    Response: (example) "V1;\r\n"
 *   BEGIN FIREHOSE:
 *    Command: "B;"
 *    Note:    Must be followed by S command to stop output.
 *             Output identical to the R command is continuously sent to the client.
 *             This command is not recommended.  Buffering can cause stale data.
 *             Use 'R' command instead.
 *  STOP FIREHOSE:
 *    Command: "S"
 *    Note:    Only valid after B command
 * Note:
 *   Inputs are pins 2-7.
 *   Ouputs (pins 8-13) are initialized for Pull-Up.
 */
 /* License:
    arduino_remote_io : Simple Arduino program for using Arduino UNO as remote I/O via RS232
    Copyright (C) 2015  Don Stokes

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  
  */

// Serial Baud Rate
#if 1
#define BAUD_RATE 115200
#else
#define BAUD_RATE 1000000 // Failed
#endif

// I/O Point Indices
#define INPUT_COUNT  6
#define OUTPUT_COUNT 6
#define INPUT_FIRST  2
#define OUTPUT_FIRST (INPUT_FIRST + INPUT_COUNT)
#define INPUT_LAST   (INPUT_FIRST  + INPUT_COUNT  - 1)
#define OUTPUT_LAST  (OUTPUT_FIRST + OUTPUT_COUNT - 1)
#define ANALOG_COUNT 6
#define STR_BUF_SZ   16
#define DECIMAL      10
// API Version
#define VERSION '2'

/*
 * Called by Arduino application framework to initialize the application.
 * Initialize I/O here.
 */
void setup() {
  Serial.begin(BAUD_RATE);
  
  int i;
  // Configure inputs
  for (i = INPUT_FIRST; i <= INPUT_LAST; i++) {
    pinMode(i, INPUT_PULLUP);
  }
  // Configure outputs
  for (i = OUTPUT_FIRST; i <= OUTPUT_LAST; i++) {
    pinMode(i, OUTPUT);
    // Initialize output to LOW
    digitalWrite(i, LOW);
  }
}

/*
 * Loop until byte available on serial port and return it.
 * @return  byte received on serial port
 */
char readChar() {
   while (!Serial.available())
     /*Do nothing*/;
   return Serial.read();
}

/*
 * Read bytes from serial port until a valid command character is received.
 * Syntax errors are reported to client via serial port string "E;".
 * Inter command white space is ignored.
 * @return  Valid command char, like 'R' or 'W'.
 */
char readCmd() {
  char cmd = 0;
  boolean cmdRead = false;
  do {
    cmd = readChar();
    switch (cmd) {
      case 'R':
        cmdRead = true;
        break;
      
      case 'W':
        cmdRead = true;
        break;
      
      case 'A':
        cmdRead = true;
        break;
      
      case 'V':
        cmdRead = true;
        break;
      
      case 'B':
        cmdRead = true;
        break;
      
      case ';':
        // Empty command
        break;
        
      case '\r':
      case '\n':
      case '\t':
      case ' ':
        // White Space
        break;
        
      default:
        onError();
    }
  } while (!cmdRead);
  return cmd;
}

/*
 * Handler for Read ('R') command.
 * Each input is sampled and a response is sent to client via serial port.
 * Response example: "R111110\r\n"
 */
void sendInputs() {
  Serial.write('R');
  int i;
  for (i = 7; i >= 2; i--) {
    int val = digitalRead(i);
    Serial.write(val == HIGH ? '1' : '0');
  }
  Serial.write(";\r\n");
}

/*
 * Write error response to client.
 */
void onError() {
  Serial.write("E;\r\n");
}

/*
 * Handler for Write ('W') command.
 * Syntax is 'W' followed by output index0 followed by output value0, ... indexN, valueN, ";" (optional "\r\n").
 * Acknowledgement "W;\r\n" sent to client.
 */
void writeOutputs() {
  char ch = 0;
  do {
    // Read the output index
    ch = readChar();
    if (ch != ';') {
      int index = 0;
      if (ch >= '0' && ch < '0' + (char)OUTPUT_COUNT)
        index = (int)ch - (int)'0';
      else {
        onError();
        break;
      }
      // Read the output value
      ch = readChar();
      if (ch == '0' || ch == '1')
        digitalWrite(OUTPUT_FIRST + index, ch == '1' ? HIGH : LOW);
      else {
        onError();
        break;
      }
    }
  } while (ch != ';');
  Serial.write("W;\r\n");
}

/*
 * Handler for version command.
 */
void sendVersion() {
  Serial.write('V');
  Serial.write(VERSION);
  Serial.write(";\r\n");
}

/*
 * Handler for BeginFirehose command.
 */
void fireHose() {
  Serial.write("B;\r\n");
  boolean done = false;
  while (!done) {
    sendInputs();
    if (Serial.available()) {
      char ch = readChar();
      switch (ch) {
        case 'S':
          // Handle Stop command during BeginFirehose command
          done = true;
          Serial.write("S;\r\n");
          break;
        case ';':
        case ' ':
        case '\r':
        case '\n':
        case '\t':
          break;
        default:
          onError();
          done = true;
      }
    }
  }
}

/*
 * Handler for for Analog Read command
 */
int readAnalog() {
  int ret = -1;
    // Read the output index
  char  ch = readChar();
  if (ch >= '0' && ch < '0' + ANALOG_COUNT) {
    // Convert ASCII char to int
    int pin = (int)ch - '0';
    ret = analogRead(pin);
    char strNum[STR_BUF_SZ] = "0";
    itoa(ret, strNum, DECIMAL);
    // Send command response
    Serial.write('A');
    // Analog pin index
    Serial.write(ch);
    // A2D value
    Serial.write(strNum);
    // Response terminator
    Serial.write(";\r\n");
  } else
    onError();
  return ret;
}

/*
 * Dispatch command to appropriate handler function.
 */
void dispatchCmd(char cmd) {
  switch (cmd) {
    case 'R':
      sendInputs();
      break;
    case 'W':
      writeOutputs();
      break;
    case 'A':
      readAnalog();
      break;
    case 'B':
      fireHose();
      break;
    case 'V':
      sendVersion();
      break;
    default:
      onError();
  }
}

/*
 * Called by Arduino application framework after "setup()" in infinite loop.
 * This is the main loop of the application.
 */
void loop() {
  // Read next command
  char cmd = readCmd();
  // Process command
  dispatchCmd(cmd);
}
