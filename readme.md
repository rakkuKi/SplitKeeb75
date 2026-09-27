# splitkeeb75

![splitkeeb75](imgur.com image replace me!)

*My first keyboard it's just a plain halleffect split keyboard that I'm using as an excuse to actively trying to learn soldering and learn a bit of C.*
I'll try my best to implement some features like rapid trigger but I won't promise it'll be as good as wootings and other brands.

* Keyboard Maintainer: [rakkuKi](https://github.com/rakkuKi)
* Hardware Supported: *rp2040zero, 49E hall effect sensor and ADG732SUZ mux*
* Hardware Availability: *all on aliexpress and amazon I kinda don't have the specific links*

the things below are all TO DO later if I don't forgot them

Make example for this keyboard (after setting up your build environment):

    make splitkeeb75:default

Flashing example for this keyboard:

    make splitkeeb75:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (usually the top left key or Escape) and plug in the keyboard
* **Physical reset button**: Briefly press the button on the back of the PCB - some may have pads you must short instead
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available
