# Window movement and settings

Drag the lake, last-catch strip, dashboard title, portrait, or sidebar headings to move the game. Interactive buttons, inventory slots and the upgrade tree retain their own input behavior. The cursor indicates draggable regions. Releasing a drag keeps the window on a monitor's work area and saves its position.

The gear button opens a **512 × 548** settings panel above the lake with five tabs. Every row shows a title and a one-line description; toggles flip when the row is clicked, sliders drag or respond to the mouse wheel, and segmented choices highlight the active value. Changes apply immediately and are saved once the mouse button is released (sliders would otherwise write the save file every frame).

Header controls: recover window position, minimize, save and exit. Footer: **Reset settings** (restores every preference plus auto-sell and always-on-top; fish, money and upgrades are kept) and **Save now**.

## General

- Discord Rich Presence: connects or disconnects the local Discord IPC while the game runs. `--no-discord` still disables it for a session.
- Start with Windows: adds or removes a `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` entry pointing at the current executable. The entry is refreshed on startup only while the option is on.
- Show tooltips: hover hints for buttons, slots and fish.
- 12-hour clock: HUD clock as `6:00 PM` instead of `18:00`.
- Save file: shows the save path with an **Open folder** button.
- Local time and version information.

## Window

- Always on top: existing window pin behavior.
- Lock window position: prevents mouse dragging; the settings controls remain available.
- Remember window position: saves the compact lake position (in window pixels at the current scale), including when sidebars are open. Startup clamps it to an available monitor.
- Dock position: left, center or right. Changing it, or pressing Dock, snaps the window above the taskbar.
- Gap above taskbar: 0–48 px in 4 px steps; default 8 px. Applied immediately while docked.
- Opacity: 30%–100% in 5% steps; default 100%.
- Opacity when idle: 30%–100%. Used while the window is unfocused and the cursor is not over it; the window eases between the two values. 100% disables the fade.
- Recover window: **Reset now** or **Shift + F12** moves the window above the taskbar of the primary monitor, for windows lost on a disconnected display.

## Display

- Window scale: 75%, 100%, 125% or 150%. The whole UI (lake, dashboard, stats tree) renders through a scaled camera; input is mapped back to the 512-wide layout. **Shift + F11** restores 100% scale and full opacity.
- Frame rate: 30, 60 or 120 FPS while active. Minimized rendering remains limited to 15 FPS.
- Background frame rate: Same, 30 or 15 FPS while unfocused with the cursor away.
- VSync: toggles the swap interval at runtime.
- Show clock and catch status.
- Catch and discovery notices.
- Water and casting effects.
- Animate lake background: freezes its animation without freezing the local hour or gameplay.

## Fishing

- Auto sell common catches.
- Protect special catches: newly stored specials are automatically locked. Existing stored items are not relocked.
- Protect rare catches: same automatic lock for Rare, Epic and Legendary catches.
- When the Fish Box is full: **Wait** (default, the catch stays on the hook), **Sell catch** (sells the new catch unless a protect rule applies to it) or **Sell cheapest** (sells the lowest-value unlocked stored fish to make room; if everything is locked the game waits).
- Pause fishing when unfocused: pauses the fishing timer while other windows have focus. Default off.
- Special encounter rate and local time are shown for reference.

## Hotkeys

- Keyboard shortcuts: disables the single-key shortcuts (B, C, U, S, L, A, R, F, T, D) so accidental key presses cannot buy upgrades or sell fish. Esc and the Shift + F11 / F12 recovery keys always work.
- The tab lists every shortcut, including Alt + click, right click and Alt + F4.

## Save format

Save version 6 writes a `name <bytes> <utf8>` line after preferences so the angler name round-trips; versions 1 through 5 still load with an empty name. Version 5 writes `preferences <count> <values…> <hasPosition> <x> <y>`; the count lets future preferences be appended while older files keep defaults for anything missing. Version 4 (eleven preferences without a count) and versions 1 through 3 remain supported. Out-of-range values are rejected with the existing backup recovery behavior.

The grouping is tailored to this game; Task Bar Hero's settings (window pin and scale, FPS cap, VSync, log/notification options, the Shift + F11 / F12 recovery hotkeys and the auto-retry idle toggle) were used as the reference for which controls a taskbar companion should expose.
