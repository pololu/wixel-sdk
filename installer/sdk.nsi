# sdk.nsi - sub-installer script for the Wixel SDK itself

!define SDK_VER "120127"

OutFile "build\pololu-wixel-sdk-${SDK_VER}.exe"

!include LogicLib.nsh
!include FileFunc.nsh

SetCompressor /solid lzma
RequestExecutionLevel admin
Name "Wixel SDK ${SDK_VER}"
ShowInstDetails show
AllowSkipFiles on
Page directory "" "" checkDirectory
Page instfiles

!macro TryInstDir path
    StrCpy $INSTDIR "${path}"
    ${unless} ${FileExists} $INSTDIR
        Return
    ${endif}
!macroend

Function .onInit
    # Choose an appropriate installation directory that does not already exist.
    !insertmacro TryInstDir "C:\wixel-sdk"
    !insertmacro TryInstDir "C:\wixel-sdk-${SDK_VER}"
    StrCpy $0 "2"
    ${do}
        !insertmacro TryInstDir "C:\wixel-sdk-${SDK_VER}-$0"
        IntOp $0 $0 + 1
    ${loop}
FunctionEnd

Function checkDirectory
    # Make sure the user didn't choose a directory that already exists, because that
    # could wipe out his previous changes to the SDK and leave him with an
    # inconsistent set of SDK files.
    ${if} ${FileExists} $INSTDIR
        MessageBox MB_OK|MB_ICONSTOP "The folder $INSTDIR already exists.  To avoid overwriting previous work, please choose a different folder that does not already exist."
        Abort
    ${endif}
FunctionEnd

Section "Main"
    SetOutPath $INSTDIR
    File /r "build\wixel-sdk"
    # TODO: put this note in a better place (e.g. use nsDialogs to put it on its own page)
    DetailPrint "PLEASE NOTE: This is NOT the latest version of the Wixel SDK."
    DetailPrint "For the latest version with all the latest apps and libraries,"
    DetailPrint "go to: https://github.com/pololu/wixel-sdk"
SectionEnd