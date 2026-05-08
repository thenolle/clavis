# Clavis - Official Guide

Clavis is a lightweight Windows app that plays keyboard and mouse sounds from soundpacks, and lets you mute or unmute instantly with a global hotkey from anywhere on your PC. It is a native Win32 `.exe`, uses a tray icon, loads packs from the built-in embedded list plus the `Soundpacks` folder next to the app, and starts with one selected pack when at least one pack is available.

## Start here

1. Launch `Clavis.exe`.
2. Clavis creates a `Soundpacks` folder next to the executable if it does not already exist.
3. Put one or more soundpack folders inside `Soundpacks`.
4. Select a pack from the list in the app window.
5. Use the mute button in the window or the global hotkey to mute and unmute Clavis at any time.

If Clavis finds at least one available pack, it automatically loads the first one in the list when the app starts.

## Using Clavis

Clavis shows a soundpack list, a settings area, a volume slider, a mute button, a hotkey row, and a status line at the bottom. The list can contain built-in embedded packs and packs discovered from the `Soundpacks` directory, and the selected row shows a checkmark while embedded packs can include a star in the visible label text.

To use the app day to day:

- Click a soundpack in the list to load it immediately.
- Turn key down sounds on or off with the first checkbox.
- Turn key up sounds on or off with the second checkbox.
- Turn mouse click sounds on or off with the third checkbox.
- Turn close-to-tray behavior on or off with the fourth checkbox.
- Drag the volume slider to set playback volume from 0 to 100.
- Click the large mute button to switch between active and muted states instantly.
- Click the hotkey edit control to start remapping the global mute shortcut.

The status text at the bottom updates for actions such as loading a pack, updating the hotkey, or cancelling remap, then returns to the normal loaded-pack count message after a short delay.

## Soundpacks

A soundpack is a folder containing a required `config.json` file and one or more audio files referenced by that config. Clavis discovers packs by scanning subfolders inside `Soundpacks` and only includes folders that contain `config.json` directly inside them.

Expected structure:

```text
Soundpacks/
└── my-pack/
    ├── config.json
    ├── a.wav
    ├── enter.wav
    ├── space.wav
    └── icon.png
```

For disk-based packs, Clavis reads the folder from `Soundpacks`, loads `config.json`, and resolves audio file paths relative to that pack folder. For embedded packs, Clavis reads the same `config.json` data from the compiled-in file table and uses that pack the same way from the user’s point of view.

## Writing config.json

Clavis reads the pack display name from the top-level `"name"` field in `config.json`, and falls back to the folder name only if no name is present. It supports a keyboard sound mode using `"key_define_type"` and either a single default file via `"sound"` or a per-key map via `"defines"`.

Single-sound example:

```json
{
  "name": "My Pack",
  "key_define_type": "single",
  "sound": "a.wav",
  "mouse_down": "click.wav",
  "mouse_up": "release.wav"
}
```

Per-key example:

```json
{
  "name": "NK Cream",
  "key_define_type": "multi",
  "defines": {
    "default": "a.wav",
    "KEY_RETURN": "enter.wav",
    "KEY_SPACE": "space.wav",
    "KEY_LEFTSHIFT": "shift.wav",
    "KEY_RIGHTSHIFT": "shift.wav"
  }
}
```

Clavis also supports optional `"defines_up"` for key-release sounds, plus optional `"mouse_down"` and `"mouse_up"` entries for mouse click playback. If a pressed key has no direct match in `"defines"` or `"defines_up"`, Clavis falls back to `"default"` when it exists, and if that is also missing it falls back to the first loaded sound entry.

## Supported key names

Clavis maps many common Win32 virtual keys to config key names, including letters, digits, arrows, navigation keys, modifiers, numpad keys, punctuation keys, and function keys. Examples include `KEY_A` to `KEY_Z`, `KEY_0` to `KEY_9`, `KEY_RETURN`, `KEY_SPACE`, `KEY_TAB`, `KEY_BACKSPACE`, `KEY_LEFTSHIFT`, `KEY_RIGHTSHIFT`, `KEY_LEFTCTRL`, `KEY_RIGHTCTRL`, `KEY_LEFTALT`, `KEY_RIGHTALT`, `KEY_F1` to `KEY_F12`, and `KEY_LEFT`, `KEY_RIGHT`, `KEY_UP`, `KEY_DOWN`.

A few useful examples:

- `KEY_RETURN` for Enter
- `KEY_SPACE` for Space
- `KEY_BACKSPACE` for Backspace
- `KEY_LEFTSHIFT` and `KEY_RIGHTSHIFT` for Shift
- `KEY_LEFTCTRL` and `KEY_RIGHTCTRL` for Ctrl
- `KEY_LEFTALT` and `KEY_RIGHTALT` for Alt
- `default` for a fallback sound when no specific key match exists

## Hotkey and mute

Clavis registers a global hotkey with Windows and uses it to toggle mute even when the app window is not focused. The current default in the code is `Ctrl+Shift+M`, because `HOTKEY_DEFAULT_VK` is `0x4D` and `HOTKEY_DEFAULT_MODS` is `MOD_CONTROL | MOD_SHIFT`.

To remap the hotkey:

1. Click the hotkey edit control in the app.
2. The status text changes to `Press a new key combination…`.
3. Press your new modifier combination and trigger key.
4. Clavis re-registers the hotkey immediately with no restart required.

If you click the edit control again while remap is active, Clavis cancels the remap and restores normal state without changing the hotkey. When the hotkey changes successfully, the UI refreshes its label and the tray tooltip is updated to include the current state and the hotkey label.

## Tray behavior

Clavis adds a notification area icon when it starts, and the tray tooltip begins in the active state. The tray menu includes actions for opening the window, toggling mute or unmute, enabling start with Windows, enabling close to tray, and exiting the app.

Tray behavior works like this:

- Double left-click the tray icon to show or hide the window.
- Right-click the tray icon to open the tray menu.
- Choose the mute or unmute entry to toggle sound playback state.
- Choose `Open Clavis` to show or hide the main window.
- Choose `Exit` to close the app fully.

If close-to-tray is enabled, clicking the window close button hides Clavis instead of exiting it. If close-to-tray is disabled, closing the window destroys the app normally.

## Audio behavior

Clavis plays sounds either from embedded in-memory file data or from file paths on disk, depending on how the selected pack was loaded. The current volume setting is applied to playback, mute prevents playback without unloading the current pack, and keyboard and mouse hooks remain installed so unmuting is immediate.

For the best experience:

- Prefer short `.wav` files.
- Keep filenames in `config.json` exactly matched to the files in the pack folder.
- Use `default` in multi-key packs so unmatched keys still produce sound.
- Add `mouse_down` and `mouse_up` only if you want mouse click sounds

## Troubleshooting

If a pack does not appear, make sure it is inside the `Soundpacks` folder next to `Clavis.exe` and that `config.json` is directly inside the pack folder. If the app opens but you hear nothing, make sure a pack is selected, Clavis is not muted, the relevant sound toggles are enabled, and the volume slider is above zero.

If the hotkey does not trigger, another app may already be using the same shortcut, because Clavis relies on Windows hotkey registration and falls back only by retrying registration without `MOD_NOREPEAT` when needed. If closing the window seems to “do nothing,” close-to-tray is enabled, so check the tray icon instead of launching a second copy, since Clavis uses a single-instance mutex and ignores additional launches while one instance is already running.