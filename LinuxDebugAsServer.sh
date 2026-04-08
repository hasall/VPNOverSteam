# run from ./build

sudo LD_LIBRARY_PATH=$(pwd)/:$LD_LIBRARY_PATH \
     STEAM_CLIENT_PATH=$(pwd)/ \
     gdbserver :1234 ./VPNOverSteamApp -s -ln MyLobby -lp MyPassword