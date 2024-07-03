Invoke-WebRequest -Uri https://github.com/pbatard/libwdi/releases/download/v1.4.1/zadig-2.7.exe -Outfile zadig-2.7.exe
# TODO: Make PowerShell wait for this to exit before continuing the script, then uncomment Remove-Item
.\zadig-2.7.exe
#Remove-Item zadig-2.7.exe

pip3 install pyserial esptool
