@echo off
set loop=0

echo upload start

echo resetting the pico
<<<<<<< HEAD
mode com3 BAUD=1200 
=======
mode com13 BAUD=1200 
>>>>>>> 0c298be106369aea0613ffb5026da950a453de78

:loop

copy build\*.uf2 D:\
echo %errorlevel%
if "%errorlevel%"=="0" goto success

set /a loop=%loop%+1 
if "%loop%"=="10" goto fail

timeout 1 > NUL
goto loop

:fail
echo failed to programm
timeout 1 > NUL
exit

:success
echo pico programmed
timeout 1 > NUL
exit