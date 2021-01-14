// Required header files
#include <DS3231.h>
#include <Wire.h>

// Defines from headers
DS3231 Clock;
RTClib myRTC;

// Values of date information
byte YEAR, MONTH, DATE, DOW, HOUR, MINUTE, SECOND;

// Define pins for each 74hc595
// 1st 74hc595 variables
const int U1_CLR = 6;   // aka the clear pin
const int U1_CLK = 7;   // aka the clock pin
const int U1_RCLK = 8;  // aka the latch pin
const int U1_SER = 9;   // aka the data pin
// 2nd 74hc595 variables
const int U2_CLR = 2;   // aka the clear pin
const int U2_CLK = 3;   // aka the clock pin
const int U2_RCLK = 4;  // aka the latch pin
const int U2_SER = 5;   // aka the data pin

// Define 7-segment pins
const int QA = A0;
const int QB = A1;
const int QC = A2;
const int QD = 13;
const int QE = 12;
const int QF = 10;
const int QG = 11;
const int QDP = A3;

// Bit strings for 7 segment displays
//
//    a
//   ---
// f|   |b
//   -g-
// e|   |c
//   ---  .dp
//    d               A  B  C  D  E  F  G  DP
const byte VAL0[8] = {1, 1, 1, 1, 1, 1, 0, 0};
const byte VAL1[8] = {0, 1, 1, 0, 0, 0, 0, 0};
const byte VAL2[8] = {1, 1, 0, 1, 1, 0, 1, 0};
const byte VAL3[8] = {1, 1, 1, 1, 0, 0, 1, 0};
const byte VAL4[8] = {0, 1, 1, 0, 0, 1, 1, 0};
const byte VAL5[8] = {1, 0, 1, 1, 0, 1, 1, 0};
const byte VAL6[8] = {1, 0, 1, 1, 1, 1, 1, 0};
const byte VAL7[8] = {1, 1, 1, 0, 0, 0, 0, 0};
const byte VAL8[8] = {1, 1, 1, 1, 1, 1, 1, 0};
const byte VAL9[8] = {1, 1, 1, 1, 0, 1, 1, 0};

// Define bit codes to activate each of the appropriate lines on the 74hc595.
// These are the int forms of byte codes e.g., 16 == 00010000
// This does not tell which of the 74hc595 to use
const byte SEVEN_SEG_1 = 16; // Qe
const byte SEVEN_SEG_2 = 8; // Qd
const byte SEVEN_SEG_3 = 4; // Qc
const byte SEVEN_SEG_4 = 2; // Qb
const byte SEVEN_SEG_5 = 4; // Qc
const byte SEVEN_SEG_6 = 2; // Qb
const byte SEVEN_SEG_7 = 64; // Qg
const byte SEVEN_SEG_8 = 32; // Qf
const byte SEVEN_SEG_9 = 64; // Qg
const byte SEVEN_SEG_10 = 32; // Qf

// Array of the byte codes for each individual seven segment display
const byte DIGIT[10] = {SEVEN_SEG_1, SEVEN_SEG_2, SEVEN_SEG_3, SEVEN_SEG_4, SEVEN_SEG_5, \
  SEVEN_SEG_6, SEVEN_SEG_7, SEVEN_SEG_8, SEVEN_SEG_9, SEVEN_SEG_10};

// Array of the segments in the order we address them
const byte segmentPins[] = {QA, QB, QC, QD, QE, QF, QG, QDP};

// Array arrays that tell which segments to light up when a given value is displayed
const byte *OUT_VAL[10] = {VAL0, VAL1, VAL2, VAL3, VAL4, VAL5, VAL6, VAL7, VAL8, VAL9};

// Time stamp holder
int TIMESTAMP[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


// Function prototypes
void FetchTimeStamp();
void NowTime();
void GetDateStuff(byte& YEAR, byte& MONTH, byte& DATE, byte& DOW, byte& HOUR, byte& MINUTE, byte& SECOND);
void digit_proc(int device, byte value);
void shift_out(int local_data, int local_clock, byte local_out);

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(segmentPins[i], OUTPUT);
    digitalWrite(segmentPins[i], HIGH);
  }

  pinMode(U1_CLR, OUTPUT);
  digitalWrite(U1_CLR, HIGH);

  pinMode(U2_CLR, OUTPUT);
  digitalWrite(U2_CLR, HIGH);

  pinMode(U1_RCLK, OUTPUT);
  pinMode(U1_CLK, OUTPUT);
  pinMode(U1_SER, OUTPUT);
  pinMode(U2_RCLK, OUTPUT);
  pinMode(U2_CLK, OUTPUT);
  pinMode(U2_SER, OUTPUT);

  Wire.begin();
  Serial.begin(9600);
}

void FetchTimeStamp() {
  DateTime now = myRTC.now();
  uint32_t stamp = now.unixtime();

  int i = 9;
  while(stamp) {
    TIMESTAMP[i] = stamp % 10;
    stamp /= 10;
    i--;
  }
}

void NowTime() {
  DateTime now = myRTC.now();

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print("since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
}

void GetDateStuff(byte& YEAR, byte& MONTH, byte& DATE, byte& DOW, byte& HOUR, byte& MINUTE, byte& SECOND) {
  // Call this if you notice something coming in on 
  // the serial port. The stuff coming in should be in 
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean string_done = false;
  char input_char;
  byte temp_a, temp_b;
  char input_arr[20];
  byte j = 0;

  // Wait until the incoming string is finished
  while (!string_done) {
    // If there is something coming from the serial...
    if (Serial.available()) {
      // Read the incoming character
      input_char = Serial.read();
      // an array of characters 20 bytes long, add the latest character to the buffer
      input_arr[j] = input_char;
      j++;
      // This defines our line ending character
      if (input_char == 'x') {
        string_done = true;
      }
    }
  }

  // Print whatever came in
  Serial.println(input_arr);

  // Process each digit as a byte, shifted by 48
  // First two digits are the year
  temp_a = (byte)input_arr[0] - 48;
  temp_b = (byte)input_arr[1] - 48;
  YEAR = temp_a*10 + temp_b;

  // The next two digits are the month
  temp_a = (byte)input_arr[2] - 48;
  temp_b = (byte)input_arr[3] - 48;
  MONTH = temp_a*10 + temp_b;

  // The next two digits are the date
  temp_a = (byte)input_arr[4] - 48;
  temp_b = (byte)input_arr[5] - 48;
  DATE = temp_a*10 + temp_b;

  // The next two digits are the hour
  temp_a = (byte)input_arr[7] - 48;
  temp_b = (byte)input_arr[8] - 48;
  HOUR = temp_a*10 + temp_b;

  // the next two digits are the minute
  temp_a = (byte)input_arr[9] - 48;
  temp_b = (byte)input_arr[10] - 48;

  // And finally, the last two digits are second
  temp_a = (byte)input_arr[11] - 48;
  temp_b = (byte)input_arr[12] - 48;
  SECOND = temp_a*10 + temp_b;
}

void digit_proc(int device, byte value) {
  int local_rclk = 0;
  int local_clk = 0;
  int local_ser = 0;

  // Set the local clock pins for the appropriate 74hc595
  if (device == 1) {
    local_rclk = U1_RCLK;
    local_clk = U1_CLK;
    local_ser = U1_SER;
  } else if (device == 2) {
    local_rclk = U2_RCLK;
    local_clk = U2_CLK;
    local_ser = U2_SER;
  } else {
    return;
  }

  // Write the value to the 74hc595
  digitalWrite(local_rclk, 0);
  shift_out(local_ser, local_clk, value);
  digitalWrite(local_rclk, 1);

  // ...and reset to 0
  digitalWrite(local_rclk, 0);
  shift_out(local_ser, local_clk, 0);
  digitalWrite(local_rclk, 1);
}

void SetTheClock() {
  GetDateStuff(YEAR, MONTH, DATE, DOW, HOUR, MINUTE, SECOND);

  // Always use 24 hour time
  Clock.setClockMode(false);

  // Set each value in the DS2321
  Clock.setYear(YEAR);
  Clock.setMonth(MONTH);
  Clock.setDate(DATE);
  Clock.setHour(HOUR);
  Clock.setMinute(MINUTE);
  Clock.setSecond(SECOND);
}


void loop() {
  // If something is coming in on the serial line, it's
  // a time correction so set the clock accordingly.
  if (Serial.available()) {
    SetTheClock();
    NowTime();
  }

  // Ask the DS3231 for the current time
  FetchTimeStamp();

  // Iterate through the (i) digits of our 10 digit timestamp and (j) the segments of each display
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 8; j++) {
      digitalWrite(segmentPins[j], OUT_VAL[TIMESTAMP[i]][j]);
    }
    // Make sure to use the correct 74hc595 for the 7segs that use it.
    if ((i < 4) || ((i < 8) && (i > 5))) {
      digit_proc(1, DIGIT[i]);
    } else {
      digit_proc(2, DIGIT[i]);
    }
  }

}


void shift_out(int local_data, int local_clock, byte local_out) {
  int i = 0;
  int pin_state;

  // Set the initial values to 0
  digitalWrite(local_data, 0);
  digitalWrite(local_clock, 0);

  /*
   * Note: this counts down for MSB order.
   */
  // Count down from most significant digit (MSB)
  for (int i = 7; i >= 0; i--)  {
    digitalWrite(local_clock, 0);

    // If the bitmask value is True, activate pin
    if ( local_out & (1 << i) ) {
      pin_state = 1;
    } else {
      pin_state = 0;
    }

    // Write on clock upstroke and reset
    digitalWrite(local_data, pin_state);
    digitalWrite(local_clock, 1);
    digitalWrite(local_data, 0);
  }

  // End with clock low
  digitalWrite(local_clock, 0);
}
