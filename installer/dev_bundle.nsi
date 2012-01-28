# wixel_tools_mui.nsi - installer script for the wixel development bundle

!define BUNDLE_VER "120127"
!define SDK_VER "120127"
!define SDCC_VER "3.1.0"
!define UTILS_VER "120126"
!define NPP_VER "5.9.8"

!define STARTDIR ".\build"
OutFile ".\build\wixel-dev-bundle-${BUNDLE_VER}.exe"

!include FileFunc.nsh
!include EnvVarUpdate.nsh
!include MUI.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin

; !define STARTDIR=c:\foo\bar
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP_NOSTRETCH 
!define MUI_HEADERIMAGE_BITMAP ".\wixel_fullname_sm.bmp"
!define MUI_FINISHPAGE_NOAUTOCLOSE

!define MUI_COMPONENTSPAGE_SMALLDESC

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_INSTFILES

Name "Pololu Wixel Development Bundle"
!insertmacro MUI_LANGUAGE "English"

LangString DESC_SDK ${LANG_ENGLISH} "Libraries and example code for the Wixel."
LangString DESC_SDCC ${LANG_ENGLISH} "Free C compiler that is used to compile the code in the Wixel SDK."
LangString DESC_UTILS ${LANG_ENGLISH} "Open-source utilities required by the Wixel SDK: cat, cp, echo, grep, make, mv, rm, and sed."
LangString DESC_NPP ${LANG_ENGLISH} "Free source code editor which is nice to use when editing Wixel code."

Section "Wixel SDK ${SDK_VER}" SDK
	DetailPrint "Installing wixel-sdk..."
	SetOutPath "$TEMP"
	File "${STARTDIR}\pololu-wixel-sdk-${SDK_VER}.exe"
	ExecWait "$TEMP\pololu-wixel-sdk-${SDK_VER}.exe"
SectionEnd

Section "Pololu GNU Build Utilities ${UTILS_VER}" UTILS
	DetailPrint "Installing Pololu GNU Build Utilities..."
	SetOutPath "$TEMP"
	File "${STARTDIR}\pololu-gnu-build-utils-${UTILS_VER}.exe"
	#MessageBox MB_OK "The Wixel Dev Bundle will now launch the installer for the Pololu GNU Build Utilities"
	ExecWait "$TEMP\pololu-gnu-build-utils-${UTILS_VER}.exe"
SectionEnd

Section "SDCC ${SDCC_VER}" SDCC
	DetailPrint "Installing SDCC..."
	SetOutPath "$TEMP"
	File "${STARTDIR}\sdcc-${SDCC_VER}-setup.exe"
	#MessageBox MB_OK "The Wixel Development Bundle will now launch the installer for SDCC ${SDCC_VER}."
	ExecWait "$TEMP\sdcc-${SDCC_VER}-setup.exe"
	DetailPrint "Making sure that SDCC's path is set properly..."
	ReadRegStr $9 HKLM "Software\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\SDCC" 'InstallLocation'
	ReadRegStr $9 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\SDCC" 'InstallLocation'
	${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$9\bin"
SectionEnd

Section "Notepad++ v${NPP_VER}" NPP
	SetOutPath "$TEMP"
	DetailPrint "Installing Notepad++..."
	File "${STARTDIR}\npp.${NPP_VER}.Installer.exe"
	#MessageBox MB_OK "The Wixel Development Bundle will now launch the installer for Notepad++."
	ExecWait "$TEMP\npp.${NPP_VER}.Installer.exe"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SDK} $(DESC_SDK)
	!insertmacro MUI_DESCRIPTION_TEXT ${SDCC} $(DESC_SDCC)
	!insertmacro MUI_DESCRIPTION_TEXT ${UTILS} $(DESC_UTILS)
	!insertmacro MUI_DESCRIPTION_TEXT ${NPP} $(DESC_NPP)
!insertmacro MUI_FUNCTION_DESCRIPTION_END