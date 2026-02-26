; ParthenonChain Windows Installer (NSIS Script)
; Production-grade installer for Windows 10/11

!define PRODUCT_NAME "ParthenonChain"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "ParthenonChain Foundation"
!define PRODUCT_WEB_SITE "https://parthenonchain.org"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\parthenond.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

; Modern UI
!include "MUI2.nsh"
!include "StrRep.nsh"
!include "StrFunc.nsh"
!include "LogicLib.nsh"

; Output file
!ifndef BUILD_CONFIG
  !define BUILD_CONFIG "Release"
!endif

; Allow build scripts to inject fully-resolved artifact paths.
; If not provided, fall back to discovering config-scoped and flat output layouts.
!ifndef DAEMON_BINARY_PATH
  !define DAEMON_BINARY_CONFIGURED "..\..\build\clients\core-daemon\${BUILD_CONFIG}\parthenond.exe"
  !define DAEMON_BINARY_FLAT "..\..\build\clients\core-daemon\parthenond.exe"
  !if /FileExists "${DAEMON_BINARY_CONFIGURED}"
    !define DAEMON_BINARY_PATH "${DAEMON_BINARY_CONFIGURED}"
  !else
    !if /FileExists "${DAEMON_BINARY_FLAT}"
      !define DAEMON_BINARY_PATH "${DAEMON_BINARY_FLAT}"
    !else
      !error "Missing parthenond.exe. Expected one of: ${DAEMON_BINARY_CONFIGURED} OR ${DAEMON_BINARY_FLAT}"
    !endif
  !endif
!endif

!ifndef CLI_BINARY_PATH
  !define CLI_BINARY_CONFIGURED "..\..\build\clients\cli\${BUILD_CONFIG}\parthenon-cli.exe"
  !define CLI_BINARY_FLAT "..\..\build\clients\cli\parthenon-cli.exe"
  !if /FileExists "${CLI_BINARY_CONFIGURED}"
    !define CLI_BINARY_PATH "${CLI_BINARY_CONFIGURED}"
  !else
    !if /FileExists "${CLI_BINARY_FLAT}"
      !define CLI_BINARY_PATH "${CLI_BINARY_FLAT}"
    !else
      !error "Missing parthenon-cli.exe. Expected one of: ${CLI_BINARY_CONFIGURED} OR ${CLI_BINARY_FLAT}"
    !endif
  !endif
!endif

!ifndef DESKTOP_BINARY_PATH
  !define DESKTOP_BINARY_CONFIGURED "..\..\build\clients\desktop\${BUILD_CONFIG}\parthenon-qt.exe"
  !define DESKTOP_BINARY_FLAT "..\..\build\clients\desktop\parthenon-qt.exe"
  !if /FileExists "${DESKTOP_BINARY_CONFIGURED}"
    !define DESKTOP_BINARY_PATH "${DESKTOP_BINARY_CONFIGURED}"
  !else
    !if /FileExists "${DESKTOP_BINARY_FLAT}"
      !define DESKTOP_BINARY_PATH "${DESKTOP_BINARY_FLAT}"
    !else
      !error "Missing parthenon-qt.exe. Expected one of: ${DESKTOP_BINARY_CONFIGURED} OR ${DESKTOP_BINARY_FLAT}"
    !endif
  !endif
!endif

OutFile "parthenon-${PRODUCT_VERSION}-windows-x64-setup.exe"

; Install directory
InstallDir "$PROGRAMFILES64\${PRODUCT_NAME}"

; Get install folder from registry if available
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""

; Request application privileges
RequestExecutionLevel admin

; Interface settings
!define MUI_ABORTWARNING
; Custom icons - Update these paths when icon files are generated
; !define MUI_ICON "..\..\clients\desktop\assets\icon.ico"
; !define MUI_UNICON "..\..\clients\desktop\assets\icon.ico"
; Temporary: use default NSIS icons until custom icons are generated
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Pages
!insertmacro MUI_PAGE_LICENSE "..\..\EULA.md"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; Declare string-replacement functions used for PATH manipulation (no plugin required)
${StrStr}
${StrRep}
${un.StrRep}

; Installer sections
Section "Core Daemon (parthenond)" SecDaemon
  SectionIn RO
  
  SetOutPath "$INSTDIR\bin"
  File "${DAEMON_BINARY_PATH}"
  File "..\..\build\clients\core-daemon\parthenond.conf"
  
  ; Create data directory
  CreateDirectory "$APPDATA\ParthenonChain"
  
  ; Create Start Menu shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain Daemon.lnk" "$INSTDIR\bin\parthenond.exe"
  
SectionEnd

Section "Command Line Tools (parthenon-cli)" SecCLI
  SetOutPath "$INSTDIR\bin"
  File "${CLI_BINARY_PATH}"
  
  ; Add to system PATH only if not already present (native registry, no plugin required)
  ReadRegStr $R0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  ${StrStr} $R1 "$R0" "$INSTDIR\bin"
  ${If} $R1 == ""
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$R0;$INSTDIR\bin"
    SendMessage 0xFFFF 0x001A 0 "STR:Environment" /TIMEOUT=5000
  ${EndIf}
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain CLI.lnk" "$INSTDIR\bin\parthenon-cli.exe"
SectionEnd

Section "Desktop Wallet (parthenon-qt)" SecGUI
  SetOutPath "$INSTDIR\bin"
  File "${DESKTOP_BINARY_PATH}"
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain Wallet.lnk" "$INSTDIR\bin\parthenon-qt.exe"
  CreateShortCut "$DESKTOP\ParthenonChain Wallet.lnk" "$INSTDIR\bin\parthenon-qt.exe"
SectionEnd

Section "Documentation" SecDocs
  SetOutPath "$INSTDIR\docs"
  File "..\..\README.md"
  File "..\..\LICENSE"
  File "..\..\CHANGELOG.md"
  File "..\..\docs\INSTALLATION.md"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\bin\parthenond.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\bin\parthenond.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDaemon} "The full node daemon (required)"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecCLI} "Command line tools for RPC interaction"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecGUI} "Desktop wallet with graphical interface"
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDocs} "Documentation and release notes"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; Uninstaller
Section Uninstall
  Delete "$INSTDIR\bin\parthenond.exe"
  Delete "$INSTDIR\bin\parthenond.conf"
  Delete "$INSTDIR\bin\parthenon-cli.exe"
  Delete "$INSTDIR\bin\parthenon-qt.exe"
  Delete "$INSTDIR\docs\*.*"
  Delete "$INSTDIR\uninst.exe"
  
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\docs"
  RMDir "$INSTDIR"
  
  Delete "$SMPROGRAMS\${PRODUCT_NAME}\*.*"
  RMDir "$SMPROGRAMS\${PRODUCT_NAME}"
  Delete "$DESKTOP\ParthenonChain Wallet.lnk"
  
  ; Remove $INSTDIR\bin from system PATH (semicolon-delimited to avoid partial matches)
  ReadRegStr $R8 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
  StrCpy $R8 "$R8;"  ; normalize: append ; so every entry is followed by ;
  ${un.StrRep} $R9 "$R8" ";$INSTDIR\bin;" ";"  ; remove entry from middle or end
  ${un.StrRep} $R9 "$R9" "$INSTDIR\bin;" ""    ; remove entry from start of PATH
  StrLen $R8 "$R9"
  ${If} $R8 > 0
    StrCpy $R7 "$R9" 1 -1  ; read last character
    ${If} $R7 == ";"
      IntOp $R8 $R8 - 1
      StrCpy $R9 "$R9" $R8  ; strip trailing ;
    ${EndIf}
  ${EndIf}
  WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$R9"
  SendMessage 0xFFFF 0x001A 0 "STR:Environment" /TIMEOUT=5000
  
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  SetAutoClose true
SectionEnd
