#include "../settingsitem.h"

// ---- Translation markers (for lupdate) ----
// These strings are used in REGISTER_SETTINGS_CONTENT macros below.
// lupdate picks them up here since it doesn't expand macros.
static const char *_tr_content_strings[] = {
    QT_TRANSLATE_NOOP("SettingsDialog", "Audio"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Enable audio capture"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Capture system audio or a specific process"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Capture source"),
    QT_TRANSLATE_NOOP("SettingsDialog", "System audio"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Process audio"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Display"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Font settings"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Font size, color and style"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Opacity"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Adjust window transparency"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Translation"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Enable translation"),
    QT_TRANSLATE_NOOP("SettingsDialog", "API Key"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Enter your API key"),

    QT_TRANSLATE_NOOP("SettingsDialog", "Font"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Font family"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Font size"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Font color"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Sample text preview"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Window opacity"),
    QT_TRANSLATE_NOOP("SettingsDialog", "100%"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Target PID"),
    QT_TRANSLATE_NOOP("SettingsDialog", "Enter PID or pick"),
};

// ---- Content registrations ----

REGISTER_SETTINGS_CONTENT("Audio", "toggle",
    "Enable audio capture",
    "Capture system audio or a specific process",
    {"key", "audio.enable_capture"},
    {"checked", true})

REGISTER_SETTINGS_CONTENT("Audio", "choice",
    "Capture source", "",
    {"key", "audio.capture_source"},
    {"choices", QStringList{"System audio", "Process audio"}},
    {"current", "System audio"})

REGISTER_SETTINGS_CONTENT("Audio", "pid",
    "Target PID", "",
    {"key", "audio.target_pid"},
    {"placeholder", "Enter PID or pick"})

REGISTER_SETTINGS_CONTENT("Display", "navigate",
    "Font settings",
    "Font size, color and style",
    {"key", "display.font"},
    {"target", "font"})

REGISTER_SETTINGS_CONTENT("Display", "navigate",
    "Opacity",
    "Adjust window transparency",
    {"key", "display.opacity"},
    {"target", "opacity"})

REGISTER_SETTINGS_CONTENT("Translation", "toggle",
    "Enable translation", "",
    {"key", "translation.enable_translation"},
    {"checked", false})

REGISTER_SETTINGS_CONTENT("Translation", "input",
    "API Key", "",
    {"key", "translation.api_key"},
    {"placeholder", "Enter your API key"})
