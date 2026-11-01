# MallVR Cardboard: Pac-Man VR

This is a native Android phone build of MallVR for Google Cardboard. It launches
only Pac-Man VR in single-player mode. The Cardboard SDK handles head tracking,
viewer lens correction, and QR viewer setup. MallVR's existing DOSBox-X renderer
produces the two eye views.

## Controller

Pair an Xbox controller in Android Bluetooth settings before putting the phone
in the viewer. The launcher has a shortcut to Bluetooth settings.

- Left stick: strafe and move back
- Right stick: turn
- Right trigger: move forward
- Left trigger: move backward
- Xbox Menu/Start: start the game (coin and start)
- Xbox View/Back: recenter the headset
- Hold B + Menu/Start for one second: return to the launcher

## First launch

1. Install the APK and open **MallVR Cardboard**.
2. Allow the requested file access so MallVR can install the game and write its
   DOSBox configuration and save data under `/sdcard/MallVR`.
3. Pair the controller. Optionally scan the QR code on the Cardboard viewer; if
   skipped, the SDK uses its Cardboard V1 viewer profile.
4. Start Pac-Man VR and place the phone in the viewer.

The APK is built with the Pac-Man executable, CD image, and the shared DOS/VPC
files from the supplied SU2000 dump. It does not expose or launch the other
games in that dump.

## Build with your own game files

Keep the extracted `games` folder from your MallVR installation outside this
Git checkout, for example at `C:\Games\MallVR\games`. The build expects the
folder to contain the SU2000 dump with one `PAC/PAC.EXE`, its matching
`PACMAN.CUE` and `PACMAN.BIN`, and the shared DOS/VPC support directories. The
script checks the supported Pac-Man executable and CD image by SHA-1 before
packaging them.

Pass the location of your `games` folder to `-GameFiles`:

```powershell
cd .\quest\quest\phone
.\build_cardboard.ps1 -GameFiles 'C:\Games\MallVR\games'
```

The files are staged only in `app/build/generated/assets/gamefiles/` while
building the APK; they do not belong in `app/src/main` and are ignored by Git.
The APK includes Pac-Man and its shared DOS/VPC support, but not the other
arcade games. The script uses the Android SDK and SDL2/gl4es checkouts in the
adjacent `work` directory. It writes the debug APK to the workspace's `outputs`
folder.

## Device notes

The build targets arm64 Android 10 or newer with GLES 3.0 and a gyroscope. The
APK can be built without a phone attached; Cardboard optics, headset recentering,
Bluetooth controller mapping, and game performance still need a real-device
check.
