This directory contains the NSIS source scripts used to compile the Wixel Development Bundle's installer.  For more information, see:

http://nsis.sourceforge.net

NSIS is Copyright (C) 1995-2009 Contributors
For full license information, see:

http://nsis.sourceforge.net/License

To rebuild the installer from scratch:

First, assemble the following:
1. The Wixel SDK files themselves, in a directory called wixel-sdk
2. The GNU build tools
3. The SDCC installer (by default the system expects the SDCC version declared in line 22 of wixel_tools_mui.nsh, e.g. !define SDCCVER "3.0.0")
4. The notepad++ installer (the system currently expects version 5.8.7, this is declared in line 23)
5. Place the SDCC and notepad++ installers in the same directory as the wixel-sdk directory, one level above the wixel-sdk files
6. Download and install doxygen (http://www.stack.nl/~dimitri/doxygen/), which is used to build the documentation files
7. With doxygen installed, open a command-line window (start->run->cmd) and navigate to your wixel-sdk folder
8. Type "make docs" at the command line


To create the installer:
(For more info on command line NSIS, see http://nsis.sourceforge.net/Docs/Chapter3.html)
1. Install NSIS v2.46+ along with the EnvVarUpdate plugin (available at http://nsis.sourceforge.net/Environmental_Variables:_append,_prepend,_and_remove_entries)
2. Compile build_tools.nsi from the command line, passing it the the location of the loose files as the variable "STARTDIR"
    e.g. "makensis /DSTARTDIR=c:\working\wixel-installer\gnu-build-tools c:\working\wixel-installer\build_tools.nsi"
    You should end up with a file named pololu_gnu_build_tools_(DATE).exe in the directory two levels above your "installer" directory.
3. Compile wixel_tools_mui.nsi from the command line, passing it the location of the wixel-sdk folder as the variable "STARTDIR"
    e.g. "makensis /DSTARTDIR=c:\working\main\ c:\working\main\wixel-sdk\installer\Wixel_Tools_MUI.nsi"

Notes:
1. The wixel_tools_mui.nsi script will look for the GNU build tools installer (by default pololu_gnu_build_tools_DATE.exe) in the same directory as the wixel-sdk folder.  If your install script is in a default location (under \wixel-sdk\installer) it should end up there by default.
