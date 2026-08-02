# zmk-behavior-quick-scroll

[English](#english) | [日本語](#japanese)

<a id="english"></a>

A small ZMK module that provides immediate key-driven scrolling with a fast double-tap hold mode.
It is intended for scroll keys such as `&msc MOVE_Y(120)`, where the input is a key press/release rather than a continuous pointing device event.

The module provides:

- `modula,behavior-quick-scroll`: a keymap behavior that emits relative wheel input immediately on press.
- `&qmsc`: a default behavior label for vertical and horizontal wheel scrolling.
- A matching `zmk,input-listener` node so the generated input events are sent through ZMK input processing.

## Installation

Add this module to your ZMK user config `config/west.yml`.

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: BP
      url-base: https://github.com/taichan1113

  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3
      import: app/west.yml

    - name: zmk-behavior-quick-scroll
      remote: BP
      revision: main

  self:
    path: config
```

Then update your workspace (if you use a zmk-workspace style setup):

```sh
west update zmk-behavior-quick-scroll
```

## Keymap Usage

Include the default behavior definition in your keymap:

```dts
#include <behaviors/quick_scroll.dtsi>
```

Then use `&qmsc` with standard ZMK pointing movement values:

```dts
bindings = <
    &qmsc MOVE_Y(120)
    &qmsc MOVE_Y(-120)
>;
```

The behavior starts scrolling immediately on press. A short tap emits one fixed scroll step. Holding continues scrolling at the normal speed. Pressing again within `double-tap-ms` and holding scrolls at the fast speed.

## Behavior Customization

The included `quick_scroll.dtsi` defines `&qmsc` with defaults:

```dts
&qmsc {
    trigger-period-ms = <16>;
    time-to-max-speed-ms = <300>;
    acceleration-exponent = <0>;
    double-tap-ms = <200>;
    fast-multiplier = <8>;
    tap-step-ms = <80>;
};
```

Override only the properties you want to change in your `.keymap`:

```dts
&qmsc {
    double-tap-ms = <100>;
    fast-multiplier = <4>;
    tap-step-ms = <50>;
};
```

## Devicetree Usage

If you do not want to use the default `&qmsc` node, define your own behavior in your `.keymap` or overlay.

```dts
#include <dt-bindings/zmk/pointing.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    behaviors {
        qmsc: quick_mouse_scroll {
            compatible = "modula,behavior-quick-scroll";
            #binding-cells = <1>;
            x-input-code = <INPUT_REL_HWHEEL>;
            y-input-code = <INPUT_REL_WHEEL>;
            trigger-period-ms = <16>;
            time-to-max-speed-ms = <300>;
            acceleration-exponent = <0>;
            double-tap-ms = <200>;
            fast-multiplier = <8>;
            tap-step-ms = <80>;
        };
    };

    qmsc_input_listener: qmsc_input_listener {
        compatible = "zmk,input-listener";
        device = <&qmsc>;
    };
};
```

### Properties

- `x-input-code`: Required. Relative input code used when the bound value has an X component, usually `INPUT_REL_HWHEEL`.
- `y-input-code`: Required. Relative input code used when the bound value has a Y component, usually `INPUT_REL_WHEEL`.
- `trigger-period-ms`: Optional. Repeat interval while the key is held. Defaults to `16`.
- `delay-ms`: Optional. Initial hold delay before repeated events. Defaults to `0`.
- `time-to-max-speed-ms`: Optional. Acceleration ramp duration when acceleration is enabled. Defaults to `300`.
- `acceleration-exponent`: Optional. Acceleration curve exponent. Defaults to `0`, which disables acceleration.
- `double-tap-ms`: Optional. Maximum release-to-next-press interval for entering fast hold. Defaults to `200`.
- `fast-multiplier`: Optional. Speed multiplier used for the second press and hold. Defaults to `8`.
- `tap-step-ms`: Optional. Immediate tap movement amount, expressed as speed over this many milliseconds. Defaults to `80`.

## Example: Mouse Layer Scroll Keys

```dts
#include <behaviors/quick_scroll.dtsi>

&qmsc {
    double-tap-ms = <100>;
};

/ {
    keymap {
        compatible = "zmk,keymap";

        mouse_layer {
            bindings = <
                &qmsc MOVE_Y(120)
                &qmsc MOVE_Y(-120)
            >;
        };
    };
};
```

---

<a id="japanese"></a>

# zmk-behavior-quick-scroll

キー入力によるスクロールを、押した直後から発火できるようにする小さな ZMK モジュールです。
`&msc MOVE_Y(120)` のようなキー押下/リリースを対象にしたスクロール用途を想定しています。

このモジュールが提供するもの:

- `modula,behavior-quick-scroll`: 押下直後に relative wheel input を送る keymap behavior。
- `&qmsc`: 垂直/水平ホイールスクロール用のデフォルト behavior label。
- 生成した input event を ZMK input processing に流すための `zmk,input-listener` node。

## インストール

ZMK user config の `config/west.yml` にこのモジュールを追加します。

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: BP
      url-base: https://github.com/taichan1113

  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3
      import: app/west.yml

    - name: zmk-behavior-quick-scroll
      remote: BP
      revision: main

  self:
    path: config
```

zmk-workspace 形式で使っている場合は、workspace を更新します。

```sh
west update zmk-behavior-quick-scroll
```

## keymap での使い方

keymap でデフォルト behavior 定義を include します。

```dts
#include <behaviors/quick_scroll.dtsi>
```

その後、ZMK 標準の pointing movement value と一緒に `&qmsc` を使います。

```dts
bindings = <
    &qmsc MOVE_Y(120)
    &qmsc MOVE_Y(-120)
>;
```

この behavior は押した直後にスクロールを開始します。短押しでは固定量を 1 回スクロールします。押し続けると通常速度で連続スクロールします。`double-tap-ms` 以内にもう一度押してホールドすると高速スクロールになります。

## behavior のカスタマイズ

同梱の `quick_scroll.dtsi` は、以下のデフォルト値で `&qmsc` を定義します。

```dts
&qmsc {
    trigger-period-ms = <16>;
    time-to-max-speed-ms = <300>;
    acceleration-exponent = <0>;
    double-tap-ms = <200>;
    fast-multiplier = <8>;
    tap-step-ms = <80>;
};
```

変更したい property だけを `.keymap` 側で上書きします。

```dts
&qmsc {
    double-tap-ms = <100>;
    fast-multiplier = <4>;
    tap-step-ms = <50>;
};
```

## Devicetree での使い方

デフォルトの `&qmsc` node を使わない場合は、`.keymap` や overlay で独自の behavior を定義できます。

```dts
#include <dt-bindings/zmk/pointing.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

/ {
    behaviors {
        qmsc: quick_mouse_scroll {
            compatible = "modula,behavior-quick-scroll";
            #binding-cells = <1>;
            x-input-code = <INPUT_REL_HWHEEL>;
            y-input-code = <INPUT_REL_WHEEL>;
            trigger-period-ms = <16>;
            time-to-max-speed-ms = <300>;
            acceleration-exponent = <0>;
            double-tap-ms = <200>;
            fast-multiplier = <8>;
            tap-step-ms = <80>;
        };
    };

    qmsc_input_listener: qmsc_input_listener {
        compatible = "zmk,input-listener";
        device = <&qmsc>;
    };
};
```

### Properties

- `x-input-code`: 必須。X 成分を持つ binding value に使う relative input code。通常は `INPUT_REL_HWHEEL`。
- `y-input-code`: 必須。Y 成分を持つ binding value に使う relative input code。通常は `INPUT_REL_WHEEL`。
- `trigger-period-ms`: 任意。キーを押し続けたときの repeat 間隔。デフォルトは `16`。
- `delay-ms`: 任意。repeat 開始前の初期 delay。デフォルトは `0`。
- `time-to-max-speed-ms`: 任意。acceleration 有効時に最大速度へ到達するまでの時間。デフォルトは `300`。
- `acceleration-exponent`: 任意。acceleration curve の指数。デフォルトは `0` で、acceleration 無効。
- `double-tap-ms`: 任意。高速ホールドに入るための、リリースから次の押下までの最大間隔。デフォルトは `200`。
- `fast-multiplier`: 任意。2 回目押下後のホールドで使う速度倍率。デフォルトは `8`。
- `tap-step-ms`: 任意。押下直後に発火する移動量。指定速度でこの時間だけ動いた量として扱います。デフォルトは `80`。

## 例: Mouse Layer のスクロールキー

```dts
#include <behaviors/quick_scroll.dtsi>

&qmsc {
    double-tap-ms = <100>;
};

/ {
    keymap {
        compatible = "zmk,keymap";

        mouse_layer {
            bindings = <
                &qmsc MOVE_Y(120)
                &qmsc MOVE_Y(-120)
            >;
        };
    };
};
```
