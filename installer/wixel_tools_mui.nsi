####################
#
# wixel_tools_mui.nsi - installer script for the wixel development bundle
#
# Last modified: 110316 by KK
#
####################

# This installer expects to be passed the variable "STARTDIR" at the command line
# E.g. "makensis /DSTARTDIR=c:\working\wixel-installer c:\working\wixel-installer\wixel_tools_mui.nsi"

!include FileFunc.nsh
#Modern UI for fun and profit
!include MUI.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin

; !define STARTDIR=c:\foo\bar
; !define SDCC "sdcc-3.0.0-setup.exe"
!define WIXELTOOLVERSION "110317"
!define SDCCVER "3.0.0"
!define NPVER "5.8.7"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${STARTDIR}\wixel-sdk\installer\wixel_fullname_sm.bmp"
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH 
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

OutFile "..\..\wixel_dev_bundle_${WIXELTOOLVERSION}.exe"
InstallDir "$DOCUMENTS\Pololu\wixel-sdk"
Name "Pololu Wixel Development Bundle"
!insertmacro MUI_LANGUAGE "English"



LangString DESC_SECTION1 ${LANG_ENGLISH} "The Wixel SDK contains source code (for libraries and applications) that will help you develop your own applications for the Wixel."
LangString DESC_SECTION2 ${LANG_ENGLISH} "The Small Device C Compiler (SDCC) is a free compiler that is used to compile the code in the Wixel SDK."
LangString DESC_SECTION3 ${LANG_ENGLISH} "The Pololu GNU Build Utilities are open-source utilities that are required by the Wixel SDK: cat, cp, echo, grep, make, mv, and rm."
LangString DESC_SECTION4 ${LANG_ENGLISH} "Notepad++ is a free source code editor which is convenient to use when editing Wixel code."




Section "Source Code (wixel-sdk)" Section1
	; SectionIn RO
	DetailPrint "Now installing the wixel-sdk files"
	Call SDKexists
	SetOutPath "$INSTDIR"
	File /r "${STARTDIR}\wixel-sdk\"
	
	; SetOutPath "$INSTDIR\wixel-sdk\installer\"
	; File "c:\working\kalan\wixel\installer\wixel_tools_mui.nsi"
	; File "c:\working\kalan\wixel\installer\build_tools.nsi"
	; File "c:\working\kalan\wixel\installer\wixel_tools\wixel-sdk\installer\readme.txt"
	; DetailPrint "Now writing the uninstaller"
	; WriteUninstaller "$INSTDIR\uninstall wixel-sdk.exe"
	; WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk" \"DisplayName" "Pololu wixel-sdk"
	; WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk" "UninstallString" "$\"$INSTDIR\uninstall wixel-sdk.exe$\""
	; WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk" "Publisher" "Pololu"
	; WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk" "DisplayVersion" "${WIXELTOOLVERSION}"
	
	; DetailPrint "Now installing sdcc..."
	; SetOutPath "$TEMP"
	; File "c:\working\kalan\wixel\installer\wixel_tools\${SDCC}"
	; MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for SDCC - the small device C compiler"
	; ExecWait "$TEMP\${SDCC}"
SectionEnd

Section "SDCC ${SDCCVER}" Section2
	DetailPrint "Now installing sdcc..."
	SetOutPath "$TEMP"
	File "${STARTDIR}\sdcc-${SDCCVER}-setup.exe"
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for SDCC ${SDCCVER} - the small device C compiler"
	ExecWait "$TEMP\sdcc-${SDCCVER}-setup.exe"
SectionEnd

Section "Pololu GNU Build Utilities" Section3
	SetOutPath "$TEMP"
	File "${STARTDIR}\Pololu_GNU_build_tools_${WIXELTOOLVERSION}.exe"
	DetailPrint "Now running the Pololu GNU Build Utilities installer"
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for the Pololu GNU Build Utilities"
	ExecWait "$TEMP\Pololu_GNU_build_tools_${WIXELTOOLVERSION}"
SectionEnd

Section "Notepad++ Text Editor" Section4
	SetOutPath "$TEMP"
	DetailPrint "Installing Notepad++"
	File "${STARTDIR}\npp.${NPVER}.Installer.exe"
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for Notepad++"
	ExecWait "$TEMP\npp.${NPVER}.Installer.exe"
SectionEnd

Section "Uninstall"

	; DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk"
	
SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${Section1} $(DESC_SECTION1)
	!insertmacro MUI_DESCRIPTION_TEXT ${Section2} $(DESC_SECTION2)
	!insertmacro MUI_DESCRIPTION_TEXT ${Section3} $(DESC_SECTION3)
	!insertmacro MUI_DESCRIPTION_TEXT ${Section4} $(DESC_SECTION4)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


; Function .onInit
; ${if} ${FileExists} $Instdir
	; StrCpy $INSTDIR "$InstDir-${WIXELTOOLVERSION}"
; ${EndIF}
; FunctionEnd

Function SDKexists
${if} ${FileExists} $INSTDIR
	MessageBox MB_OK "The Wixel Dev Bundle installer has detected a previously-installed version of the Wixel SDK files in $INSTDIR.  To avoid conflicts, the new files will be installed to the backup location: $INSTDIR-${WIXELTOOLVERSION}."
	StrCpy $INSTDIR "$INSTDIR-${WIXELTOOLVERSION}"
${andif} ${FileExists} $INSTDIR
	${GetTime} "" "LS" $0 $1 $2 $3 $4 $5 $6
	MessageBox MB_OK "The Wixel Dev Bundle installer has detected a previously-installed version of the Wixel-SDK files in the backup location $INSTDIR.  To avoid conflicts, we will now install to $INSTDIR-$2$1$0$4$5$6"
	StrCpy $INSTDIR "$INSTDIR-$2$1$0$4$5$6"
${EndIF}
FunctionEnd
