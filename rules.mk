UART_DRIVER_REQUIRED = yes
ANALOG_DRIVER_REQUIRED = yes
ANALOG_DRIVER = rp2040_adc
SPLIT_KEYBOARD = yes
SERIAL_DRIVER = vendor
UART_DRIVER = vendor
CONSOLE_ENABLE = yes
CUSTOM_MATRIX = lite
SRC += hallscan.c

RAW_ENABLE = yes

# Optimizations
#LTO_ENABLE = yes

# Enable VIA and persistent dynamic keymaps so VIA can save layout changes to EEPROM
DYNAMIC_KEYMAP_ENABLE = yes