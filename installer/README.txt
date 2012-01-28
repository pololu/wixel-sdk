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
2. Install the "Large strings" build of NSIS on top of that so we
   truncate user's paths to 8192 bytes instead of 1024.
3. Install the EnvVarUpdate plugin for NSIS, available from
   http://nsis.sourceforge.net/Environmental_Variables:_append,_prepend,_and_remove_entries
   EnvVarUpdate.nsh needs to be put in C:\Program Files (x86)\NSIS\include.
4. Install Doxygen, available from  http://www.stack.nl/~dimitri/doxygen/
5. Update BUNDLE_VER and SDK_VER in dev_bundle.nsi to today's date.

Second, create a new folder named "build" in this directory and assemble the
following in the "build" folder:
1. A folder named "pololu-gnu-build-utils-VERSION" that contains the
   Pololu GNU Build Utilities.  Update the UTILS_VER variable in dev_bundle.nsi
   and utils.nsi to match this version number if needed.
2. The SDCC installer, renamed to "sdcc-VERSION-setup.exe".  Update the SDCC_VER
   variable in dev_bundle.nsi to match this version number if needed.
3. The Notepad++ installer (e.g. "npp.VERSION.Installer.exe").  Update the
   NPP_VER variable in dev_bundle.nsi to match this version number if needed.
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
   - Does NOT have any compilation output files.
   You can use prepare_sdk.bat to create the wixel-sdk folder.

Finally, to create the installer:
1. Right click on sdk.nsi and select "Compile NSIS Script".
2. Do the same for utils.nsi.
3. Do the same for dev_bundle.nsi.
