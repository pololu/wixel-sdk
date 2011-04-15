####################
#
# build_tools.nsi - sub-installer script for the pololu GNU build utils
# 	designed to be run from within a larger installer
#
####################

# This installer expects to be passed the variable "STARTDIR" at the command line
# The directory you point the installer at should directly contain the executables
# E.g. "makensis /DSTARTDIR=c:\working\wixel-installer\gnu-build-tools c:\working\wixel-installer\build_tools.nsi"

!define TOOLSVER "110415"
!include EnvVarUpdate.nsh

; !define STARTDIR "c:\foo\bar"

SetCompressor /solid lzma
RequestExecutionLevel admin
OutFile "..\..\pololu_gnu_build_tools.exe"
InstallDir "$PROGRAMFILES\Pololu\GNU Build Utilities\"
Name "Pololu GNU Build Utilities"
ShowInstDetails show
ShowUninstDetails show
AllowSkipFiles on
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Main"
	SetOutPath "$INSTDIR"
	File ${STARTDIR}\*.*
	; File /r "c:\working\kalan\wixel\installer\wixel_tools\pololu_build_utilities_windows\"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities" "DisplayName" "Pololu GNU Build Utilities"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities" "UninstallString" "$\"$INSTDIR\Uninstall Pololu GNU Build Utilities.exe$\""
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities" "Publisher" "Pololu"
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities" "DisplayVersion" "${TOOLSVER}"
	WriteUninstaller "$INSTDIR\Uninstall Pololu GNU Build Utilities.exe"
	${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR"
SectionEnd

Section "Uninstall"
	Delete "$INSTDIR\cat.exe"
	Delete "$INSTDIR\cp.exe"
	Delete "$INSTDIR\echo.exe"
	Delete "$INSTDIR\grep.exe"
	Delete "$INSTDIR\license.txt"
	Delete "$INSTDIR\make.exe"
	Delete "$INSTDIR\mv.exe"
	Delete "$INSTDIR\readme.txt"
	Delete "$INSTDIR\rm.exe"
	Delete "$INSTDIR\sed.exe"
	Delete "$INSTDIR\Uninstall Pololu GNU Build Utilities.exe"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities"
	${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR"
	RMDir "$INSTDIR"
SectionEnd
