# Modified sprite paks

The client reads these from `run/client/SPRITES/`; this folder keeps the versions that differ from the 3.82 set
(`run/client/SPRITES_v382/`). Copy them there to install. The scripts that build them are in `src/tools/sprites/`.

| Pak | What it is | Made with |
|---|---|---|
| `LoginDialog.pak`, `New-Dialog.pak` | login, loading, main menu and closing screens with the Korea 2024 "The Heldenian" logo and "Loading Data" strip over the Classic art | `apply_login_psd.py` (patches drawn in `G:\DEV\hb\login.psd`, taken at full resolution from Korea 2024) |
| `GameDialog.pak` | Korea 2024 HUD and dialog boxes; sheet 0 frame 0 is the 4.08 System Menu window | Korea 2024 pak + `bmp16to24.py` + `kr_sysmenu.py` |
| `DialogText.pak` | Korea 2024 window titles and buttons | Korea 2024 `dialogtext.pak` + `bmp16to24.py` |
| `interface2.pak` | Korea 2024 HUD extras, small font, F1 help, crafting | Korea 2024 pak + `bmp16to24.py` |
| `interface.pak` | 3.0 cursor and sprite font (what Korea 2024 ships) | History `sprites/3.0/interface.pak` |
| `lgn_newacc.lpk` | create-account screen: the official 2.02 "The Crusade" board and buttons | `newacc_202.py` (History `sprites/2.02/New-Dialog.pak`, sheets 3 and 8) |

Korea 2024 paks: the decrypted copies in https://github.com/Original-Helbreath/History (`derived/sprites/korea-2024/`).
The client reads only 1/4/8/24-bit BMP sheets, hence `bmp16to24.py` for the Korean RGB555 ones.
