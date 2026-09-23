#include QMK_KEYBOARD_H
#include "mux_config.h"
#include "matrix.h"
#include <stdbool.h>

// Public API for the hallscan module
// Initialize hardware and run calibration
void matrix_init_custom(void);

// Perform a single scan and update the provided matrix rows
bool matrix_scan_custom(matrix_row_t current_matrix[]);


// Trigger an explicit calibration (recomputes baselines and thresholds)
//void hallscan_calibrate(void);
void calibrate_sensors(void);

// Accessors for per-sensor data (keep storage private to hallscan.c)
//uint16_t hallscan_get_baseline(mux32_ref_t id);
//uint16_t hallscan_get_threshold(mux32_ref_t id);
