#include "Bluepad32.h"

#include <string.h>

#include "ESPectrum.h"
#include "fabgl.h"

extern "C" {
#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <uni.h>
}

namespace {

static uint8_t previousModifiers = 0;
static uint8_t previousKeys[UNI_KEYBOARD_PRESSED_KEYS_MAX] = {0};

static bool keyPresent(const uint8_t* keys, uint8_t usage) {
    for (int i = 0; i < UNI_KEYBOARD_PRESSED_KEYS_MAX; ++i)
        if (keys[i] == usage)
            return true;
    return false;
}

static fabgl::VirtualKey usageToVirtualKey(uint8_t u) {
    if (u >= 0x04 && u <= 0x1d)
        return (fabgl::VirtualKey)(fabgl::VK_A + (u - 0x04));
    if (u >= 0x1e && u <= 0x26)
        return (fabgl::VirtualKey)(fabgl::VK_1 + (u - 0x1e));
    if (u == 0x27) return fabgl::VK_0;

    switch (u) {
        case 0x28: return fabgl::VK_RETURN;
        case 0x29: return fabgl::VK_ESCAPE;
        case 0x2a: return fabgl::VK_BACKSPACE;
        case 0x2b: return fabgl::VK_TAB;
        case 0x2c: return fabgl::VK_SPACE;
        case 0x2d: return fabgl::VK_MINUS;
        case 0x2e: return fabgl::VK_EQUALS;
        case 0x2f: return fabgl::VK_LEFTBRACKET;
        case 0x30: return fabgl::VK_RIGHTBRACKET;
        case 0x31: return fabgl::VK_BACKSLASH;
        case 0x32: return fabgl::VK_HASH;
        case 0x33: return fabgl::VK_SEMICOLON;
        case 0x34: return fabgl::VK_QUOTE;
        case 0x35: return fabgl::VK_GRAVEACCENT;
        case 0x36: return fabgl::VK_COMMA;
        case 0x37: return fabgl::VK_PERIOD;
        case 0x38: return fabgl::VK_SLASH;
        case 0x39: return fabgl::VK_CAPSLOCK;

        case 0x3a: return fabgl::VK_F1;
        case 0x3b: return fabgl::VK_F2;
        case 0x3c: return fabgl::VK_F3;
        case 0x3d: return fabgl::VK_F4;
        case 0x3e: return fabgl::VK_F5;
        case 0x3f: return fabgl::VK_F6;
        case 0x40: return fabgl::VK_F7;
        case 0x41: return fabgl::VK_F8;
        case 0x42: return fabgl::VK_F9;
        case 0x43: return fabgl::VK_F10;
        case 0x44: return fabgl::VK_F11;
        case 0x45: return fabgl::VK_F12;

        case 0x46: return fabgl::VK_PRINTSCREEN;
        case 0x47: return fabgl::VK_SCROLLLOCK;
        case 0x48: return fabgl::VK_PAUSE;
        case 0x49: return fabgl::VK_INSERT;
        case 0x4a: return fabgl::VK_HOME;
        case 0x4b: return fabgl::VK_PAGEUP;
        case 0x4c: return fabgl::VK_DELETE;
        case 0x4d: return fabgl::VK_END;
        case 0x4e: return fabgl::VK_PAGEDOWN;
        case 0x4f: return fabgl::VK_RIGHT;
        case 0x50: return fabgl::VK_LEFT;
        case 0x51: return fabgl::VK_DOWN;
        case 0x52: return fabgl::VK_UP;

        case 0x53: return fabgl::VK_NUMLOCK;
        case 0x54: return fabgl::VK_KP_DIVIDE;
        case 0x55: return fabgl::VK_KP_MULTIPLY;
        case 0x56: return fabgl::VK_KP_MINUS;
        case 0x57: return fabgl::VK_KP_PLUS;
        case 0x58: return fabgl::VK_KP_ENTER;
        case 0x59: return fabgl::VK_KP_1;
        case 0x5a: return fabgl::VK_KP_2;
        case 0x5b: return fabgl::VK_KP_3;
        case 0x5c: return fabgl::VK_KP_4;
        case 0x5d: return fabgl::VK_KP_5;
        case 0x5e: return fabgl::VK_KP_6;
        case 0x5f: return fabgl::VK_KP_7;
        case 0x60: return fabgl::VK_KP_8;
        case 0x61: return fabgl::VK_KP_9;
        case 0x62: return fabgl::VK_KP_0;
        case 0x63: return fabgl::VK_KP_PERIOD;

        case 0x65: return fabgl::VK_APPLICATION;
        default: return fabgl::VK_NONE;
    }
}

static void emitKey(uint8_t usage, bool down) {
    fabgl::VirtualKey vk = usageToVirtualKey(usage);
    if (vk != fabgl::VK_NONE)
        ESPectrum::PS2Controller.keyboard()->injectVirtualKey(vk, down, false);
}

static void emitModifier(uint8_t bit, fabgl::VirtualKey vk, bool now) {
    bool old = (previousModifiers & bit) != 0;
    if (old != now)
        ESPectrum::PS2Controller.keyboard()->injectVirtualKey(vk, now, false);
}

static void processKeyboard(const uni_keyboard_t* kb) {
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_CONTROL, fabgl::VK_LCTRL,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_LEFT_CONTROL) != 0);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_CONTROL, fabgl::VK_RCTRL,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_RIGHT_CONTROL) != 0);
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_SHIFT, fabgl::VK_LSHIFT,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_LEFT_SHIFT) != 0);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_SHIFT, fabgl::VK_RSHIFT,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_RIGHT_SHIFT) != 0);
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_ALT, fabgl::VK_LALT,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_LEFT_ALT) != 0);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_ALT, fabgl::VK_RALT,
                 (kb->modifiers & UNI_KEYBOARD_MODIFIER_RIGHT_ALT) != 0);

    for (int i = 0; i < UNI_KEYBOARD_PRESSED_KEYS_MAX; ++i) {
        uint8_t oldKey = previousKeys[i];
        if (oldKey && !keyPresent(kb->pressed_keys, oldKey))
            emitKey(oldKey, false);
    }

    for (int i = 0; i < UNI_KEYBOARD_PRESSED_KEYS_MAX; ++i) {
        uint8_t newKey = kb->pressed_keys[i];
        if (newKey && !keyPresent(previousKeys, newKey))
            emitKey(newKey, true);
    }

    previousModifiers = kb->modifiers;
    memcpy(previousKeys, kb->pressed_keys, sizeof(previousKeys));
}

static void bp32_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    (void)d;
    if (ctl->klass == UNI_CONTROLLER_CLASS_KEYBOARD)
        processKeyboard(&ctl->keyboard);
}

static void releaseKeyboardState() {
    for (int i = 0; i < UNI_KEYBOARD_PRESSED_KEYS_MAX; ++i) {
        if (previousKeys[i])
            emitKey(previousKeys[i], false);
        previousKeys[i] = 0;
    }

    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_CONTROL, fabgl::VK_LCTRL, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_CONTROL, fabgl::VK_RCTRL, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_SHIFT, fabgl::VK_LSHIFT, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_SHIFT, fabgl::VK_RSHIFT, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_ALT, fabgl::VK_LALT, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_ALT, fabgl::VK_RALT, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_LEFT_GUI, fabgl::VK_LGUI, false);
    emitModifier(UNI_KEYBOARD_MODIFIER_RIGHT_GUI, fabgl::VK_RGUI, false);

    previousModifiers = 0;
}

static void bp32_on_init_complete(void) {
    logi("[BLUEPAD32] Bluetooth ready; scanning enabled\n");
    uni_bt_enable_new_connections_unsafe(true);
}

static void bp32_init(int argc, const char** argv) {
    (void)argc;
    (void)argv;
    logi("[BLUEPAD32] init\n");
}

static void bp32_task(void*) {
    btstack_init();

    static struct uni_platform platform = {};
    platform.name = "ESPectrum";
    platform.init = bp32_init;
    platform.on_init_complete = bp32_on_init_complete;
    platform.on_device_disconnected = [](uni_hid_device_t* d) {
        (void)d;
        releaseKeyboardState();
        logi("[BLUEPAD32] Keyboard/controller disconnected\n");
    };
    platform.on_controller_data = bp32_on_controller_data;

    uni_platform_set_custom(&platform);
    uni_init(0, nullptr);
    btstack_run_loop_execute();
    vTaskDelete(nullptr);
}

}  // namespace

namespace Bluepad32Input {

void setup() {
    printf("[BLUEPAD32] Starting Bluetooth task\n");
    xTaskCreatePinnedToCore(bp32_task, "bluepad32", 8192, nullptr, 5, nullptr, 0);
}

}  // namespace Bluepad32Input
