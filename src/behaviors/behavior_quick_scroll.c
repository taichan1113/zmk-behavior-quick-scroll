/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT modula_behavior_quick_scroll

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <dt-bindings/zmk/pointing.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define QUICK_SCROLL_MAX_HELD 8

struct vector2d {
    float x;
    float y;
};

struct movement_state_1d {
    float remainder;
    int16_t speed;
    int64_t start_time;
};

struct movement_state_2d {
    struct movement_state_1d x;
    struct movement_state_1d y;
};

struct held_binding {
    bool active;
    uint32_t position;
    int16_t x;
    int16_t y;
};

struct behavior_quick_scroll_data {
    struct k_work_delayable tick_work;
    const struct device *dev;
    struct movement_state_2d state;
    struct held_binding held[QUICK_SCROLL_MAX_HELD];
    int64_t last_release_ms;
    uint32_t last_param;
};

struct behavior_quick_scroll_config {
    int16_t x_code;
    int16_t y_code;
    uint16_t delay_ms;
    uint16_t time_to_max_speed_ms;
    uint8_t trigger_period_ms;
    uint8_t acceleration_exponent;
    uint16_t double_tap_ms;
    uint8_t fast_multiplier;
    uint16_t tap_step_ms;
};

#if CONFIG_MINIMAL_LIBC
static float powf(float base, float exponent) {
    float power = 1.0f;
    for (; exponent >= 1.0f; exponent--) {
        power *= base;
    }
    return power;
}
#else
#include <math.h>
#endif

static int64_t ticks_since_start(int64_t start, int64_t now, int64_t delay) {
    if (start == 0) {
        return 0;
    }

    int64_t move_duration = now - (start + delay);
    return MAX(move_duration, 0);
}

static float speed(const struct behavior_quick_scroll_config *config, float max_speed,
                   int64_t duration_ticks) {
    if ((1000 * duration_ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC) > config->time_to_max_speed_ms ||
        config->time_to_max_speed_ms == 0 || config->acceleration_exponent == 0) {
        return max_speed;
    }

    if (duration_ticks == 0) {
        return 0;
    }

    float time_fraction = (float)(1000 * duration_ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC) /
                          config->time_to_max_speed_ms;
    return max_speed * powf(time_fraction, config->acceleration_exponent);
}

static void track_remainder(float *move, float *remainder) {
    float new_move = *move + *remainder;
    *remainder = new_move - (int)new_move;
    *move = (int)new_move;
}

static float update_movement_1d(const struct behavior_quick_scroll_config *config,
                                struct movement_state_1d *state, int64_t now) {
    if (state->speed == 0) {
        state->remainder = 0;
        return 0;
    }

    int64_t move_duration = ticks_since_start(state->start_time, now, config->delay_ms);
    float move = (move_duration > 0)
                     ? (speed(config, state->speed, move_duration) * config->trigger_period_ms /
                        1000)
                     : 0;

    track_remainder(&move, &state->remainder);
    return move;
}

static struct vector2d update_movement_2d(const struct behavior_quick_scroll_config *config,
                                          struct movement_state_2d *state, int64_t now) {
    return (struct vector2d){
        .x = update_movement_1d(config, &state->x, now),
        .y = update_movement_1d(config, &state->y, now),
    };
}

static bool should_be_working(struct behavior_quick_scroll_data *data) {
    return data->state.x.speed != 0 || data->state.y.speed != 0;
}

static void report_move(const struct device *dev, float x, float y) {
    const struct behavior_quick_scroll_config *cfg = dev->config;
    bool have_x = x != 0;
    bool have_y = y != 0;

    if (have_x) {
        int err = input_report_rel(dev, cfg->x_code, (int16_t)CLAMP(x, INT16_MIN, INT16_MAX),
                                   !have_y, K_NO_WAIT);
        if (err < 0) {
            LOG_WRN("Failed to report quick scroll x input: %d", err);
        }
    }
    if (have_y) {
        int err = input_report_rel(dev, cfg->y_code, (int16_t)CLAMP(y, INT16_MIN, INT16_MAX), true,
                                   K_NO_WAIT);
        if (err < 0) {
            LOG_WRN("Failed to report quick scroll y input: %d", err);
        }
    }
}

static void tick_work_cb(struct k_work *work) {
    struct k_work_delayable *d_work = k_work_delayable_from_work(work);
    struct behavior_quick_scroll_data *data =
        CONTAINER_OF(d_work, struct behavior_quick_scroll_data, tick_work);
    const struct device *dev = data->dev;
    const struct behavior_quick_scroll_config *cfg = dev->config;

    struct vector2d move = update_movement_2d(cfg, &data->state, k_uptime_ticks());
    report_move(dev, move.x, move.y);

    if (should_be_working(data)) {
        k_work_schedule(&data->tick_work, K_MSEC(cfg->trigger_period_ms));
    }
}

static void set_start_time_for_activity(struct movement_state_1d *state) {
    if (state->speed != 0 && state->start_time == 0) {
        state->start_time = k_uptime_ticks();
    } else if (state->speed == 0) {
        state->start_time = 0;
    }
}

static void update_work_scheduling(const struct device *dev) {
    struct behavior_quick_scroll_data *data = dev->data;
    const struct behavior_quick_scroll_config *cfg = dev->config;

    set_start_time_for_activity(&data->state.x);
    set_start_time_for_activity(&data->state.y);

    if (should_be_working(data)) {
        k_work_schedule(&data->tick_work, K_MSEC(cfg->trigger_period_ms));
    } else {
        k_work_cancel_delayable(&data->tick_work);
        data->state.x.remainder = 0;
        data->state.y.remainder = 0;
    }
}

static int adjust_speed(const struct device *dev, int16_t dx, int16_t dy) {
    struct behavior_quick_scroll_data *data = dev->data;

    data->state.x.speed += dx;
    data->state.y.speed += dy;
    update_work_scheduling(dev);

    return 0;
}

static void send_immediate_step(const struct device *dev, int16_t x, int16_t y) {
    const struct behavior_quick_scroll_config *cfg = dev->config;
    float move_x = (float)x * cfg->tap_step_ms / 1000;
    float move_y = (float)y * cfg->tap_step_ms / 1000;

    report_move(dev, move_x, move_y);
}

static struct held_binding *find_held(struct behavior_quick_scroll_data *data, uint32_t position) {
    for (int i = 0; i < QUICK_SCROLL_MAX_HELD; i++) {
        if (data->held[i].active && data->held[i].position == position) {
            return &data->held[i];
        }
    }

    return NULL;
}

static struct held_binding *alloc_held(struct behavior_quick_scroll_data *data) {
    for (int i = 0; i < QUICK_SCROLL_MAX_HELD; i++) {
        if (!data->held[i].active) {
            return &data->held[i];
        }
    }

    return NULL;
}

static bool is_fast_press(struct behavior_quick_scroll_data *data,
                          const struct behavior_quick_scroll_config *cfg, uint32_t param) {
    int64_t now = k_uptime_get();

    return data->last_release_ms > 0 && param == data->last_param &&
           (now - data->last_release_ms) <= cfg->double_tap_ms;
}

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct behavior_quick_scroll_config *cfg = dev->config;
    struct behavior_quick_scroll_data *data = dev->data;
    struct held_binding *held = alloc_held(data);

    if (held == NULL) {
        LOG_WRN("No free quick-scroll held binding slot");
        return -ENOMEM;
    }

    int16_t x = MOVE_X_DECODE(binding->param1);
    int16_t y = MOVE_Y_DECODE(binding->param1);
    uint8_t multiplier = is_fast_press(data, cfg, binding->param1) ? cfg->fast_multiplier : 1;

    x *= multiplier;
    y *= multiplier;

    *held = (struct held_binding){
        .active = true,
        .position = event.position,
        .x = x,
        .y = y,
    };

    send_immediate_step(dev, x, y);
    adjust_speed(dev, x, y);

    return ZMK_BEHAVIOR_OPAQUE;
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct behavior_quick_scroll_data *data = dev->data;
    struct held_binding *held = find_held(data, event.position);

    if (held == NULL) {
        return 0;
    }

    adjust_speed(dev, -held->x, -held->y);

    held->active = false;
    data->last_release_ms = k_uptime_get();
    data->last_param = binding->param1;

    return ZMK_BEHAVIOR_OPAQUE;
}

static int behavior_quick_scroll_init(const struct device *dev) {
    struct behavior_quick_scroll_data *data = dev->data;

    data->dev = dev;
    k_work_init_delayable(&data->tick_work, tick_work_cb);

    return 0;
}

static const struct behavior_driver_api behavior_quick_scroll_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define QUICK_SCROLL_INST(n)                                                                      \
    static struct behavior_quick_scroll_data behavior_quick_scroll_data_##n = {};                 \
    static struct behavior_quick_scroll_config behavior_quick_scroll_config_##n = {               \
        .x_code = DT_INST_PROP(n, x_input_code),                                                  \
        .y_code = DT_INST_PROP(n, y_input_code),                                                  \
        .trigger_period_ms = DT_INST_PROP(n, trigger_period_ms),                                  \
        .delay_ms = DT_INST_PROP(n, delay_ms),                                                    \
        .time_to_max_speed_ms = DT_INST_PROP(n, time_to_max_speed_ms),                            \
        .acceleration_exponent = DT_INST_PROP(n, acceleration_exponent),                          \
        .double_tap_ms = DT_INST_PROP(n, double_tap_ms),                                          \
        .fast_multiplier = DT_INST_PROP(n, fast_multiplier),                                      \
        .tap_step_ms = DT_INST_PROP(n, tap_step_ms),                                              \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(n, behavior_quick_scroll_init, NULL,                                  \
                            &behavior_quick_scroll_data_##n,                                      \
                            &behavior_quick_scroll_config_##n, POST_KERNEL,                       \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                  \
                            &behavior_quick_scroll_driver_api);

DT_INST_FOREACH_STATUS_OKAY(QUICK_SCROLL_INST)
