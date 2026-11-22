# Modified sprite paks

The client reads these from `run/client/SPRITES/`; this folder keeps the versions that differ from the 3.82 set
(`run/client/SPRITES_v382/`). Copy them there to install. The scripts that build them are in `src/tools/sprites/`.

| Pak | What it is | Made with |
|---|---|---|
| `LoginDialog.pak`, `New-Dialog.pak` | login, loading, main menu and closing screens with the Korea 2024 "The Heldenian" logo and "Loading Data" strip over the Classic art. Sheets 0 (loading, 854x502) and 1 (main menu, 854x510) of New-Dialog are 16:9, drawn across the whole frame | `apply_login_psd.py` (patches drawn in `G:\DEV\hb\login.psd`, taken at full resolution from Korea 2024); sheet 1: `set_sheet.py New-Dialog.pak 1 loginScreen.psd 0=0,0,854,480,0,0`; sheet 0: the art and subtitle layers of `loginScreen.psd` (so both screens match pixel for pixel) plus the white "Loading Data" and the strip of `loadingScreen2.psd` (both PSDs in `G:\DEV\hb\`) |
| `GameDialog.pak` | Korea 2024 HUD and dialog boxes; sheet 0 frame 0 is the 4.08 System Menu window | Korea 2024 pak + `bmp16to24.py` + `kr_sysmenu.py` |
| `DialogText.pak` | Korea 2024 window titles and buttons | Korea 2024 `dialogtext.pak` + `bmp16to24.py` |
| `interface2.pak` | Korea 2024 HUD extras, small font, F1 help, crafting | Korea 2024 pak + `bmp16to24.py` |
| `interface.pak` | 3.0 cursor and sprite font (what Korea 2024 ships) | the 3.0 client's `interface.pak` |
| `lgn_newacc.lpk` | create-account screen: the official 2.02 "The Crusade" board and buttons | `newacc_202.py` (the 2.02 client's `New-Dialog.pak`, sheets 3 and 8) |

Korea 2024 paks: decrypted copies of the Korea 2024 client's sprites.
The client reads only 1/4/8/24-bit BMP sheets, hence `bmp16to24.py` for the Korean RGB555 ones.
