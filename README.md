# arduino_remote_io
Simple Arduino program for using Arduino UNO as remote I/O via RS232.

USAGE:

Note: This program will configure pins 2-7 as pull up inputs and pins 8-13 as outputs with initial values of LOW.  This can be modified if you would like a different distribution of inputs and outputs.

Compile and upload this program to your Arduino UNO.  You may test it using a terminanal program such as minicom.  Connect at 115200 baud, 8 data bits, 1 stop bit, no parity. Send the comand "R;".  You should get a response like "R111111;" followed by a carriage return and line feed.  If you short pin 2 to ground, and reissue the command "R;", you should get the response "R111110".  Notice the built-in LED connected to pin 13 is off.  If you issue the command "W51;", you will see the LED illuminate and get the response "W;" follwed by CR LF.  The W command is followed by pairs of digits consiting of the output index (pin - 8) and value (0 or 1) and terminated with a ";".  Issue the command "W50;" and the LED will turn off.

There are more commands available.  See the comment at the top of the source code for a list of commands.  Currently, only digital I/O is supported.  I plan on implementing analog inputs soon.

Once you understand how the commands and responses work, integrate this with your host application by writing and reading the serial port.  You may need a USB to RS232 adapter if you computer does not have a serial port.  If you have a Raspberry Pi, an RS232 port is present.
