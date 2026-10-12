// HALLSCAN MODULE - Hall effect sensor matrix scanning implementation
// Based on shego75_breadboard approach
#include <stdbool.h>
#include <stdint.h>
#include "info_config.h"
#include "keyboard.h"
#include "mux_config.h"
#include QMK_KEYBOARD_H
#include "hallscan.h"
#include "mux_keymap.h"
#include "uart.h"
#include "gpio.h"
#include "matrix.h"
#include "quantum.h"
#include "analog.h"
#include "wait.h"
#include "timer.h"
#include "print.h"
#include "split_util.h"
#include <stdio.h>
#include <string.h>


// ========================================
// INTERNAL STATE
// ========================================

// Key state tracking for debouncing
static bool key_pressed[MAX_KEYS];
static uint32_t key_timer[MAX_KEYS];
//static uint32_t last_debug_time = 0;
static bool debug_mode = true;
static bool last_state = false;
static bool rapid_trigger = true;


// ========================================
// MUX CHANNEL MAPPINGS
// ========================================
// These map physical MUX channels to logical sensor IDs
// The actual mappings are defined in hallscan_keymap.h


static uint16_t adc_values[MAX_KEYS];  // Store all ADC values for display
static uint32_t last_adc_print_time = 0;
#define ADC_PRINT_INTERVAL_MS 1000  // Print every 1000ms (1 second)

// Key state tracking for 81 keys (6 rows × 15 cols max)
// Using MATRIX_ROWS * MATRIX_COLS = 6 * 15 = 90 slots to be safe

// Auto-calibration storage
static uint16_t key_baseline[MAX_KEYS];      // Baseline (resting) ADC value for each key
static uint16_t key_threshold[MAX_KEYS];     // (legacy) absolute threshold - kept for compatibility
static uint8_t key_sensitivity_percent[MAX_KEYS]; // Sensitivity percent per key (deviation percent)
static bool calibration_complete = false;    // Flag indicating calibration status

// ========================================
// HELPER FUNCTIONS
// ========================================

static void writePin(pin_t pin, uint8_t level){
    gpio_write_pin(pin, level);
}

static void writePinLow(pin_t pin){
    gpio_write_pin_low(pin);
}

static void writePinHigh(pin_t pin){
    gpio_write_pin_high(pin);
}

static void setPinOutput(pin_t pin){
    gpio_set_pin_output(pin);
}

static void select_lmux_channel(uint8_t channel) {
    writePin(MUX1_SEL0, (channel & 0x01) ? 1 : 0);
    writePin(MUX1_SEL1, (channel & 0x02) ? 1 : 0);
    writePin(MUX1_SEL2, (channel & 0x04) ? 1 : 0);
    writePin(MUX1_SEL3, (channel & 0x08) ? 1 : 0);
    writePin(MUX1_SEL4, (channel & 0x10) ? 1 : 0);

    writePin(MUX2_SEL0, (channel & 0x01) ? 1 : 0);
    writePin(MUX2_SEL1, (channel & 0x02) ? 1 : 0);
    writePin(MUX2_SEL2, (channel & 0x04) ? 1 : 0);
    writePin(MUX2_SEL3, (channel & 0x08) ? 1 : 0);
    writePin(MUX2_SEL4, (channel & 0x10) ? 1 : 0);

    writePinLow(LMUX_WR);
    wait_us(5); // short pulse to ensure latch
    writePinHigh(LMUX_WR);
}


static void select_rmux_channel(uint8_t channel) {
    writePin(MUX3_SEL0, (channel & 0x01) ? 1 : 0);
    writePin(MUX3_SEL1, (channel & 0x02) ? 1 : 0);
    writePin(MUX3_SEL2, (channel & 0x04) ? 1 : 0);
    writePin(MUX3_SEL3, (channel & 0x08) ? 1 : 0);
    writePin(MUX3_SEL4, (channel & 0x10) ? 1 : 0);

    writePin(MUX4_SEL0, (channel & 0x01) ? 1 : 0);
    writePin(MUX4_SEL1, (channel & 0x02) ? 1 : 0);
    writePin(MUX4_SEL2, (channel & 0x04) ? 1 : 0);
    writePin(MUX4_SEL3, (channel & 0x08) ? 1 : 0);
    writePin(MUX4_SEL4, (channel & 0x10) ? 1 : 0);

    writePinLow(RMUX_WR);
    wait_us(5); // short pulse to ensure latch
    writePinHigh(RMUX_WR);
}

void matrix_init_custom(void) {
    // Minimal MUX pin setup
    if(is_keyboard_left()){
        setPinOutput(MUX1_SEL0);
        setPinOutput(MUX1_SEL1);
        setPinOutput(MUX1_SEL2);
        setPinOutput(MUX1_SEL3);
        setPinOutput(MUX1_SEL4);

        setPinOutput(MUX2_SEL0);
        setPinOutput(MUX2_SEL1);
        setPinOutput(MUX2_SEL2);
        setPinOutput(MUX2_SEL3);
        setPinOutput(MUX2_SEL4);

        #ifdef LMUX_WR
        setPinOutput(LMUX_WR);
        writePinHigh(LMUX_WR);
        #endif

        #ifdef MUX_CS1
        setPinOutput(MUX_CS1);
        writePinHigh(MUX_CS1);
        #endif
        #ifdef MUX_CS2
        setPinOutput(MUX_CS2);
        writePinHigh(MUX_CS2);
        #endif
    }else {

        setPinOutput(MUX3_SEL0);
        setPinOutput(MUX3_SEL1);
        setPinOutput(MUX3_SEL2);
        setPinOutput(MUX3_SEL3);
        setPinOutput(MUX3_SEL4);

        setPinOutput(MUX4_SEL0);
        setPinOutput(MUX4_SEL1);
        setPinOutput(MUX4_SEL2);
        setPinOutput(MUX4_SEL3);
        setPinOutput(MUX4_SEL4);

        #ifdef RMUX_WR
            setPinOutput(RMUX_WR);
            writePinHigh(RMUX_WR);
        #endif

        #ifdef MUX_CS3
        setPinOutput(MUX_CS3);
        writePinHigh(MUX_CS3);
        #endif
        #ifdef MUX_CS4
        setPinOutput(MUX_CS4);
        writePinHigh(MUX_CS4);
        #endif
    }

    // Initialize key state arrays
    for (uint8_t i = 0; i < MAX_KEYS; i++) {
        key_pressed[i] = false;
        key_timer[i] = 0;
        key_baseline[i] = 0;
        key_threshold[i] = SENSOR_THRESHOLD; // Use default until calibration completes
    }
}

// Auto-calibration: Scan all keys and establish baseline + dynamic thresholds
// This should be called after matrix_init_custom, during keyboard_post_init_kb
void calibrate_sensors(void) {

    pin_t ladc_pins[2] = {MUX1_ADC, MUX2_ADC};
    const mux32_ref_t* lmux_tables[2] = {mux1_channels, mux2_channels};

    pin_t radc_pins[2] = {MUX3_ADC, MUX4_ADC};
    const mux32_ref_t* rmux_tables[2] = {mux3_channels, mux4_channels};


    // Perform multiple reads per key and average them for stability
    //const uint8_t CALIBRATION_SAMPLES = 5;
    uint32_t sample_accumulator[MAX_KEYS] = {0};
    uint8_t sample_count[MAX_KEYS] = {0};

    // Collect samples
    for (uint8_t sample = 0; sample < CALIBRATION_SAMPLES; sample++) {
        for (uint8_t mux_idx = 0; mux_idx < 2; mux_idx++) {
            // Select appropriate CS
    if(is_keyboard_left()){
            #ifdef MUX_CS1
                if (mux_idx == 0) { writePinLow(MUX_CS1); } else { writePinHigh(MUX_CS1); }
            #endif
            #ifdef MUX_CS2
                if (mux_idx == 1) { writePinLow(MUX_CS2); } else { writePinHigh(MUX_CS2); }
            #endif
    }else{
            #ifdef MUX_CS3
                if (mux_idx == 0) { writePinLow(MUX_CS3); } else { writePinHigh(MUX_CS3); }
            #endif
            #ifdef MUX_CS4
                    if (mux_idx == 1) { writePinLow(MUX_CS4); } else { writePinHigh(MUX_CS4); }
            #endif
    }

            for (uint8_t ch = 0; ch < 32; ch++) {
                if(is_keyboard_left()){
                    select_lmux_channel(ch);
                }
                else {
                    select_rmux_channel(ch);
                }
                    wait_us(100);

                uint16_t adc_val = analogReadPin(is_keyboard_left()? ladc_pins[mux_idx] : radc_pins[mux_idx]);

                // Filter out anomalous high values
                const uint16_t ADC_MAX_VALID = 800;
                if (adc_val > ADC_MAX_VALID) {
                    adc_val = 4095;
                }

                const mux32_ref_t* key_mapping = is_keyboard_left()? &lmux_tables[mux_idx][ch] : &rmux_tables[mux_idx][ch];

                if (!key_mapping || key_mapping->sensor == 0) continue;

                uint16_t sensor = key_mapping->sensor;
                uint8_t matrix_row = (sensor - 1) / MATRIX_COLS;
                uint8_t matrix_col = (sensor - 1) % MATRIX_COLS;

                if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;

                //uint16_t key_idx = (matrix_row * MATRIX_COLS) + matrix_col;
                uint16_t key_idx = (sensor - 1);

                if (key_idx >= MAX_KEYS) continue;

                // Accumulate valid readings only (skip obvious disconnects)
                if (adc_val < 4000) {
                    sample_accumulator[key_idx] += adc_val;
                    sample_count[key_idx]++;
                }
            }


            // Release CS
            if(is_keyboard_left()){
                #ifdef MUX_CS1
                    writePinHigh(MUX_CS1);
                #endif
                #ifdef MUX_CS2
                    writePinHigh(MUX_CS2);
                #endif
            }else{
                #ifdef MUX_CS3
                    writePinHigh(MUX_CS3);
                #endif
                #ifdef MUX_CS4
                    writePinHigh(MUX_CS4);
                #endif
            }

//#ifdef MUX_CS3
//            writePinHigh(MUX_CS3);
//#endif
        }
        wait_ms(10); // Small delay between calibration samples
    }

    // Calculate baseline and threshold for each key
    for (uint8_t key_idx = 0; key_idx < MAX_KEYS; key_idx++) {
        if (sample_count[key_idx] > 0) {
            // Calculate average baseline
            key_baseline[key_idx] = sample_accumulator[key_idx] / sample_count[key_idx];

            // Initialize sensitivity percent to a default value (deviation percent)
            // e.g., default 4% -> trigger when value deviates +/-4% from baseline
            // Lower = more sensitive, Higher = less sensitive (requires harder press)
            // 4% is a good balance: responsive without noise/crosstalk false triggers
            //const uint8_t DEFAULT_SENSITIVITY_PERCENT = 4;
            const uint8_t DEFAULT_SENSITIVITY_PERCENT = 20; //rn used to set threshold value between 1-180
            key_sensitivity_percent[key_idx] = DEFAULT_SENSITIVITY_PERCENT;

            // Keep an absolute fallback threshold (lower bound) for compatibility
            key_threshold[key_idx] = (key_baseline[key_idx] * CALIBRATION_THRESHOLD_PERCENT) / 100;

            // Safety clamps: ensure threshold is reasonable
            if (key_threshold[key_idx] < 100) {
                key_threshold[key_idx] = 100; // Minimum threshold
            }
            if (key_threshold[key_idx] > 700) {
                key_threshold[key_idx] = 700; // Maximum threshold
            }
        } else {
            // No valid readings - use default
            key_baseline[key_idx] = 512;
            key_threshold[key_idx] = SENSOR_THRESHOLD;
        }
    }

    calibration_complete = true;
}


// Allow external modules to set a per-key sensitivity percent (deviation percent)
// percent: e.g., 10 => trigger when ADC deviates +/-10% from stored baseline
void set_key_threshold(uint16_t key_idx, uint8_t percent) {
    if (key_idx >= MAX_KEYS) return;

    // Clamp percent to sane values (1-90)
    if (percent < 1) percent = 1;
    if (percent > 90) percent = 90;

    key_sensitivity_percent[key_idx] = percent;

    // Also update legacy absolute threshold for compatibility (lower bound only)
    uint16_t base = key_baseline[key_idx] ? key_baseline[key_idx] : 512;
    uint32_t abs_t = ((uint32_t)base * (uint32_t)(100 - percent)) / 100;
    if (abs_t < 100) abs_t = 100;
    if (abs_t > 700) abs_t = 700;
    key_threshold[key_idx] = (uint16_t)abs_t;
}

bool left_side_scan(matrix_row_t current_matrix[]){

    bool changed = false;
    uint32_t now = timer_read32();

    // Throttle matrix scan to ~667Hz (1.5ms between scans) - faster for better response
    static uint32_t last_scan = 0;
    if (timer_elapsed32(last_scan) < 2) {  // QMK timer is 1ms resolution, so use 2ms
        return false;
    }
    last_scan = now;

    // Clear matrix output
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
    }

    // ADC pins and mapping table
        pin_t ladc_pins[2] = {MUX1_ADC, MUX2_ADC};
        const mux32_ref_t* lmux_tables[2] = {mux1_channels, mux2_channels};


    const uint16_t ADC_GND_THRESHOLD = 100; // skip obviously unconnected channels

    // Scan each mux and channel
    for (uint8_t mux_idx = 0; mux_idx < 2; mux_idx++) {
        // Select appropriate CS
        #ifdef MUX_CS1
        if (mux_idx == 0) { writePinLow(MUX_CS1); } else { writePinHigh(MUX_CS1); }
        #endif
        #ifdef MUX_CS2
        if (mux_idx == 1) { writePinLow(MUX_CS2); } else { writePinHigh(MUX_CS2); }
        #endif

        for (uint8_t ch = 0; ch < 32; ch++) {
            select_lmux_channel(ch);
            wait_us(10);  // Increased delay without filter caps to allow signal settling

            uint16_t adc_val = analogReadPin(ladc_pins[mux_idx]);



            // Filter out anomalous high values (crosstalk without filter caps)
            // Valid Hall sensor range: 0 (pressed) to ~512 (released)
            // Values above 800 are likely crosstalk
            const uint16_t ADC_MAX_VALID = 800;
            if (adc_val > ADC_MAX_VALID) {
                adc_val = 4095;  // Treat as invalid/unpressed
            }

            const mux32_ref_t* key_mapping = &lmux_tables[mux_idx][ch];

            if (!key_mapping) continue;

            // If channel is unmapped and reads near GND, skip
            if (key_mapping->sensor == 0) {
                if (adc_val <= ADC_GND_THRESHOLD) continue;
                else continue;
            }

            uint16_t sensor = key_mapping->sensor;
            if (sensor == 0) continue;

            // Map sensor to matrix position
            uint16_t key_idx = (sensor - 1);
            uint8_t matrix_row = key_idx / MATRIX_COLS;
            uint8_t matrix_col = key_idx % MATRIX_COLS;


            if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;

            //uint16_t key_idx = (matrix_row * MATRIX_COLS) + matrix_col;
            if (key_idx >= MAX_KEYS) continue;




            // Use dynamic per-key sensitivity if calibration is complete
            bool should_press = false;


            if (calibration_complete) {
                uint16_t base = key_baseline[key_idx] ? key_baseline[key_idx] : 512;
                //uint8_t sens = key_sensitivity_percent[key_idx] ? key_sensitivity_percent[key_idx] : 10; // percent
                uint8_t sens = key_sensitivity_percent[key_idx] ? key_sensitivity_percent[key_idx] : 20; // value where it should press range from 1 - 180

                // Compute lower and upper bounds based on percent deviation
                // uint32_t lower = ((uint32_t)base * (100 - sens)) / 100;
                // uint32_t upper = ((uint32_t)base * (100 + sens)) / 100;
                int16_t lower = base - adc_val;
                int16_t upper = -lower;
                if (lower < 1) lower = 0;
                if (upper < 1) upper = 0;

                //int16_t prev_lower = base - adc_values[key_idx];
                //int16_t prev_upper = -prev_lower;
                //if (prev_lower < 1) prev_lower = 0;
                //if (prev_upper < 1) prev_upper = 0;

                int16_t key_direction = adc_val - adc_values[key_idx];

                // Safety clamps
                //if (lower < 1) lower = 1;

                // Press if value deviates below lower OR above upper
                should_press = (lower > sens) || (upper > sens);
                //Rapid trigger (at least my take on it)
                if(rapid_trigger){
                    bool going_down = (key_direction <= 2);
                    bool going_up = (key_direction >= 2);
                    if(going_down)last_state = true;
                    else if (going_up)last_state = false;
                    should_press &= last_state;
                }

            } else {
                // Fallback to legacy absolute threshold
                uint16_t threshold = key_threshold[key_idx] ? key_threshold[key_idx] : SENSOR_THRESHOLD;
                should_press = (adc_val < threshold);
            }

            // Store ADC value for debug display (show filtered value)
            if (key_idx < KEY_COUNT) {
                adc_values[key_idx] = adc_val;
            }
            // Debounce: only change state if debounce time elapsed
            if (timer_elapsed32(key_timer[key_idx]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx]) {
                    key_pressed[key_idx] = should_press;
                    key_timer[key_idx] = now;
                    changed = true;
                }
            }

            if (key_pressed[key_idx]) {
                current_matrix[matrix_row] |= (1 << matrix_col);
            }
        }

        // Release CS for this MUX (keep others disabled)

                #ifdef MUX_CS1
                    writePinHigh(MUX_CS1);
                #endif
                #ifdef MUX_CS2
                    writePinHigh(MUX_CS2);
                #endif

    }
    return changed;
}


bool right_side_scan(matrix_row_t current_matrix[]){

    bool changed = false;
    uint32_t now = timer_read32();

    // Throttle matrix scan to ~667Hz (1.5ms between scans) - faster for better response
    static uint32_t last_scan = 0;
    if (timer_elapsed32(last_scan) < 2) {  // QMK timer is 1ms resolution, so use 2ms
        return false;
    }
    last_scan = now;

    // Clear matrix output
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
    }

    // Clear ADC values array
    for (uint8_t i = 0; i < MAX_KEYS; i++) {
        adc_values[i] = 0;
    }

    // ADC pins and mapping table
        pin_t radc_pins[2] = {MUX3_ADC, MUX4_ADC};
        const mux32_ref_t* rmux_tables[2] = {mux3_channels, mux4_channels};


    const uint16_t ADC_GND_THRESHOLD = 100; // skip obviously unconnected channels

    // Scan each mux and channel
    for (uint8_t mux_idx = 0; mux_idx < 2; mux_idx++) {
        // Select appropriate CS

        #ifdef MUX_CS3
                if (mux_idx == 0) { writePinLow(MUX_CS3); } else { writePinHigh(MUX_CS3); }
        #endif
        #ifdef MUX_CS4
                if (mux_idx == 1) { writePinLow(MUX_CS4); } else { writePinHigh(MUX_CS4); }
        #endif

        for (uint8_t ch = 0; ch < 32; ch++) {

            select_rmux_channel(ch);
            wait_us(10);  // Increased delay without filter caps to allow signal settling

            uint16_t adc_val = analogReadPin(radc_pins[mux_idx]);

            // Filter out anomalous high values (crosstalk without filter caps)
            // Valid Hall sensor range: 0 (pressed) to ~512 (released)
            // Values above 800 are likely crosstalk
            const uint16_t ADC_MAX_VALID = 800;
            if (adc_val > ADC_MAX_VALID) {
                adc_val = 4095;  // Treat as invalid/unpressed
            }

            const mux32_ref_t* key_mapping = &rmux_tables[mux_idx][ch];

            if (!key_mapping) continue;

            // If channel is unmapped and reads near GND, skip
            if (key_mapping->sensor == 0) {
                if (adc_val <= ADC_GND_THRESHOLD) continue;
                else continue;
            }

            uint16_t sensor = key_mapping->sensor;
            if (sensor == 0) continue;

            // Map sensor to matrix position
            uint8_t matrix_row = (sensor - 1) / MATRIX_COLS;
            uint8_t matrix_col = (sensor - 1) % MATRIX_COLS;

            if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;

            //uint16_t key_idx = (matrix_row * MATRIX_COLS) + matrix_col;
            uint16_t key_idx = (sensor - 1);
            if (key_idx >= MAX_KEYS) continue;


            // Use dynamic per-key sensitivity if calibration is complete
            bool should_press = false;
            if (calibration_complete) {
                uint16_t base = key_baseline[key_idx] ? key_baseline[key_idx] : 512;
                //uint8_t sens = key_sensitivity_percent[key_idx] ? key_sensitivity_percent[key_idx] : 10; // percent
                uint8_t sens = key_sensitivity_percent[key_idx] ? key_sensitivity_percent[key_idx] : 20; // value where it should press range from 1 - 180

                // Compute lower and upper bounds based on percent deviation
                // uint32_t lower = ((uint32_t)base * (100 - sens)) / 100;
                // uint32_t upper = ((uint32_t)base * (100 + sens)) / 100;
                int16_t lower = base - adc_val;
                int16_t upper = -lower;
                if (lower < 1) lower = 0;
                if (upper > 1) upper = 0;

                int16_t key_direction = adc_val - adc_values[key_idx];

                // Press if value deviates below lower OR above upper
                should_press = (lower > sens) || (upper > sens);
                //Rapid trigger (at least my take on it)
                if(rapid_trigger){
                    bool going_down = (key_direction <= 2);
                    bool going_up = (key_direction >= 2);
                    if(going_down)last_state = true;
                    else if (going_up)last_state = false;
                    should_press &= last_state;
                }
            } else {
                // Fallback to legacy absolute threshold
                uint16_t threshold = key_threshold[key_idx] ? key_threshold[key_idx] : SENSOR_THRESHOLD;
                should_press = (adc_val < threshold);
            }

            // Store ADC value for debug display (show filtered value)
            if (key_idx < KEY_COUNT) {
                adc_values[key_idx] = adc_val;
            }

            // Debounce: only change state if debounce time elapsed
            if (timer_elapsed32(key_timer[key_idx]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx]) {
                    key_pressed[key_idx] = should_press;
                    key_timer[key_idx] = now;
                    changed = true;
                }
            }

            if (key_pressed[key_idx]) {
                current_matrix[matrix_row] |= (1 << matrix_col);
            }
        }

        // Release CS for this MUX (keep others disabled)

            #ifdef MUX_CS3
                writePinHigh(MUX_CS3);
            #endif
            #ifdef MUX_CS4
                writePinHigh(MUX_CS4);
            #endif

    }
    return changed;
}


bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = is_keyboard_left()? left_side_scan(current_matrix) : right_side_scan(current_matrix);
    uint32_t now = timer_read32();

    if(debug_mode && timer_elapsed32(last_adc_print_time) > ADC_PRINT_INTERVAL_MS){
        last_adc_print_time = now;
        for (uint8_t i = 0; i<MAX_KEYS; i++) {

            if(i % MATRIX_COLS == 0){
                uprint("\n");
            }
            int16_t diff = key_baseline[i] - adc_values[i];
            if(diff >= 512){continue;}
            uprintf("|%02u,%3u,%3d| ", i, adc_values[i], diff);
        }
        uprint("\n");
        uprint("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    }

    return changed;
}






/*old matrix scan fonction

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool changed = false;
    uint32_t now = timer_read32();

    // Throttle matrix scan to ~667Hz (1.5ms between scans) - faster for better response
    static uint32_t last_scan = 0;
    if (timer_elapsed32(last_scan) < 2) {  // QMK timer is 1ms resolution, so use 2ms
        return false;
    }
    last_scan = now;

    // Clear matrix output
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        current_matrix[row] = 0;
    }

    // Clear ADC values array
    for (uint8_t i = 0; i < MAX_KEYS; i++) {
        adc_values[i] = 0;
    }

    // ADC pins and mapping table
        pin_t ladc_pins[2] = {MUX1_ADC, MUX2_ADC};
        const mux32_ref_t* lmux_tables[2] = {mux1_channels, mux2_channels};
        pin_t radc_pins[2] = {MUX3_ADC, MUX4_ADC};
        const mux32_ref_t* rmux_tables[2] = {mux3_channels, mux4_channels};


    const uint16_t ADC_GND_THRESHOLD = 100; // skip obviously unconnected channels

    // Scan each mux and channel
    for (uint8_t mux_idx = 0; mux_idx < 2; mux_idx++) {
        // Select appropriate CS
        if(is_keyboard_left()){
            #ifdef MUX_CS1
            if (mux_idx == 0) { writePinLow(MUX_CS1); } else { writePinHigh(MUX_CS1); }
            #endif
            #ifdef MUX_CS2
            if (mux_idx == 1) { writePinLow(MUX_CS2); } else { writePinHigh(MUX_CS2); }
            #endif
        }else{
            #ifdef MUX_CS3
                    if (mux_idx == 0) { writePinLow(MUX_CS3); } else { writePinHigh(MUX_CS3); }
            #endif
            #ifdef MUX_CS4
                    if (mux_idx == 1) { writePinLow(MUX_CS4); } else { writePinHigh(MUX_CS4); }
            #endif
        }

        for (uint8_t ch = 0; ch < 32; ch++) {

            if(is_keyboard_left()){
                    select_lmux_channel(ch);
            }
            else{
                    select_rmux_channel(ch);
            }
            wait_us(100);  // Increased delay without filter caps to allow signal settling

            uint16_t adc_val = analogReadPin(is_keyboard_left()? ladc_pins[mux_idx] : radc_pins[mux_idx]);

            // Filter out anomalous high values (crosstalk without filter caps)
            // Valid Hall sensor range: 0 (pressed) to ~512 (released)
            // Values above 800 are likely crosstalk
            const uint16_t ADC_MAX_VALID = 800;
            if (adc_val > ADC_MAX_VALID) {
                adc_val = 4095;  // Treat as invalid/unpressed
            }

            const mux32_ref_t* key_mapping = is_keyboard_left()? &lmux_tables[mux_idx][ch] : &rmux_tables[mux_idx][ch];

            if (!key_mapping) continue;

            // If channel is unmapped and reads near GND, skip
            if (key_mapping->sensor == 0) {
                if (adc_val <= ADC_GND_THRESHOLD) continue;
                else continue;
            }

            uint16_t sensor = key_mapping->sensor;
            if (sensor == 0) continue;

            // Map sensor to matrix position
            uint8_t matrix_row = (sensor - 1) / MATRIX_COLS;
            uint8_t matrix_col = (sensor - 1) % MATRIX_COLS;

            if (matrix_row >= MATRIX_ROWS || matrix_col >= MATRIX_COLS) continue;

            //uint16_t key_idx = (matrix_row * MATRIX_COLS) + matrix_col;
            uint16_t key_idx = (sensor - 1);
            if (key_idx >= MAX_KEYS) continue;

            // Store ADC value for debug display (show filtered value)
            if (key_idx < KEY_COUNT) {
                adc_values[key_idx] = adc_val;
            }

            // Use dynamic per-key sensitivity if calibration is complete
            bool should_press = false;
            if (calibration_complete) {
                uint16_t base = key_baseline[key_idx] ? key_baseline[key_idx] : 512;
                uint8_t sens = key_sensitivity_percent[key_idx] ? key_sensitivity_percent[key_idx] : 10; // percent

                // Compute lower and upper bounds based on percent deviation
                uint32_t lower = ((uint32_t)base * (100 - sens)) / 100;
                uint32_t upper = ((uint32_t)base * (100 + sens)) / 100;

                // Safety clamps
                if (lower < 1) lower = 1;
                if (upper > 4095) upper = 4095;

                // Press if value deviates below lower OR above upper
                should_press = (adc_val < lower) || (adc_val > upper);
            } else {
                // Fallback to legacy absolute threshold
                uint16_t threshold = key_threshold[key_idx] ? key_threshold[key_idx] : SENSOR_THRESHOLD;
                should_press = (adc_val < threshold);
            }

            // Debounce: only change state if debounce time elapsed
            if (timer_elapsed32(key_timer[key_idx]) > DEBOUNCE_MS) {
                if (should_press != key_pressed[key_idx]) {
                    key_pressed[key_idx] = should_press;
                    key_timer[key_idx] = now;
                    changed = true;
                }
            }

            if (key_pressed[key_idx]) {
                current_matrix[matrix_row] |= (1 << matrix_col);
            }
        }

        // Release CS for this MUX (keep others disabled)
            if(is_keyboard_left()){
                #ifdef MUX_CS1
                    writePinHigh(MUX_CS1);
                #endif
                #ifdef MUX_CS2
                    writePinHigh(MUX_CS2);
                #endif
            }else{
                #ifdef MUX_CS3
                    writePinHigh(MUX_CS3);
                #endif
                #ifdef MUX_CS4
                    writePinHigh(MUX_CS4);
                #endif
            }
    }


    if(debug_mode && timer_elapsed32(last_adc_print_time) > ADC_PRINT_INTERVAL_MS){
        last_adc_print_time = now;
        for (uint8_t i = 0; i<MAX_KEYS; i++) {

            if(i % MATRIX_COLS == 0){
                uprint("\n");
            }

            uprintf("|%02u,%3u,%b| ", i, adc_values[i], key_pressed[i]);
        }
        uprint("\n");
        uprint("-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    }

    return changed;
}

*/
