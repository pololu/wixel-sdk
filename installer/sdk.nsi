# sdk.nsi - sub-installer script for the Wixel SDK itself

!define SDK_VER "120127"

!define STARTDIR ".\build\wixel-sdk\"
OutFile ".\build\pololu-wixel-sdk-${SDK_VER}.exe"

!include LogicLib.nsh
!include FileFunc.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin
InstallDir "C:\wixel-sdk\"
Name "Wixel SDK ${SDK_VER}"
ShowInstDetails show
ShowUninstDetails show
AllowSkipFiles on
Page directory
Page instfiles

Section "Main"
	Call SDKExists
	SetOutPath $INSTDIR
	File /r "${STARTDIR}"
	DetailPrint "PLEASE NOTE: This is NOT the latest version of the Wixel SDK."
	DetailPrint "For the latest version with all the latest apps and libraries,"
	DetailPrint "go to: https://github.com/pololu/wixel-sdk"
SectionEnd

Function SDKExists
	${if} ${FileExists} $INSTDIR
	        StrCpy $8 "$INSTDIR"
		StrCpy $INSTDIR "$INSTDIR-${SDK_VER}"
		MessageBox MB_OK "The installation folder $8 already exists.  To avoid conflicts, the new Wixel SDK files will be installed to: $INSTDIR"
	${andif} ${FileExists} $INSTDIR
		${GetTime} "" "L" $0 $1 $2 $3 $4 $5 $6
	        StrCpy $8 "$INSTDIR"
		StrCpy $INSTDIR "$INSTDIR-$2$1$0$4$5$6"
		MessageBox MB_OK "The installation folder $8 already exists.  To avoid conflicts, the new Wixel SDK files will be installed to: $INSTDIR"
	${EndIF}
FunctionEnd