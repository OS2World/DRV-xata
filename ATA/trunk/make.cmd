@echo off
:: make.cmd - nmake wrapper

:: 2010-09-13 SHL Ensure 4OS2; switch to ISO dates; switch to BUILDTIME BUILDMACH

:: Ensure 4OS2
if "%@eval[0]" == "0" goto is4xxx
  echo Must run in 4OS2 session
  goto eof
:is4xxx

if "%_DOS%" != "OS2" ( echo Must run in 4OS2 session %+ beep %+ cancel )

:: Define build time and build machine
set BUILDTIME=%_YEAR/%@right[2,0%_MONTH]/%@right[2,0%_DAY] %_TIME

iff not defined BUILDMACH then
  set BUILDMACH=%HOSTNAME
  iff not defined BUILDMACH then
    set BUILDMACH=unknown
    echo Warning: BUILDMACH and HOSTNAME not defined - BUILDMACH defaulted to %BUILDMACH
    beep
  endiff
endiff

echo on
nmake /nologo %$

:eof
