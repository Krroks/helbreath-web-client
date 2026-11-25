# Modified sprite paks

The client reads these from `run/client/SPRITES/`; this folder keeps the versions that differ from the 3.82 set
(`run/client/SPRITES_v382/`). Copy them there to install. The scripts that build them are in `src/tools/sprites/`.

| Pak | What it is | Made with |
|---|---|---|
| `LoginDialog.pak`, `New-Dialog.pak` | login, loading, main menu and closing screens with the Korea 2024 "The Heldenian" logo and "Loading Data" strip over the Classic art. Sheets 0 (loading, 854x502) and 1 (main menu, 854x510) of New-Dialog are 16:9, drawn across the whole frame | `apply_login_psd.py` (patches drawn in `G:\DEV\hb\login.psd`, taken at full resolution from Korea 2024); sheet 1: `set_sheet.py New-Dialog.pak 1 loginScreen.psd 0=0,0,854,480,0,0`; sheet 0: the art and subtitle layers of `loginScreen.psd` (so both screens match pixel for pixel) plus the white "Loading Data" and the strip of `loadingScreen2.psd` (both PSDs in `G:\DEV\hb\`) |
| `GameDialog.pak` | Korea 2024 HUD and dialog boxes; sheet 0 frame 0 is the 4.08 System Menu window | Korea 2024 pak + `bmp16to24.py` + `kr_sysmenu.py` + `exp_line.py` (frame 19 of sheet 6 = green 640x3 experience line) |
| `DialogText.pak` | Korea 2024 window titles and buttons | Korea 2024 `dialogtext.pak` + `bmp16to24.py` |
| `interface2.pak` | Korea 2024 HUD extras, small font, F1 help, crafting | Korea 2024 pak + `bmp16to24.py` |
| `interface.pak` | 3.0 cursor and sprite font (what Korea 2024 ships) | History `sprites/3.0/interface.pak` |
| `lgn_newacc.lpk` | create-account screen: the official 2.02 "The Crusade" board and buttons | `newacc_202.py` (History `sprites/2.02/New-Dialog.pak`, sheets 3 and 8) |
| `EventIcons.pak` | the v8.00 "EXP" and "EK" event badges (27x29, one sheet, frame 0 = Double EXP, 1 = Double EK), drawn top right while an event runs | `make_event_icons.py` (cut from `run/history_sheets/v800/GameDialog/006.png`, frames 30 and 31 of the v8.00 `GameDialog.pak` sheet 6) |
| `LowHealth.pak` | the v8.00 red screen-edge vignette of the low-health warning (854x480, dark middle = colour key), drawn translucent while HP is under 25 % | `make_lowhealth_vignette.py` (from `run/history_sheets/v800/hud1/000.png`) |
| `PartyHud.pak` | our party panel (header, member row with a level slot and HP / MP bars, bottom edge, bar fills), only drawn by this client; without it the client keeps the old party bars | `make_party_hud.py` (an empty panel drawn by the Queueueue `flux.edit` job from the Helbreath HUD as style reference, cut and measured in the script, which prints the numbers `PARTYHUD_*` in `Game.cpp`; `PartyHud_preview.png` shows the result) |

Korea 2024 paks: the decrypted copies in https://github.com/Original-Helbreath/History (`derived/sprites/korea-2024/`).
The client reads only 1/4/8/24-bit BMP sheets, hence `bmp16to24.py` for the Korean RGB555 ones.
