# run from ./build

sudo LD_LIBRARY_PATH=$(pwd)/:$LD_LIBRARY_PATH \
     STEAM_CLIENT_PATH=$(pwd)/ \
     gdbserver :1234 ./VPNOverSteamApp -s -li 0 -lp MyPassword