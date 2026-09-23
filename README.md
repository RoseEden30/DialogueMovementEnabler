# Dialogue Movement Enabler

Keep moving while you talk to people in Starfield, like *Dialogue Movement Enabler* for Skyrim.

- Walk during conversations, with your own key bindings. Running, jumping, sneaking and switching view are optional.
- Walk away and the conversation ends.
- Push the cursor against a screen edge to look around.

Movement keys no longer pick dialogue lines: use the mouse, arrow keys or number keys. The mod stays off when Dialogue Camera is turned on in Settings > Accessibility, and while another menu such as barter is open.

Requires [SFSE](https://sfse.silverlock.org/) and [Address Library for SFSE Plugins](https://www.nexusmods.com/starfield/mods/3256). Built for Starfield 1.16.244; on another version, anything that no longer matches is turned off and logged.

## Settings

`SFSE/Plugins/DialogueMovementEnabler.ini`, read at startup.

| Setting | Default | What it does |
|---|---|---|
| `bAllowMovement` | 1 | Walk and strafe |
| `bAllowRunning` | 0 | Run and sprint |
| `bAllowJumping` | 0 | Jump |
| `bAllowSneaking` | 0 | Sneak |
| `bAllowPOVSwitch` | 0 | Switch between first and third person |
| `bAutoClose` | 1 | End the conversation when you walk away |
| `fAutoCloseDistance` | 19 | How far, in metres |
| `fAutoCloseTolerance` | 6 | Extra distance for conversations started from further away |
| `bEdgeRotation` | 1 | Look around with the mouse at the screen edges |
| `fEdgeSize` | 0.12 | Width of the edge area, as a share of the screen |
| `fEdgeSizeBottom` | 0.03 | Same for the bottom edge, kept thin so it stays clear of dialogue lines |
| `fEdgeSpeed` | 12 | How fast the view turns |
| `bDebugLog` | 0 | Write details to the log, for troubleshooting |

Log: `Documents/My Games/Starfield/SFSE/Logs/DialogueMovementEnabler.log`.

## Credits

- Vermunds and alandtse: [Dialogue Movement Enabler](https://www.nexusmods.com/skyrimspecialedition/mods/43708) (Skyrim)
- SFSE team: [SFSE](https://sfse.silverlock.org/)
- meh321: [Address Library](https://www.nexusmods.com/starfield/mods/3256)
- [CommonLibSF](https://github.com/libxse/CommonLibSF) contributors

## License

[GPL-3.0-or-later](LICENSE) with the CommonLibSF [exceptions](EXCEPTIONS).

## Building

Visual Studio 2022, CMake 3.21+, Ninja and vcpkg with `VCPKG_ROOT` set.

```bash
git submodule update --init --recursive
```

```bash
cmake --preset release
```

```bash
cmake --build --preset release
```

The files ready to package end up in `build/release/package`. Set `STARFIELD_MODS_FOLDER` or `STARFIELD_FOLDER` to also copy the plugin there after each build; an existing `.ini` is never overwritten.
