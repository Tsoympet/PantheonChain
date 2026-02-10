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

; Output file
!ifndef BUILD_CONFIG
  !define BUILD_CONFIG "Release"
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

; Installer sections
Section "Core Daemon (parthenond)" SecDaemon
  SectionIn RO
  
  SetOutPath "$INSTDIR\bin"
  File "..\..\build\clients\core-daemon\${BUILD_CONFIG}\parthenond.exe"
  File "..\..\build\clients\core-daemon\${BUILD_CONFIG}\parthenond.conf"
  
  ; Create data directory
  CreateDirectory "$APPDATA\ParthenonChain"
  
  ; Create Start Menu shortcuts
  CreateDirectory "$SMPROGRAMS\${PRODUCT_NAME}"
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain Daemon.lnk" "$INSTDIR\bin\parthenond.exe"
  
SectionEnd

Section "Command Line Tools (parthenon-cli)" SecCLI
  SetOutPath "$INSTDIR\bin"
  File "..\..\build\clients\cli\${BUILD_CONFIG}\parthenon-cli.exe"
  
  ; Add to PATH
  EnVar::SetHKLM
  EnVar::AddValue "PATH" "$INSTDIR\bin"
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain CLI.lnk" "$INSTDIR\bin\parthenon-cli.exe"
SectionEnd

Section "Desktop Wallet (parthenon-qt)" SecGUI
  SetOutPath "$INSTDIR\bin"
  File "..\..\build\clients\desktop\${BUILD_CONFIG}\parthenon-qt.exe"
  
  CreateShortCut "$SMPROGRAMS\${PRODUCT_NAME}\ParthenonChain Wallet.lnk" "$INSTDIR\bin\parthenon-qt.exe"
  CreateShortCut "$DESKTOP\ParthenonChain Wallet.lnk" "$INSTDIR\bin\parthenon-qt.exe"
SectionEnd

Section "Documentation" SecDocs
  SetOutPath "$INSTDIR\docs"
  File "..\..\README.md"
  File "..\..\LICENSE"
  File "..\..\CHANGELOG.md"
  File "..\..\docs\PHASE*.md"
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
  
  ; Remove from PATH
  EnVar::SetHKLM
  EnVar::DeleteValue "PATH" "$INSTDIR\bin"
  
  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  
  SetAutoClose true
SectionEnd
