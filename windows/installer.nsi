; KitForge NSIS installer — Standalone app + VST3 plugin (unsigned).
;
; Preserves Release/ artefact layout so the WebView finds Resources/ui/dist:
;   $INSTDIR\Standalone\KitForge.exe
;   $INSTDIR\Resources\ui\dist\...
; VST3 is copied to the standard system folder.
;
; Driven by /D defines from scripts/release.ps1 and .github/workflows/release.yml:
;   /DKITFORGE_VERSION="0.3.0"
;   /DKITFORGE_STAGE_DIR="C:/path/to/stage"
;   /DKITFORGE_VST3_SRC="C:/path/to/KitForge_artefacts/Release/VST3"
;   /DKITFORGE_OUTPUT="C:/path/to/dist/KitForge-0.3.0-Setup.exe"

!ifndef KITFORGE_VERSION
    !define KITFORGE_VERSION "0.0.0-dev"
!endif

!ifndef KITFORGE_VERSION_NUMERIC
    !define KITFORGE_VERSION_NUMERIC "0.0.0.0"
!endif

!ifndef KITFORGE_STAGE_DIR
    !error "Pass /DKITFORGE_STAGE_DIR=... (Standalone + Resources tree)"
!endif

!ifndef KITFORGE_VST3_SRC
    !error "Pass /DKITFORGE_VST3_SRC=... (directory containing KitForge.vst3)"
!endif

!ifndef KITFORGE_OUTPUT
    !define KITFORGE_OUTPUT "KitForge-${KITFORGE_VERSION}-Setup.exe"
!endif

Unicode true
ManifestDPIAware true
SetCompressor /SOLID lzma

Name        "KitForge ${KITFORGE_VERSION}"
OutFile     "${KITFORGE_OUTPUT}"
InstallDir  "$PROGRAMFILES64\KitForge"
InstallDirRegKey HKLM "Software\KitForge" "InstallDir"
RequestExecutionLevel admin

VIProductVersion                "${KITFORGE_VERSION_NUMERIC}"
VIAddVersionKey ProductName     "KitForge"
VIAddVersionKey CompanyName     "KitForge"
VIAddVersionKey LegalCopyright  "Copyright (c) KitForge"
VIAddVersionKey FileDescription "KitForge Installer"
VIAddVersionKey FileVersion     "${KITFORGE_VERSION}"
VIAddVersionKey ProductVersion  "${KITFORGE_VERSION}"

!include "MUI2.nsh"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "KitForge application" SecCore
    SectionIn RO
    SetOutPath "$INSTDIR"
    File /r "${KITFORGE_STAGE_DIR}\*.*"

    WriteRegStr HKLM "Software\KitForge" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\KitForge" "Version"    "${KITFORGE_VERSION}"

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "DisplayName"     "KitForge"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "DisplayVersion"  "${KITFORGE_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "Publisher"       "KitForge"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "DisplayIcon"     "$INSTDIR\Standalone\KitForge.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge" \
        "NoRepair" 1

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\KitForge"
    CreateShortCut  "$SMPROGRAMS\KitForge\KitForge.lnk"   "$INSTDIR\Standalone\KitForge.exe"
    CreateShortCut  "$SMPROGRAMS\KitForge\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "VST3 plugin" SecVst3
    SectionIn RO
    SetOutPath "$COMMONFILES64\VST3"
    File /r "${KITFORGE_VST3_SRC}\KitForge.vst3"
SectionEnd

Section "Desktop shortcut" SecDesktop
    CreateShortCut "$DESKTOP\KitForge.lnk" "$INSTDIR\Standalone\KitForge.exe"
SectionEnd

Section "Uninstall"
    Delete "$DESKTOP\KitForge.lnk"
    Delete "$SMPROGRAMS\KitForge\KitForge.lnk"
    Delete "$SMPROGRAMS\KitForge\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\KitForge"

    RMDir /r "$COMMONFILES64\VST3\KitForge.vst3"
    RMDir /r "$INSTDIR"

    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\KitForge"
    DeleteRegKey HKLM "Software\KitForge"
SectionEnd
