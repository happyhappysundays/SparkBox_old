# SparkBox V0.56
This is a Dev Kit 32 version of SparkBox. SparkBox is another BT pedal for the Positive Grid Spark 40.  I only needed the functionality of the simpler BT pedals. However many of them use captured hex chunks to communicate with the Spark, or were Python based. Instead I wanted to use Paul Hamshere's amazing code to create and process real messages. Also I wanted to extend the functionality a bit and make a pretty UI.

# Functions
- Expression pedal input on D34 for altering the current parameter or on/off switch
- Uses BLE so that it can be used with music function of the Spark app
- Allows connection of the app for full simultaneous control
- Switch presets either on footswitch, app or Spark to update display
- Switch on and off all four major effects dynamically
- Graphically display the effect state on the display
- Supports 4-button pedals
- Hold down any switch for 1s to switch between Effect mode and Preset mode
- Battery level indicator on UI
- BLE RSSI inidcator

# Arduino libraries and board versions
Under Files->Preferences->Additional Boards Manager URLs, enter the following:
- https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
- https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.5/package_heltec_esp32_index.json

Under Tools->Board->Board manager ensure that you have the following version:
- ESP32 by Espressif 1.0.4 (SparkBox - ESP32 Dev Module)

Under Tools->Manage Libraries nsure that you have the following libraries and versions:
- ThingPulse SSD1306 driver 4.2.1 (SparkBox)
- NimBLE=Arduino 1.3.1

# Compile options

Uncomment ONE battery option to match your hardware.

**define BATT_CHECK_0**

You have no mods to monitor the battery, so it will show empty (default).

**define BATT_CHECK_1**

You are monitoring the battery via a 2:1 10k/10k resistive divider to GPIO23.
You can see an accurate representation of the remaining battery charge and a kinda-sorta
indicator of when the battery is charging. Maybe.

**define BATT_CHECK_2**

You have the battery monitor mod described above AND you have a connection between the 
CHRG pin of the charger chip and GPIO 33. Go you! Now you have a guaranteed charge indicator too.

**define EXPRESSION_PEDAL**

Expression pedal define. Comment this out if you DO NOT have the expression pedal mod.

![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6791.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/SparkBox.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6785.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6786.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/thumbnail_IMG_6994.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/V0_4.jpg?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/SparkBox_Heltec_Exp.png?raw=true)
![alt text](https://github.com/happyhappysundays/SparkBox_old/blob/main/Pictures/SparkBox_Battery.png?raw=true)

# Parts list

| Item | Description           | Link               |
| -----| ----------------------|--------------------|
|   1  | Box                   |https://www.aliexpress.com/item/32693268669.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   2  | Stomp switch          |https://www.aliexpress.com/item/32918205335.html?spm=a2g0s.9042311.0.0.27424c4dszp4Ie
|   3  | ESP-WROOM-32U module  |https://www.aliexpress.com/item/32864722159.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   4  | LCD screen            |https://www.ebay.com.au/itm/333085424031
|   5  | BT antenna            |https://www.aliexpress.com/item/4001054693109.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH and https://www.ebay.com.au/itm/233962468558
|   6  | USB extension         |https://www.aliexpress.com/item/32808991941.html?spm=a2g0s.9042311.0.0.27424c4dlzGiUH
|   7  | Power switch          |https://www.jaycar.com.au/dpdt-miniature-toggle-switch-solder-tag/p/ST0355
|   8  | DC input jack         |https://www.jaycar.com.au/2-5mm-bulkhead-male-dc-power-connector/p/PS0524
|   9  | Pedal jack            |https://www.jaycar.com.au/6-5mm-stereo-enclosed-insulated-switched-socket/p/PS0184
|  10  | LiPo battery          |https://www.ebay.com.au/itm/133708965813
|  11  | LiPo charger          |https://www.ebay.com.au/itm/161821599467
|  12  | LiPo booster          |https://www.jaycar.com.au/arduino-compatible-5v-dc-to-dc-converter-module/p/XC4512
|  13  | 9V to 5V converter    |https://www.ebay.com.au/itm/303839459634
|  14  | Glass window (opt)    |https://www.aliexpress.com/item/4000377316108.html?spm=a2g0s.12269583.0.0.1a1e62440DlgU2

