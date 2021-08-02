// Defines
#define NUM_SWITCHES 4
#define VBAT_AIN 32       // Vbat sense (2:1 divider)
#define CHRG_AIN 33       // Charge pin sense (10k pull-up)
#define CHRG_LOW 2000
//
#define BATTERY_LOW 2082  // Noise floor of 3.61V (<5%)
#define BATTERY_10 2127   // Noise floor of 3.69V (<10%)
#define BATTERY_20 2151   // Noise floor of 3.73V (<20%)
#define BATTERY_30 2174   // Noise floor of 3.77V (<30%)
#define BATTERY_40 2191   // Noise floor of 3.80V (<40%)
#define BATTERY_50 2214   // Noise floor of 3.84V (<50%)
#define BATTERY_60 2231   // Noise floor of 3.87V (<60%)
#define BATTERY_70 2277   // Noise floor of 3.95V (<70%)
#define BATTERY_80 2318   // Noise floor of 4.02V (<80%)
#define BATTERY_90 2352   // Noise floor of 4.08V (<85%) (was 4.11V for 90%)
#define BATTERY_100 2422  // Noise floor of 4.20V (100%)
//
#define BATTERY_HIGH 2256 // Noise floor of 3.91V (>66%)
#define BATTERY_MAX 2290  // Noise floor of 3.97V (>80%)
#define BATTERY_CHRG 2451 // Totally empirical, adjust as required. Currently 4.25V-ish
#define VBAT_NUM 10       // Number of vbat readings to average

// Globals
static int iRSSI = 0;                           // BLE signal strength
int vbat_ring_count = 0;
int vbat_result;                                // For battery monitoring
int temp;   
int vbat_ring_sum = 0;
int chrg_result;                                // For charge state monitoring
int sw_val[NUM_SWITCHES];     
int sw_pin[]{17,5,18,23};                       // Switch gpio numbers

                                                // SW1 Toggle Drive 
                                                // SW2 Toggle Modulation
                                                // SW3 Toggle Delay
                                                // SW4 Toggle Reverb
                                                // SW5 Decrement preset
                                                // SW6 Increment preset
                                            
const unsigned long longPressThreshold = 1000;  // the threshold (in milliseconds) before a long press is detected
const unsigned long debounceThreshold = 50;     // the threshold (in milliseconds) for a button press to be confirmed (i.e. not "noise")
unsigned long buttonTimer[NUM_SWITCHES];        // stores the time that the button was pressed (relative to boot time)
unsigned long buttonPressDuration[NUM_SWITCHES];// stores the duration (in milliseconds) that the button was pressed/held down for
boolean buttonActive[NUM_SWITCHES];             // indicates if the button is active/pressed
boolean longPressActive[NUM_SWITCHES];          // indicate if the button has been long-pressed
boolean AnylongPressActive = false;             // OR of any longPressActive states
boolean latchpress;                             // latch to stop repeating the long press state

// Flags
boolean isStatusReceived;                       // Status received from Spark
boolean isOLEDUpdate;                           // Flag OLED needs refresh
boolean isPedalMode;                            // Pedal mode: 0 = preset, 1 = effect
boolean isHWpresetgot;                          // Flag to show that the hardware preset number has been processed
