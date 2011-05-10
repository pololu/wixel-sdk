This directory contains the NSIS source scripts used to compile the
Wixel Development Bundle's installer.  For more information, see:

http://nsis.sourceforge.net

NSIS is Copyright (C) 1995-2009 Contributors
For full license information, see:

http://nsis.sourceforge.net/License

To rebuild the installer from scratch:

First:
1. Install NSIS v2.46+, available from http://nsis.sourceforge.net.
   You should add the NSIS installation directory to your path so you
   can easily run makensis.
2. Install the EnvVarUpdate plugin for NSIS, available from
   http://nsis.sourceforge.net/Environmental_Variables:_append,_prepend,_and_remove_entries
3. Install Doxygen, available from  http://www.stack.nl/~dimitri/doxygen/
4. Update WIXELTOOLVERSION in wixel_tools_mui.nsi to today's date.
5. Update TOOLSVER in build_tools.nsi if the Pololu GNU Build
   Utilities have changed since last time.

Second, create a new folder named "bundle" and assemble the following
in that folder:
1. A folder named "build_tools" that contains the Pololu GNU Build
   Utilities.
2. The SDCC installer (e.g. "sdcc-3.0.0-setup.exe").  The version number
   is specified in the SDCCVER variable in wixel_tools_mui.nsh;  update
   that variable if needed.
3. The Notepad++ installer (e.g. "npp.5.9.Installer.exe").  The version
   number is specified in the NPVER variable in wixel_tools_mui.nsh;
   update that variable if needed.
4. The Wixel SDK repository itself, in a folder named  "wixel-sdk".
   Make sure that wixel-sdk:
   - Contains any changes you made in the previous steps.
   - Contains a .git folder
   - Specifies a remote named pololu in .git/config like this:
     [remote "pololu"]
        url = git://github.com/pololu/wixel-sdk.git
        fetch = +refs/heads/*:refs/remotes/pololu/*
   - Has the autocrlf option turned on ("autocrlf = true" in the
     [core] section of .git/config)
   - Has Doxygen-generated docs in wixel-sdk/docs/html.  Run
     "make docs" to generate these docs.

Finally, to create the installer:
1. Open a Command Prompt and navigate to the bundle directory.
2. Compile build_tools.nsi by running:
       makensis /DSTARTDIR=C:\...\bundle\build_tools wixel-sdk\installer\build_tools.nsi"
   where "C:\...\bundle\build_tools" is the full path to the build_tools directory.
   You should end up with a file named pololu_gnu_build_tools.exe in the bundle folder.
2. Compile wixel_tools_mui.nsi by running:
       makensis /DSTARTDIR=C:\...\bundle wixel-sdk\installer\wixel_tools_mui.nsi
   where "C:\...\bundle" is the full path to the bundle directory.

For more info on command line NSIS, see http://nsis.sourceforge.net/Docs/Chapter3.html
