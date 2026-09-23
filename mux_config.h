#ifndef MUX_CONFIG_H
#define MUX_CONFIG_H

#pragma once
#include <stddef.h>  // for NULL
#include <stdint.h>  // for uint8_t
#include "keycodes.h"
//#include "quantum_keycodes.h"

// SENSOR SETTINGS

// Debounce time in milliseconds
#define DEBOUNCE_MS 1

// Calibration settings
// Number of raw ADC samples to average when calibrating each channel
#define CALIBRATION_SAMPLES 5
#define CALIBRATION_THRESHOLD_PERCENT 70

// Minimum ADC reading considered valid. Channels with values below this are
// treated as floating/unconnected and ignored during calibration & scanning.
#define ADC_MIN_VALID 200

//left MUX "select" pins

#define MUX1_SEL0 GP15
#define MUX1_SEL1 GP14
#define MUX1_SEL2 GP13
#define MUX1_SEL3 GP12
#define MUX1_SEL4 GP11

#define MUX2_SEL0 GP9
#define MUX2_SEL1 GP8
#define MUX2_SEL2 GP7
#define MUX2_SEL3 GP6
#define MUX2_SEL4 GP5

//left chip select pins
#define MUX_CS1 GP10
#define MUX_CS2 GP4

//left write/latch pin (active low, latch on rising edge)
#define LMUX_WR GP4

//ADC pin
#define MUX1_ADC GP28
#define MUX2_ADC GP29

//Hall effect sensor threash hold - key pressed when ADC is BELOW this value
#define SENSOR_THRESHOLD 400 // Shego75HE repo value

//Right MUX "select" pins
#define MUX3_SEL0 GP14
#define MUX3_SEL1 GP13
#define MUX3_SEL2 GP12
#define MUX3_SEL3 GP11
#define MUX3_SEL4 GP10

#define MUX4_SEL0 GP8
#define MUX4_SEL1 GP7
#define MUX4_SEL2 GP6
#define MUX4_SEL3 GP5
#define MUX4_SEL4 GP4

//left chip select pins
#define MUX_CS3 GP9
#define MUX_CS4 GP3

//left write/latch pin (active low, latch on rising edge)
#define RMUX_WR GP2

//ADC pin
#define MUX3_ADC GP26
#define MUX4_ADC GP27

//Hall effect sensor threash hold - key pressed when ADC is BELOW this value
#define RSENSOR_THRESHOLD 400 // Shego75HE repo value


#define MAX_KEYS 96 // 6 rows * 16 columns 

typedef enum KeyName{
    // Row 0 (matrix row 0) - 16 Keys: positions [0,0] to [0,15]
    K_ESC = 1, K_F1, K_F2, K_F3, K_F4, K_F5, K_F6, _PAD_R1_1, _PAD_R1_2,        
// |---------|-----|-----|-----|-----|-----|-----|                  
    // Row 1 (matrix row 1) - 15 Keys: positions [1,0] to [1,14], skip [1, 15]
    K_GRAVE, K_1, K_2, K_3, K_4, K_5, K_6, _PAD_R2_1, _PAD_R2_2,                      
// |-------|----|----|----|----|----|----|                          
    // Row 2 (matrix row 2) - 15 Keys: positions [2,0] to [2,14], skip [2,15]
    K_TAB, K_Q, K_W, K_E, K_R, K_T, _PAD_R3_1, _PAD_R3_2, _PAD_R3_3,                     
// |-----|----|----|----|----|----|                                 
    // Row 3 (matrix row 3) - 14 Keys: positions [3,0] to [3,13], skip [3,14-15]
    K_CAPS, K_A, K_S, K_D, K_F, K_G, _PAD_R4_1, _PAD_R4_2, _PAD_R4_3,              
// |------|----|----|----|----|----|                                
    // Row 4 (matrix row 4) - 15 Keys: positions [4,0] to [4,12], skip [4,13-15]
    K_LSHIFT, K_NUBS, K_Z, K_X, K_C, K_V, K_B, _PAD_R5_1, _PAD_R5_2,         
// |--------|-------|----|----|----|----|----|                      
    // Row 5 (matrix row 5) - 12 Keys: positions [5,0] to [5,9], skip [5,10-15]
    K_LCTRL, K_LWIN, K_LALT, MO1_L, K_SPACE_L, _PAD_R6_1, _PAD_R6_2, _PAD_R6_3, _PAD_R6_4,
// |-------|-------|-------|------|----------|                      


    K_F7, K_F8, K_F9, K_F10, K_F11, K_F12, K_PSCR, K_PAUS, K_DEL,
// |----|-----|-----|------|------|------|-------|-------|------|

    K_7, K_8, K_9, K_0, K_MINUS, K_EQUAL, K_BACKSPACE, K_HOME, _PAD_R8, //padding to align to 16 column matrix 
// |---|----|----|----|--------|--------|------------|-------|  

    K_Y, K_U, K_I, K_O, K_P, K_LBRC, K_RBRC, K_NUHS, K_PGUP, _PAD_R9, //padding to align to 16 column matrix
// |---|----|----|----|----|-------|-------|-------|-------|

    K_H, K_J, K_K, K_L, K_SEMI, K_QUOTE, K_ENTER, K_PGDN, _PAD_R10_1, _PAD_R10_2, // Padding to align to 16-column matrix
// |---|----|----|----|-------|--------|--------|-------|

    K_N, K_M, K_COMMA, K_DOT, K_SLASH, K_RSHIFT, K_UP, K_END, _PAD_R11, // Padding to align to 15-column matrix
// |---|----|--------|------|--------|---------|-----|------|

    K_SPACE_R, K_RALT, MO1_R, K_RCTRL, K_LEFT, K_DOWN, K_RIGHT,
// |---------|-------|------|--------|-------|-------|--------|

    SENSOR_COUNT_PLUS_1 // To get total sensor count
} KeyName;

// Struct for each channel entry
typedef struct {
    KeyName sensor;   // which key/sensor this channel corresponds to
} mux32_ref_t;

#define KEY_COUNT (SENSOR_COUNT_PLUS_1 - 1)

// Extern declarations for each MUX table
extern const char *sensor_names[KEY_COUNT];
extern const uint16_t sensor_to_keycode[KEY_COUNT];
extern const mux32_ref_t mux1_channels[32];
extern const mux32_ref_t mux2_channels[32];
extern const mux32_ref_t mux3_channels[32];
extern const mux32_ref_t mux4_channels[32]; // added an extra mux because I have 4 or them






// QMK Matrix functions
    //void matrix_init_custom(void);
    //bool matrix_scan_custom(matrix_row_t current_matrix[]);

// Auto-calibration function
    //void calibrate_sensors(void);

// Set a per-key sensitivity percent by key index
// percent: sensitivity percent (e.g., 10 => trigger when value deviates +/-10% from baseline)
    //void set_key_threshold(uint16_t key_idx, uint8_t percent);

#endif