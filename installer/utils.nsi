# build_tools.nsi - sub-installer script for the pololu GNU build utils
# 	designed to be run from within a larger installer

!define UTILS_VER "120126"

!define STARTDIR ".\build\pololu-gnu-build-utils-${UTILS_VER}"
OutFile ".\build\pololu-gnu-build-utils-${UTILS_VER}.exe"

!include EnvVarUpdate.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin
InstallDir "$PROGRAMFILES\Pololu\GNU Build Utilities\"
Name "Pololu GNU Build Utilities ${UTILS_VER}"
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
	WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities" "DisplayVersion" "${UTILS_VER}"
	WriteUninstaller "$INSTDIR\Uninstall Pololu GNU Build Utilities.exe"
	${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR"
SectionEnd

Section "Uninstall"
	Delete "$INSTDIR\cat.exe"
	Delete "$INSTDIR\cp.exe"
	Delete "$INSTDIR\echo.exe"
	Delete "$INSTDIR\grep.exe"
	Delete "$INSTDIR\LICENSE.txt"
	Delete "$INSTDIR\make.exe"
	Delete "$INSTDIR\mv.exe"
	Delete "$INSTDIR\README.txt"
	Delete "$INSTDIR\rm.exe"
	Delete "$INSTDIR\sed.exe"
	Delete "$INSTDIR\Uninstall Pololu GNU Build Utilities.exe"
	DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\pololu_build_utilities"
	${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR"
	RMDir "$INSTDIR"
SectionEnd
