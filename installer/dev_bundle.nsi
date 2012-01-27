# wixel_tools_mui.nsi - installer script for the wixel development bundle

!define BUNDLE_VER "120126"
!define SDCC_VER "3.1.0"
!define NPP_VER "5.9.8"
!define UTILS_VER "120126"

!define STARTDIR ".\build"
OutFile ".\build\wixel_dev_bundle_${BUNDLE_VER}.exe"

# TODO: improve the user experience in the case where they choose to not install the Wixel SDK

!include FileFunc.nsh
!include EnvVarUpdate.nsh
#Modern UI for fun and profit
!include MUI.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin

; !define STARTDIR=c:\foo\bar
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP "${STARTDIR}\wixel-sdk\installer\wixel_fullname_sm.bmp"
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH 
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE

!define MUI_FINISHPAGE_RUN_TEXT "Show the installed wixel-sdk files"
!define MUI_FINISHPAGE_RUN_FUNCTION "ShowSDKFiles"

!define MUI_FINISHPAGE_RUN


!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

InstallDir "C:\wixel-sdk"
Name "The Pololu Wixel Development Bundle"
!insertmacro MUI_LANGUAGE "English"



LangString DESC_SDK ${LANG_ENGLISH} "The Wixel SDK contains source code (for libraries and applications) that will help you develop your own applications for the Wixel."
LangString DESC_SDCC ${LANG_ENGLISH} "The Small Device C Compiler (SDCC) is a free compiler that is used to compile the code in the Wixel SDK."
LangString DESC_UTILS ${LANG_ENGLISH} "The Pololu GNU Build Utilities are open-source utilities that are required by the Wixel SDK: cat, cp, echo, grep, make, mv, rm, and sed."
LangString DESC_NPP ${LANG_ENGLISH} "Notepad++ is a free source code editor which is convenient to use when editing Wixel code."


Section "Wixel SDK" SDK
	DetailPrint "Installing wixel-sdk..."
	Call SDKexists
	SetOutPath "$INSTDIR"
	File /r "${STARTDIR}\wixel-sdk\"
SectionEnd

Section "SDCC ${SDCC_VER}" SDCC
	DetailPrint "Installing SDCC..."
	SetOutPath "$TEMP"
	File "${STARTDIR}\sdcc-${SDCC_VER}-setup.exe"
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for SDCC ${SDCC_VER} - the small device C compiler"
	ExecWait "$TEMP\sdcc-${SDCC_VER}-setup.exe"
	DetailPrint "Making sure that SDCC's path is set properly..."
	ReadRegStr $9 HKLM "Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\SDCC" 'InstallLocation'
	ReadRegStr $9 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SDCC" 'InstallLocation'
	${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$9\bin"
SectionEnd

Section "Pololu GNU Build Utilities" UTILS
	SetOutPath "$TEMP"
	File "${STARTDIR}\pololu_gnu_build_utils_${UTILS_VER}.exe"
	DetailPrint "Installing Pololu GNU Build Utilities..."
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for the Pololu GNU Build Utilities"
	ExecWait "$TEMP\pololu_gnu_build_tools_${UTILS_VER}.exe"
SectionEnd

Section "Notepad++ Text Editor" NPP
	SetOutPath "$TEMP"
	DetailPrint "Installing Notepad++..."
	File "${STARTDIR}\npp.${NPP_VER}.Installer.exe"
	MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for Notepad++"
	ExecWait "$TEMP\npp.${NPP_VER}.Installer.exe"
SectionEnd

; Section "Uninstall"

	; DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_wixel_sdk"
	
; SectionEnd


!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SDK} $(DESC_SDK)
	!insertmacro MUI_DESCRIPTION_TEXT ${SDCC} $(DESC_SDCC)
	!insertmacro MUI_DESCRIPTION_TEXT ${UTILS} $(DESC_UTILS)
	!insertmacro MUI_DESCRIPTION_TEXT ${NPP} $(DESC_NPP)
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function SDKexists
	${if} ${FileExists} $INSTDIR
		MessageBox MB_OK "The Wixel Dev Bundle installer has detected a previously-installed version of the Wixel SDK files in $INSTDIR.  To avoid conflicts, the new files will be installed to the backup location: $INSTDIR-${BUNDLE_VER}."
		StrCpy $INSTDIR "$INSTDIR-${BUNDLE_VER}"
	${andif} ${FileExists} $INSTDIR
		${GetTime} "" "LS" $0 $1 $2 $3 $4 $5 $6
		MessageBox MB_OK "The Wixel Dev Bundle installer has detected a previously-installed version of the Wixel-SDK files in the backup location $INSTDIR.  To avoid conflicts, we will now install to $INSTDIR-$2$1$0$4$5$6"
		StrCpy $INSTDIR "$INSTDIR-$2$1$0$4$5$6"
	${EndIF}
FunctionEnd

Function ShowSDKFiles
	ExecShell "open" "$INSTDIR"
FunctionEnd

; Function .oninit

; FunctionEnd