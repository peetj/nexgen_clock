# nexgen_clock

Frameless IPC-controlled clock utility (Qt Widgets). Controlled by `nexgen_tray`.

## IPC
Listens on `nexgen.clock` (QLocalServer) and accepts newline-delimited compact JSON.

Commands:
- `{ "cmd": "toggle" }`
- `{ "cmd": "show" }`
- `{ "cmd": "hide" }`
- `{ "cmd": "getState" }`
- `{ "cmd": "setTimezone", "tz": "Australia/Sydney" }`

## Build
```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="/c/Qt/6.11.0/mingw_64"
cmake --build build
./build/nexgen_clock.exe
```
