/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
 
 Author: KTOWN (Kevin Townsend)
 Copyright (C) Adafruit Industries 2017
*********************************************************************/

/*  This example constantly advertises a custom 128-bit UUID, and is
 *  intended to be used in combination with a Central sketch that scans
 *  for this UUID, and then displays an alert message, sorting matching
 *  devices by their RSSI level which is an approximate indication of
 *  distance (although highly subject to environmental obstacles).
 *  
 *  By including a custom UUID in the advertising packet, we can easily
 *  filter the scan results on the Central device, rather than manually
 *  parsing the advertising packet(s) of every device in range.
 *  
 *  This example is intended to be run with the *_central.ino version
 *  of this application.
 */

#include <bluefruit.h>
#include <ble_gap.h>

// Software Timer for blinking RED LED
SoftwareTimer blinkTimer;

// This will select the UUID used for the bird
// CHANGE THIS FOR EACH BIRD
#define SELECTED_BIRD_ID 0

#define DEBUG 0

// Note that the byte order is reversed
const uint8_t BIRD_IDS[3][16] =
{
  {
    // Bird 1
    // 784b4c66-fec9-41cc-b82c-59bebbe6654d
    0x4D, 0x65, 0xE6, 0xBB, 0xBE, 0x59, 0x2C, 0xB8,
    0xCC, 0x41, 0xC9, 0xFE, 0x66, 0x4C, 0x4B, 0x78
  },
  {
    // Bird 2
    // 3bb2049e-032c-42ba-8e76-54f978405c45
    0x4D, 0x65, 0xE6, 0xBB, 0xBE, 0x59, 0x2C, 0xB8,
    0xCC, 0x41, 0xC9, 0xFE, 0x66, 0x4C, 0x4B, 0x78
  },
  {
    // Bird 3
    // 403f0fe1-c41e-4c82-832b-b282c54fc42a
    0x2A, 0xC4, 0x4F, 0xC5, 0x82, 0xB2, 0x2B, 0x83,
    0x82, 0x4C, 0x1E, 0xC4, 0xE1, 0x0F, 0x3F, 0x40
  }
  // {
  //   // Bird 4
  //   // 78e52ff1-a0ac-4de2-8807-2c5c9c43c1f7
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x,
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x
  // },
  // {
  //   // Bird 5
  //   // f758ea44-93ac-48ca-8688-7ad56ffa5a7f
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x,
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x
  // },
  // {
  //   // Bird 6
  //   // 790039d1-61c8-4c93-aa07-3c2fdc07e9cd
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x,
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x
  // },
  // {
  //   // Bird 7
  //   // 952eeeef-3b3a-4327-b035-0fb664162f21
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x,
  //   0x, 0x, 0x, 0x, 0x, 0x, 0x, 0x
  // }    
};

BLEUuid uuid = BLEUuid(BIRD_IDS[ SELECTED_BIRD_ID ]);

void setup() 
{
  if (DEBUG) Serial.begin(115200);
  if (DEBUG) while ( !Serial ) delay(10);   // for nrf52840 with native usb

  if (DEBUG) Serial.println("Bluefruit52 Peripheral Proximity Example");
  if (DEBUG) Serial.println("----------------------------------------\n");

  // Initialize blinkTimer for 1000 ms and start it
  blinkTimer.begin(1000, blink_timer_callback);
  blinkTimer.start();

  if (!Bluefruit.begin())
  {
    if (DEBUG) Serial.println("Unable to init Bluefruit");
    while(1)
    {
      digitalToggle(LED_RED);
      delay(100);
    }
  }
  else
  {
    if (DEBUG) Serial.println("Bluefruit initialized (peripheral mode)");
  }

  Bluefruit.setTxPower(8);    // Check bluefruit.h for supported values

  // Set up and start advertising
  startAdv();

  if (DEBUG) Serial.println("Advertising started"); 
}

void startAdv(void)
{   
  // Note: The entire advertising packet is limited to 31 bytes!
  
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Preferred Solution: Add a custom UUID to the advertising payload, which
  // we will look for on the Central side via Bluefruit.Scanner.filterUuid(uuid);
  // A valid 128-bit UUID can be generated online with almost no chance of conflict
  // with another device or etup
  Bluefruit.Advertising.addUuid(uuid);

  Bluefruit.setName("birb");

  // Not enough room in the advertising packet for name
  // so store it in the Scan Response instead
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html
   */

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in units of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start();
}

void loop() 
{
}

/**
 * Software Timer callback is invoked via a built-in FreeRTOS thread with
 * minimal stack size. Therefore it should be as simple as possible. If
 * a periodically heavy task is needed, please use Scheduler.startLoop() to
 * create a dedicated task for it.
 * 
 * More information http://www.freertos.org/RTOS-software-timer.html
 */
void blink_timer_callback(TimerHandle_t xTimerID)
{
  (void) xTimerID;
  digitalToggle(LED_RED);
}

