# 2015 Growtopia (v1.93)

A working Growtopia private server written in modern C++, targeting the
**v1.93 client (July 2015)**.

It runs and is playable — you can log in, build, grow trees, trade and use most
items. It is also **rough**: expect bugs, and expect to find things that aren't
implemented yet. Contributions are very welcome.

---

## Status

### Working

- **Worlds** — procedural generation, persistent tiles, world select menu
- **Locks** — Small, Big, Huge and World Lock, access lists, per-world bans
- **Growing** — planting, grow timers, fruit counts, harvesting, seed splicing
- **Chemical combiners** — Laboratory, E-Z Cook Oven, Microwave, Barbecue Grill
- **Consumables** — all 97 in the 1.93 item set, plus timed status effects
- **Friends**, **trading**, **inventory** (200 stack cap, merging, ground drops)
- **XP and levelling**, death and respawn, lava damage
- **Blocks** — weather machines, providers, doors, gateways, portals,
  checkpoints, signs, bulletin boards, mailboxes, treasure blocks, spotlights
- **Chat commands**, emotes, moderation tools and roles

### Not implemented yet

Mannequin, Donation Box, Crystal harmonisation, Phone Booth, Scoreboard,
Achievement Block, Profile, Sungate, Security Camera, Clothing Compactor,
Summer Breeze, the game-block set, and Boombox world music, and many other specifics that I most likely forgot.

---

## Building

Requires **CMake** and a **C++17** compiler. All dependencies are vendored under
`libraries/` — there is nothing to install separately.

Windows (MSVC) is the primary target:

```bat
build.bat
```

or directly:

```bat
cmake --build out\build\x64-Debug --target server
```

A `build.sh` is included for Unix-likes.

---

## Running

The server reads the real `items.dat` from `cache/` and writes flat-file JSON
saves to `db/` (`db/players/` and `db/worlds/`) — no database setup is needed.

Launch the built `server` binary and point a v1.93 client at it.

---

## Layout

```
src/
  Commands/       chat commands
  ENetWrapper/    networking
  Event/          action, dialog and tank-packet handlers
  Manager/        items, trading, persistence
  Packet/         wire formats
  Player/         player state and inventory
  Protocol/       client protocol structures
  Server/         server and player pools
  Utils/          shared helpers
  World/          worlds, tiles, locks
libraries/        vendored dependencies
cache/            items.dat and item descriptions
runtime/          runtime data
```

---

## Contributing

Help is genuinely wanted. Useful places to start:

- **Unimplemented blocks** — several are listed above, and most are
  self-contained enough to pick up without touching the rest of the server.
- **Bug reports** — anything that behaves differently from the real 1.93 game.
- **v1.93 knowledge** — details of how a specific item or mechanic actually
  behaved are as valuable here as code.

The guiding rule for this project: mechanics should match how the 2015 game
actually behaved. Where something cannot be established, it is better to leave
it unimplemented and write down what is known than to invent behaviour that
merely looks plausible.
