# Helbreath client (Windows + web)

The C++ Helbreath International client. `Web/` builds the same code to WebAssembly, so the game runs in a browser.
It shows 854x480 (16:9): the world uses the full width, and the screens drawn for 640x480 (login, menus) are
centred. In game, the HUD and dialog boxes sit on a centred 640x480 board, and the guide map snaps to the corners of
the whole frame.

It goes with [helbreath-rust-server](https://github.com/Krroks/helbreath-rust-server), whose game server sends the
wider 28-column map window this client needs.

## Game data

The executable or the web page needs a Helbreath client folder (`SPRITES/`, `MAPDATA/`, `SOUNDS/`, `MUSIC/`,
`FONTS/`, `GM.cfg`...). It is not included here.

- Copy the paks in `assets/sprites/` into its `SPRITES/` folder. `assets/sprites/README.md` says what each one is.
- `GM.cfg` tells the client where the login server is:

  ```ini
  [CONFIG]
  log-server-address  = 127.0.0.1
  log-server-port     = 4000
  game-server-mode    = LAN
  ```

  Without it, the client uses `SERVER_IP` / `SERVER_PORT` from `GlobalDef.h`.

## Web build

Requirements: the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) installed in
`C:/emsdk` (change `EM=` at the top of `Web/build.sh` if it lives elsewhere), and a bash shell (Git Bash on
Windows).

```sh
Web/build.sh -O2      # or -O0 for a debug build
```

The output is `Web/out/index.html`, `index.js` and `index.wasm`. The object files go to `Web/obj-O2` (or
`$HB_WEB_OBJ-O2`). An object is rebuilt when its `.cpp` or any header changes; `HB_REBUILD=1` rebuilds everything.

## Run in the browser

1. Start the login server, the game server and the bridge (see helbreath-rust-server). The bridge must allow the
   login and game server hosts, for example `bridge --listen 127.0.0.1:8000 --allow-host 127.0.0.1`.
2. Serve the page:

   ```sh
   node Web/tools/serve.mjs 8080 path/to/HelbreathClient 8000
   ```

   The arguments are the port, the game data folder and the bridge port. `serve.mjs` serves `Web/out/` at `/` and
   the game folder at `/data/`; the client reads `.pak` files in pieces, so it supports HTTP Range. It also hands
   WebSocket upgrades on `/ws/...` to the bridge, and tells open pages to reload a sprite pak when one changes on
   disk.
3. Open http://127.0.0.1:8080/. Two optional query parameters: `?data=URL` for another data folder, and
   `?bridge=ws://HOST:PORT` to talk to a bridge directly.

In game, the keys 1 to 9 and 0 act as F1 to F10, because browsers keep the function keys for themselves.

## Windows build

Open `Client.sln` in Visual Studio 2022 and build Win32 (Debug or Release). Set the platform toolset (v143) and the
Windows SDK in the project properties if Visual Studio asks. The executable needs the same game data folder and
`GM.cfg`.

## Code map

| Where | What |
|---|---|
| `Game.cpp`, `Game.h` | the game: screens, network messages, drawing, input |
| `GlobalDef.h` | build options, default server address, screen size (`SCREENX`, `SCREENY`, `UISHIFTX`, `PLAYER_VIEW_TX`) |
| `DirectX/` | DirectDraw / DirectInput / DirectSound wrappers, sprites, effects |
| `Map/`, `char/`, `Net/`, `UI/` | map data, character and item data, sockets and messages, Windows entry point |
| `Web/` | browser port: stand-ins for DirectDraw, DirectInput, DirectSound and Win32, the page shell, the dev server |
| `assets/sprites/` | modified sprite paks used by this client |
