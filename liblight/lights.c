/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "lights"
#include <cutils/log.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <hardware/lights.h>

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_backlight = 255;

static unsigned int lcd_type, firmwire_ver;
static float scale_low = 1.0f;

static struct __scale_map {
    unsigned int id;
    float scale;
} onewire_map[] = {
    { 24, 0.5f },
    { 28, 2.0f },
};

static void init_onewire_backlight(void)
{
    char info[256];
    int fd;

    fd = open("/proc/driver/one-wire-info", O_RDONLY);
    if (fd < 0)
        return;

    memset(info, 0, sizeof(info));

    int ret = read(fd, info, 255);
    close(fd);

    if (ret <= 0)
        return;

    unsigned int i = 0;
    sscanf(info, "%u%u", &lcd_type, &firmwire_ver);

    for (; i < sizeof(onewire_map)/sizeof(onewire_map[0]); i++) {
        if (onewire_map[i].id == lcd_type) {
            scale_low = onewire_map[i].scale;
            break;
        }
    }

    ALOGV("onewire backlight: LCD %2d (param. %.1f)\n",
        lcd_type, scale_low);
}

static int set_onewire_backlight(int val)  /* 20 - 255 */
{
    int bright = val;
    int fd;

    if (lcd_type == 0)
        return 0;

#define ANDROID_DIM     10
#define LIGHT_MAX       255

    if (bright <= 0) {
        bright = 0;

    } else if (bright <= ANDROID_DIM) {
        bright*= scale_low;

    } else if (bright > LIGHT_MAX) {
        bright = 127;

    } else {
        // (11~255) --> (15~127)
        bright -= ANDROID_DIM;

        int min_v = ANDROID_DIM * scale_low;
        int max_v = 127;
        if (bright > 0) {
            bright = (127.0 - min_v) / (LIGHT_MAX - ANDROID_DIM - 1) * bright
                + 0.5 + min_v;
        }
    }

    ALOGV("set_onewire_backlight %3d, HW: %d", val, bright);

#define ONE_WIRE_DEV "/dev/backlight-1wire"
    fd = open(ONE_WIRE_DEV, O_RDWR);
    if (fd >= 0) {
        char buffer[20];

        int bytes = sprintf(buffer, "%d\n", bright);
        int ret = write(fd, buffer, bytes);
        close(fd);

        return ret == -1 ? -errno : 0;
    } else {
        return -errno;
    }
}

static int rgb_to_brightness(struct light_state_t const *state)
{
    int color = state->color & 0x00ffffff;

    return ((77*((color>>16) & 0x00ff))
        + (150*((color>>8) & 0x00ff)) + (29*(color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t *dev,
            struct light_state_t const *state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);

    pthread_mutex_lock(&g_lock);
    g_backlight = brightness;
    err = set_onewire_backlight(brightness);

    pthread_mutex_unlock(&g_lock);
    return err;
}

static int close_lights(struct light_device_t *dev)
{
    if (dev)
        free(dev);

    return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
                       struct hw_device_t **device)
{
    int (*set_light)(struct light_device_t *dev,
            struct light_state_t const *state);

    if (!strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else
        return -EINVAL;

    init_onewire_backlight();

    pthread_mutex_init(&g_lock, NULL);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t *)module;
    dev->common.close = (int (*)(struct hw_device_t *))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t *)dev;

    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "lights Module",
    .author = "Google, Inc.",
    .methods = &lights_module_methods,
};
