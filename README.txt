Pololu Wixel SDK
http://www.pololu.com/

== Summary ==

This package contains code, utilities, and Makefiles which will
help you build your own applications for the Pololu Wixel, a
wireless development platform based around the CC2511F32 chip from
Texas Instruments.


== Getting Started in Windows ==

Get the Wixel SDK, either by downloading it or by cloning the git
repository.

Install the C compiler by running win_utils/sdcc-3.0.0-setup.exe.
This is a stable release of SDCC, the Small Device C Compiler.
    (If you would prefer to get a later version of
    SDCC, you can download it from their website:
    http://sdcc.sourceforge.net/ )

In order for you to use the Makefile successfully, you will need to
have certain utilities on your path.  Please go to the Control Panel
and add the "win_utils" folder to your path.
    (If you alread have all of the needed utilities on your PATH,
    you can skip this step.)


== Building Your App (any platform) ==

Open a command-line terminal, navigate to the top level directory
of the SDK, and type "make".  This will build all of the apps in
the apps folder and all of the libraries that they depend on.

