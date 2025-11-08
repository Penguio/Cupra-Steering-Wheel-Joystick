// include files
#include "LIN_master_HardwareSerial.h"

// skip serial output (for time measurements)
#define SKIP_CONSOLE
#define SKIP_CONSOLE_BOUNCE_BUTTON



#include <Bounce.h>
// Start-Stop Button is not available by Lin-Bus
const int StartStopPin = 14;
Bounce pushbutton = Bounce(StartStopPin, 10);  // 10 ms debounce

// setup LIN node
LIN_Master_HardwareSerial lin(Serial2, "LIN_HW"); // parameter: HW-interface, name

// call once
void setup()
{
  // To use the Start-Stop Button we use an digital I/O Pin as Input
  pinMode(StartStopPin, INPUT_PULLUP);
#ifndef SKIP_CONSOLE_BOUNCE_BUTTON
  Serial.begin(115200);
  Serial.println("Pushbutton Bounce library test:");
#endif

  // open LIN interface
  lin.begin(19200);

// for user interaction via console
#ifndef SKIP_CONSOLE
  Serial.begin(115200);
  while (!Serial)
    ; // wait or not wait on Serial Interface to PC established before going further
#endif

} // setup()

byte previousState = HIGH;     // what state was the button last time
unsigned int count = 0;        // how many times has it changed to low
unsigned long countAt = 0;     // when count changed
unsigned int countPrinted = 0; // last count printed

uint8_t ReverseEngineeringStep = 4; // just change by hand and recompile reflash
// To find and reverse engineer the button presses I used Step4
// at the end of step 4 I switched back to step2 to output the serial string of the received frame IDs
// I logged 30-60s of LIN received string without any activation of buttons into an log file
// after this I logged 20-40s of Lin received string to an log file and only in the last 0,5s activated one button at a time
// then I compared the "noButton.log" against the "Button***.log" and decoded which bits changed
// the result you find in Case5
// unfortunally I need also Case4 to switch the steering wheel into: "send button activation" mode
// so I let Case4 run and at the end of Case4 switch to Case5 to decode each individual button
// call repeatedly
void loop()
{

  if (pushbutton.update())
  {
    if (pushbutton.fallingEdge())
    {
      count = count + 1;
      Joystick.button(19, 1); // press button "num" (1 to 32)
      countAt = millis();
    }
  } else
  {
    if (count != countPrinted)
    {
      unsigned long nowMillis = millis();
      if (nowMillis - countAt > 100)
      {
#ifndef SKIP_CONSOLE_BOUNCE_BUTTON
        Serial.print("Start-Stop pressed times: ");
        Serial.println(count);
#endif
        Joystick.button(19, 0); // release button "num" (1 to 32)
        countPrinted = count;
      }
    }
  }

  uint8_t Tx[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  LIN_Master::frame_t Type;
  uint8_t Id;
  uint8_t NumData;
  uint8_t Data[8];
  LIN_Master::error_t error;
  uint8_t linFrameData[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  // reset state machine & error
  lin.resetStateMachine();
  lin.resetError();

  switch (ReverseEngineeringStep)
  {
  case 1U: // Check at which ID, and with which lenght the Steering wheel answers to an Slave request
  {
#ifndef SKIP_CONSOLE
    Serial.println("starting to GuessFrameIds");
#endif

    for (uint8_t GuessFrameId = 0x00; GuessFrameId <= 0x3F; GuessFrameId++) // Lin has a maximum of 6bit IDs
    {
      for (uint8_t GuessNumData = 1; GuessNumData <= 8; GuessNumData++)
      {
        lin.resetStateMachine();
        lin.resetError();
        for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
        {
          linFrameData[i] = 0;
        }
        error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, GuessFrameId, GuessNumData, linFrameData);
        if (error != LIN_Master::NO_ERROR)
        {
#ifndef SKIP_CONSOLE
          Serial.print("error");
#endif
        }
        else
        {
          // only called if an valid LIN Frame is received otherwise we return from the function
          // so every reported Serial.print is an valid LIN-Frame
          Serial.print("SlaveResponseID: ");
          Serial.print(";0x");
          Serial.print(GuessFrameId < 16 ? "0" : "");
          Serial.print(GuessFrameId, HEX);
          Serial.print("; GuessNumData: ;");
          Serial.print(GuessNumData, DEC);
          Serial.println(";");
        }
        // delay(10);
        // if (GuessFrameId == 0xFF && GuessNumData == 8)
        // {
        //   return nullptr; // we never reach the lines below these because of the return statement, so there is never an "End of Guess...." in the log
        // }
      }
    }
#ifndef SKIP_CONSOLE
    Serial.println("End of GuessFrameIds");
#endif
    break;
  }

  case 2U: // After finding the response IDs we only send requests to these IDs and look at the Data changes during button presses
  {
#ifndef SKIP_CONSOLE
    Serial.println("Searching button data");
#endif
    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x0E, 8, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
// only called if an valid LIN Frame is received otherwise we return from the function
#ifndef SKIP_CONSOLE
    Serial.print("Data received @ 0x0E: ");
    Serial.print(";0x");
    Serial.print(linFrameData[0] < 16 ? "0" : "");
    Serial.print(linFrameData[0], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[1] < 16 ? "0" : "");
    Serial.print(linFrameData[1], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[2] < 16 ? "0" : "");
    Serial.print(linFrameData[2], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[3] < 16 ? "0" : "");
    Serial.print(linFrameData[3], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[4] < 16 ? "0" : "");
    Serial.print(linFrameData[4], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[5] < 16 ? "0" : "");
    Serial.print(linFrameData[5], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[6] < 16 ? "0" : "");
    Serial.print(linFrameData[6], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[7] < 16 ? "0" : "");
    Serial.print(linFrameData[7], HEX);
    Serial.println(";");
#endif

    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x0F, 8, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
// only called if an valid LIN Frame is received otherwise we return from the function
#ifndef SKIP_CONSOLE
    Serial.print("Data received @ 0x0F: ");
    Serial.print(";0x");
    Serial.print(linFrameData[0] < 16 ? "0" : "");
    Serial.print(linFrameData[0], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[1] < 16 ? "0" : "");
    Serial.print(linFrameData[1], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[2] < 16 ? "0" : "");
    Serial.print(linFrameData[2], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[3] < 16 ? "0" : "");
    Serial.print(linFrameData[3], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[4] < 16 ? "0" : "");
    Serial.print(linFrameData[4], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[5] < 16 ? "0" : "");
    Serial.print(linFrameData[5], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[6] < 16 ? "0" : "");
    Serial.print(linFrameData[6], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[7] < 16 ? "0" : "");
    Serial.print(linFrameData[7], HEX);
    Serial.println(";");
#endif

    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x11, 8, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
// only called if an valid LIN Frame is received otherwise we return from the function
#ifndef SKIP_CONSOLE
    Serial.print("Data received @ 0x11: ");
    Serial.print(";0x");
    Serial.print(linFrameData[0] < 16 ? "0" : "");
    Serial.print(linFrameData[0], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[1] < 16 ? "0" : "");
    Serial.print(linFrameData[1], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[2] < 16 ? "0" : "");
    Serial.print(linFrameData[2], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[3] < 16 ? "0" : "");
    Serial.print(linFrameData[3], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[4] < 16 ? "0" : "");
    Serial.print(linFrameData[4], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[5] < 16 ? "0" : "");
    Serial.print(linFrameData[5], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[6] < 16 ? "0" : "");
    Serial.print(linFrameData[6], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[7] < 16 ? "0" : "");
    Serial.print(linFrameData[7], HEX);
    Serial.println(";");
#endif

    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x0C, 4, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
// only called if an valid LIN Frame is received otherwise we return from the function
#ifndef SKIP_CONSOLE
    Serial.print("Data received @ 0x0C: ");
    Serial.print(";0x");
    Serial.print(linFrameData[0] < 16 ? "0" : "");
    Serial.print(linFrameData[0], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[1] < 16 ? "0" : "");
    Serial.print(linFrameData[1], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[2] < 16 ? "0" : "");
    Serial.print(linFrameData[2], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[3] < 16 ? "0" : "");
    Serial.print(linFrameData[3], HEX);
    Serial.println(";");
#endif

    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x38, 4, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
// only called if an valid LIN Frame is received otherwise we return from the function
#ifndef SKIP_CONSOLE
    Serial.print("Data received @ 0x38: ");
    Serial.print(";0x");
    Serial.print(linFrameData[0] < 16 ? "0" : "");
    Serial.print(linFrameData[0], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[1] < 16 ? "0" : "");
    Serial.print(linFrameData[1], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[2] < 16 ? "0" : "");
    Serial.print(linFrameData[2], HEX);
    Serial.print(";0x");
    Serial.print(linFrameData[3] < 16 ? "0" : "");
    Serial.print(linFrameData[3], HEX);
    Serial.println(";");

    Serial.println("End of searching button ");
#endif
    break;
  }

  case 3U: // didn't know what I've done
  {
#ifndef SKIP_CONSOLE
    Serial.println("starting to GuessFrameIds");
    for (uint8_t GuessFrameId = 0x00; GuessFrameId <= 0x3F; GuessFrameId++) // Lin has a maximum of 6bit IDs
    {
      for (uint8_t GuessNumData = 1; GuessNumData <= 8; GuessNumData++)
      {
        lin.resetStateMachine();
        lin.resetError();
        for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
        {
          linFrameData[i] = 0;
        }
        // send/receive slave response frame and get result immediately
        error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, GuessFrameId, GuessNumData, Tx);

        if (lin.getError() == LIN_Master::NO_ERROR)
        {
          // only called if an valid LIN Frame is received otherwise we return from the function
          // so every reported Serial.print is an valid LIN-Frame
          Serial.print("SlaveResponseID: ");
          Serial.print(";0x");
          Serial.print(GuessFrameId < 16 ? "0" : "");
          Serial.print(GuessFrameId, HEX);
          Serial.print("; GuessNumData: ;");
          Serial.print(GuessNumData, DEC);
          Serial.println(";");
        }

        // get frame data
        lin.getFrame(Type, Id, NumData, Data);

        for (uint8_t i = 0; (i < NumData) && (lin.getError() == LIN_Master::NO_ERROR); i++)
        {
          Serial.print("\t");
          Serial.print((int)i);
          Serial.print("\t0x");
          Serial.println((int)Data[i], HEX);
        }
        // delay(10);
        if (GuessFrameId == 0xFF && GuessNumData == 8)
        {
          return nullptr; // we never reach the lines below these because of the return statement, so there is never an "End of Guess...." in the log
        }
      }
    }
    Serial.println("End of GuessFrameIds");
#endif
    break;
  }

  case 4U: // Check at which ID, and with which lenght the Steering wheel answers to an Master request We need this to put the steering wheel in the mode to acutally output DATA
  {
#ifndef SKIP_CONSOLE
    Serial.println("starting to GuessFrameIds");
#endif
    // delay(2); //to give the serial console enough time to transfer the starting message
    for (uint8_t GuessFrameId = 0x00; GuessFrameId <= 0x3F; GuessFrameId++) // Lin has a maximum of 6bit IDs
    {
      for (uint8_t GuessNumData = 1; GuessNumData <= 8; GuessNumData++)
      {
        lin.resetStateMachine();
        lin.resetError();
        for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
        {
          linFrameData[i] = 0;
        }
        // send/receive slave response frame and get result immediately
        error = lin.sendMasterRequestBlocking(LIN_Master::LIN_V2, GuessFrameId, GuessNumData, Tx);

        if (lin.getError() == LIN_Master::NO_ERROR)
        {
// only called if an valid LIN Frame is received otherwise we return from the function
// so every reported Serial.print is an valid LIN-Frame
#ifndef SKIP_CONSOLE
          Serial.print("MasterRequestID: ");
          Serial.print(";0x");
          Serial.print(GuessFrameId < 16 ? "0" : "");
          Serial.print(GuessFrameId, HEX);
          Serial.print("; GuessNumData: ;");
          Serial.print(GuessNumData, DEC);
          Serial.println(";");
#endif
        }

        // get frame data
        lin.getFrame(Type, Id, NumData, Data);

        for (uint8_t i = 0; (i < NumData) && (lin.getError() == LIN_Master::NO_ERROR); i++)
        {
#ifndef SKIP_CONSOLE
          Serial.print("\t");
          Serial.print((int)i);
          Serial.print("\t0x");
          Serial.println((int)Data[i], HEX);
#endif
        }
        // delay(10);
        // if (GuessFrameId == 0xFF && GuessNumData == 8)
        // {
        //   return nullptr; // we never reach the lines below these because of the return statement, so there is never an "End of Guess...." in the log
        // }
      }
    }
#ifndef SKIP_CONSOLE
    Serial.println("End of GuessFrameIds");
#endif
    ReverseEngineeringStep = 5; // after setting the steering wheel into output button data mode we go further with decoding
    // ReverseEngineeringStep = 2; //after setting the steering wheel into output button data mode we go further to logging so that it is possible to reverse engineer the individual bits
    break;
  }

  case 5U: // After finding the response IDs we only send requests to these IDs and look at the Data changes during button presses
  {
    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x0E, 8, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }
    if (linFrameData[3])
    {
      switch (linFrameData[1])
      {
      case 0x02:
      {
#ifndef SKIP_CONSOLE
        Serial.println("PageRight");
#endif
        Joystick.button(5, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x03:
      {
#ifndef SKIP_CONSOLE
        Serial.println("PageLeft");
#endif
        Joystick.button(6, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x06:
      {
        if (linFrameData[3] <= 0x07)
        {
#ifndef SKIP_CONSOLE
          Serial.print("Roll Up um: ");
          Serial.print(linFrameData[3], DEC);
          Serial.println(" Ticks");
#endif
          Joystick.button(7, 1); // Press button "num" (1 to 32)
        }
        if (linFrameData[3] > 0x07)
        {
#ifndef SKIP_CONSOLE
          Serial.print("Roll Down um: ");
          Serial.print(((uint8_t)16) - linFrameData[3], DEC);
          Serial.println(" Ticks");
#endif
          Joystick.button(8, 1); // Press button "num" (1 to 32)
        }
        break;
      }
      case 0x07:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Ok");
#endif
        Joystick.button(9, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x12:
      {
        if (linFrameData[3] <= 0x07)
        {
#ifndef SKIP_CONSOLE
          Serial.print("Volume Up um: ");
          Serial.print(linFrameData[3], DEC);
          Serial.println(" Ticks");
#endif
          Keyboard.press(KEY_MEDIA_VOLUME_INC);
          // Joystick.button(10, 1); // Press button "num" (1 to 32)
        }
        if (linFrameData[3] > 0x07)
        {
#ifndef SKIP_CONSOLE
          Serial.print("Volume Down um: ");
          Serial.print(((uint8_t)16) - linFrameData[3], DEC);
          Serial.println(" Ticks");
#endif
          Keyboard.press(KEY_MEDIA_VOLUME_DEC);
          // Joystick.button(11, 1); // Press button "num" (1 to 32)
        }
        break;
      }
      case 0x15:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Next Song");
#endif
        Keyboard.press(KEY_MEDIA_NEXT_TRACK);
        // Joystick.button(12, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x16:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Previews Song");
#endif
        Keyboard.press(KEY_MEDIA_PREV_TRACK);
        // Joystick.button(13, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x19:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Speak");
#endif
        Joystick.button(14, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x20:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Mute");
#endif
        Keyboard.press(KEY_MEDIA_MUTE);
        Keyboard.press(KEY_MEDIA_PLAY_PAUSE);
        // Joystick.button(15, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x23:
      {
#ifndef SKIP_CONSOLE
        Serial.println("View");
#endif
        Joystick.button(16, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x25:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Heating");
#endif
        Joystick.button(17, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x70:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Cupra short");
#endif
        Joystick.button(3, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x71:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Cupra long");
#endif
        Joystick.button(4, 1); // Press button "num" (1 to 32)
        break;
      }
      case 0x74:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Lane Assist");
#endif
        Joystick.button(18, 1); // Press button "num" (1 to 32)
        break;
      }
      default:
      {
#ifndef SKIP_CONSOLE
        Serial.println("Default case doesnt know these button Code");
#endif
        // just to be sure we release every button here too
        Keyboard.releaseAll();
        Joystick.button(3, 0); // Release button "num" (1 to 32)
        Joystick.button(4, 0); // Release button "num" (1 to 32)
        Joystick.button(5, 0); // Release button "num" (1 to 32)
        Joystick.button(6, 0); // Release button "num" (1 to 32)
        Joystick.button(7, 0); // Release button "num" (1 to 32)
        Joystick.button(8, 0); // Release button "num" (1 to 32)
        Joystick.button(9, 0); // Release button "num" (1 to 32)
        // Joystick.button(10, 0); // Release button "num" (1 to 32)
        // Joystick.button(11, 0); // Release button "num" (1 to 32)
        // Joystick.button(12, 0); // Release button "num" (1 to 32)
        // Joystick.button(13, 0); // Release button "num" (1 to 32)
        Joystick.button(14, 0); // Release button "num" (1 to 32)
        // Joystick.button(15, 0); // Release button "num" (1 to 32)
        Joystick.button(16, 0); // Release button "num" (1 to 32)
        Joystick.button(17, 0); // Release button "num" (1 to 32)
        Joystick.button(18, 0); // Release button "num" (1 to 32)
        // used by bounce library for Start-Stop Button Joystick.button(19, 0); // Release button "num" (1 to 32)
        break;
      }
      }
    }
    else
    {
      Keyboard.releaseAll();
      Joystick.button(3, 0); // Release button "num" (1 to 32)
      Joystick.button(4, 0); // Release button "num" (1 to 32)
      Joystick.button(5, 0); // Release button "num" (1 to 32)
      Joystick.button(6, 0); // Release button "num" (1 to 32)
      Joystick.button(7, 0); // Release button "num" (1 to 32)
      Joystick.button(8, 0); // Release button "num" (1 to 32)
      Joystick.button(9, 0); // Release button "num" (1 to 32)
      // Joystick.button(10, 0); // Release button "num" (1 to 32)
      // Joystick.button(11, 0); // Release button "num" (1 to 32)
      // Joystick.button(12, 0); // Release button "num" (1 to 32)
      // Joystick.button(13, 0); // Release button "num" (1 to 32)
      Joystick.button(14, 0); // Release button "num" (1 to 32)
      // Joystick.button(15, 0); // Release button "num" (1 to 32)
      Joystick.button(16, 0); // Release button "num" (1 to 32)
      Joystick.button(17, 0); // Release button "num" (1 to 32)
      Joystick.button(18, 0); // Release button "num" (1 to 32)
      // used by bounce library for Start-Stop Button Joystick.button(19, 0); // Release button "num" (1 to 32)
    }

    lin.resetStateMachine();
    lin.resetError();
    for (uint8_t i = 0; i < sizeof(linFrameData); ++i)
    {
      linFrameData[i] = 0;
    }
    error = lin.receiveSlaveResponseBlocking(LIN_Master::LIN_V2, 0x0C, 4, linFrameData);
    if (error != LIN_Master::NO_ERROR)
    {
// we return form the guessing without any output to make our LogFile clean
#ifndef SKIP_CONSOLE
      Serial.print("error");
#endif
    }

    if ((linFrameData[0] & 0x0F) == 0x09 && (linFrameData[2] & 0x0F) == 0x09)
    {
#ifndef SKIP_CONSOLE
      Serial.println("SET");
#endif
      Joystick.button(20, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(20, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x0A && (linFrameData[2] & 0x0F) == 0x0A)
    {
#ifndef SKIP_CONSOLE
      Serial.println("RES");
#endif
      Joystick.button(21, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(21, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x0A && (linFrameData[3] & 0x0F) == 0x02)
    {
#ifndef SKIP_CONSOLE
      Serial.println("Gear Up");
#endif
      Joystick.button(1, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(1, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x09 && (linFrameData[3] & 0x0F) == 0x01)
    {
#ifndef SKIP_CONSOLE
      Serial.println("Gear Down");
#endif
      Joystick.button(2, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(2, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x09 && (linFrameData[1] & 0x0F) == 0x01)
    {
#ifndef SKIP_CONSOLE
      Serial.println("0/1");
#endif
      Joystick.button(22, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(22, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x00 && (linFrameData[1] & 0x0F) == 0x08)
    {
#ifndef SKIP_CONSOLE
      Serial.println("plus");
#endif
      Joystick.button(23, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(23, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x0C && (linFrameData[1] & 0x0F) == 0x04)
    {
#ifndef SKIP_CONSOLE
      Serial.println("minus");
#endif
      Joystick.button(24, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(24, 0); // Release button "num" (1 to 32)
    }

    if ((linFrameData[0] & 0x0F) == 0x04 && (linFrameData[3] & 0x0F) == 0x0C)
    {
#ifndef SKIP_CONSOLE
      Serial.println("Abstand");
#endif
      Joystick.button(25, 1); // Press button "num" (1 to 32)
    }
    else
    {
      Joystick.button(25, 0); // Release button "num" (1 to 32)
    }

    break;
  }

  default: // hopefully never triggerd but to be safe
  {
// just to be sure we release every button here too
#ifndef SKIP_CONSOLE
    Serial.println("Default Case");
#endif
    Keyboard.releaseAll();
    Joystick.button(3, 0); // Release button "num" (1 to 32)
    Joystick.button(4, 0); // Release button "num" (1 to 32)
    Joystick.button(5, 0); // Release button "num" (1 to 32)
    Joystick.button(6, 0); // Release button "num" (1 to 32)
    Joystick.button(7, 0); // Release button "num" (1 to 32)
    Joystick.button(8, 0); // Release button "num" (1 to 32)
    Joystick.button(9, 0); // Release button "num" (1 to 32)
    // Joystick.button(10, 0); // Release button "num" (1 to 32)
    // Joystick.button(11, 0); // Release button "num" (1 to 32)
    // Joystick.button(12, 0); // Release button "num" (1 to 32)
    // Joystick.button(13, 0); // Release button "num" (1 to 32)
    Joystick.button(14, 0); // Release button "num" (1 to 32)
    // Joystick.button(15, 0); // Release button "num" (1 to 32)
    Joystick.button(16, 0); // Release button "num" (1 to 32)
    Joystick.button(17, 0); // Release button "num" (1 to 32)
    Joystick.button(18, 0); // Release button "num" (1 to 32)
    // used by bounce library for Start-Stop Button  Joystick.button(19, 0); // Release button "num" (1 to 32)
    Joystick.button(20, 0); // Release button "num" (1 to 32)
    Joystick.button(21, 0); // Release button "num" (1 to 32)
    Joystick.button(1, 0);  // Release button "num" (1 to 32)
    Joystick.button(2, 0);  // Release button "num" (1 to 32)
    Joystick.button(22, 0); // Release button "num" (1 to 32)
    Joystick.button(23, 0); // Release button "num" (1 to 32)
    Joystick.button(24, 0); // Release button "num" (1 to 32)
    Joystick.button(25, 0); // Release button "num" (1 to 32)
    break;
  }
  }

} // loop()
