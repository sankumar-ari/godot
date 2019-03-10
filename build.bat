:: This script must be run from a Windows machine with
:: Visual Studio 2015, Python 2.7 and SCons installed at their
:: default locations. 7-Zip also needs to be installed (to compress editor binaries).

:: NOTE: You need Pywin32 to be installed to use multi-threaded compilation.
:: You may need to set "threads" to 1 for the first build, even if you have it installed.

:: Place this script at the root of your Godot Git clone.
:: CC0 1.0 Universal

:: set threads=%NUMBER_OF_PROCESSORS%
set threads=20

:: Build 64-bit Godot
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64
:: call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=yes target=release_debug
:: call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=no target=release
call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=yes

:: 
:: :: Build 32-bit Godot
:: call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
:: call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=yes target=release_debug
:: call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=no target=release
:: call "C:\Python27\Scripts\scons.bat" p=windows -j%threads% verbose=no tools=no target=release_debug
:: 
:: :: Install binaries in Godot's export templates directory
:: cd bin\
:: copy godot.windows.opt.debug.64.exe %APPDATA%\Godot\templates\windows_64_debug.exe
:: copy godot.windows.opt.64.exe %APPDATA%\Godot\templates\windows_64_release.exe
:: copy godot.windows.opt.debug.32.exe %APPDATA%\Godot\templates\windows_32_debug.exe
:: copy godot.windows.opt.32.exe %APPDATA%\Godot\templates\windows_32_release.exe
:: 
:: :: Compress editor binaries using .zip
:: call "%PROGRAMFILES%\7-Zip\7z.exe" a godot.windows.opt.tools.64.zip godot.windows.opt.tools.64.exe
:: call "%PROGRAMFILES%\7-Zip\7z.exe" a godot.windows.opt.tools.32.zip godot.windows.opt.tools.32.exe
:: 
:: :: Copy editor archives in a separate folder with a cleaner name
:: copy godot.windows.opt.tools.64.zip %APPDATA%\Godot\editor\godot-windows-x86_64.zip
:: copy godot.windows.opt.tools.32.zip %APPDATA%\Godot\editor\godot-windows-x86.zip