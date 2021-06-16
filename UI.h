// Defines
#define NUM_SWITCHES 6
#define VBAT_AIN 32

// Globals
int vbat_result;                                // For eventual battery monitoring

int sw_val[NUM_SWITCHES];                   
int sw_pin[]{19,18,4,16,5,23};                  // Switch gpio numbers
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
bool isBTConnected;                             // Duh
bool isStatusReceived;                          // Status received from Spark
bool isOLEDUpdate;                              // Flag OLED needs refresh
bool isPedalMode;                               // Pedal mode: 0 = preset, 1 = effect
