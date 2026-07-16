# ZMK Quick Scroll Behavior

Immediate key-driven scroll behavior for ZMK.

## Usage

Include the behavior definition in your keymap:

```dts
#include <behaviors/quick_scroll.dtsi>
```

Then bind scroll keys with standard ZMK pointing movement values:

```dts
&qmsc MOVE_Y(120)
&qmsc MOVE_Y(-120)
```

## Devicetree Properties

- `double-tap-ms`: release-to-next-press window for fast hold.
- `fast-multiplier`: multiplier used for the second press and hold.
- `tap-step-ms`: immediate movement amount expressed as speed over this many milliseconds.
- `trigger-period-ms`: repeat interval while held.
- `time-to-max-speed-ms`: acceleration ramp duration when acceleration is enabled.
- `acceleration-exponent`: `0` disables acceleration.
