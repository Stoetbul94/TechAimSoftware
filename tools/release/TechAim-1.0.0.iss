; ─────────────────────────────────────────────────────────────────────────────
; Tech Aim Electronic Target Control 1.0.0 — production customer installer.
;
; Payload is the VERIFIED CLIENT RUNTIME, staged from
;   C:\Users\User\Downloads\TechAim-1.0.0-Client
; into dist\v1.0.0\installer-staging and audited there before compilation.
; Nothing is read from the repository, so the installer cannot pick up source,
; tests, build artefacts or internal documentation.
;
; ── INSTALL LOCATION: why this is a per-user install, not Program Files ──────
;
; The application persists three things through paths that resolve against the
; process working directory or the executable's own directory:
;
;   appsettings.cpp  saveMatch()        -> QFile("Match_<stamp>.tch")  RELATIVE
;   appsettings.cpp  AppSettings("config.ini") -> QSettings resolves it against
;                                          the working dir at construction
;   tachuswidget.cpp saveNameAndPort()  -> applicationDirPath()/USER_DETAILS
;
; Under C:\Program Files a standard (non-elevated) user cannot write to any of
; them. saveMatch() does not report the failure - the open() simply fails and
; the guarded block is skipped - so a completed match would be SILENTLY not
; saved. That is the one failure mode this product must never have.
;
; Changing those paths is a persistence change and is out of scope for
; installer work, so the installer places the application somewhere writable
; instead: {autopf} under PrivilegesRequired=lowest resolves to
; %LOCALAPPDATA%\Programs\Tech Aim. Consequences, all deliberate:
;
;   * no UAC prompt, no administrator rights needed - works on locked-down
;     range laptops, which is the deployment target
;   * .tch match records, config.ini and the remembered COM port all remain
;     writable, exactly as they are in the portable folder today
;   * the entry appears in Settings -> Apps -> Installed apps per user
;
; Session journals, reports and logs are unaffected either way: the reliability
; layer stores them under QStandardPaths::AppLocalDataLocation
; (StoragePaths::applicationDataRoot), which is already per-user and writable.
;
; ── AppId ───────────────────────────────────────────────────────────────────
; Fixed GUID so 1.0.1 / 1.1.0 upgrade this installation in place rather than
; installing a second copy. It must NOT change between Tech Aim releases, and
; it is deliberately different from the SETA installer's AppId so Tech Aim and
; SETA remain separate installable Windows products.
; ─────────────────────────────────────────────────────────────────────────────

#ifndef StageDir
  #define StageDir "..\..\dist\v1.0.0\installer-staging"
#endif
#ifndef OutDir
  #define OutDir "..\..\dist\v1.0.0"
#endif
#ifndef IconFile
  #define IconFile "..\..\images\logo\techaim.ico"
#endif
#ifndef AppVer
  #define AppVer "1.0.0"
#endif

[Setup]
AppId={{FBB1DC00-9EF6-410D-952B-B2BCF243045C}
AppName=Tech Aim Electronic Target Control
AppVersion={#AppVer}
AppVerName=Tech Aim Electronic Target Control {#AppVer}
VersionInfoVersion=1.0.0.0
VersionInfoProductName=Tech Aim Electronic Target Control
VersionInfoDescription=Tech Aim Electronic Target Control Setup
VersionInfoCompany=JAC SHOOTING SOLUTIONS (PTY) LTD
AppPublisher=JAC SHOOTING SOLUTIONS (PTY) LTD
AppCopyright=Copyright (C) JAC SHOOTING SOLUTIONS (PTY) LTD
DefaultDirName={autopf}\Tech Aim
DefaultGroupName=Tech Aim
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=TechAim-1.0.0-Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\TechAim.exe
UninstallDisplayName=Tech Aim Electronic Target Control {#AppVer}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; See the header block: the application must be able to write beside its own
; executable, so it is installed per-user and never needs elevation.
PrivilegesRequired=lowest
; If Tech Aim is running when the customer upgrades or uninstalls, ask it to
; close rather than failing on a locked Qt DLL. The application takes a
; single-instance lock, so a stale process would otherwise block a reinstall.
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Files]
; The whole staged payload - executable, Qt runtime, plugins, Qt QML modules,
; production config.ini, release manifest, customer README and the support
; collection tooling. Nothing is filtered here: the staging directory is
; audited before this file is compiled, and omitting framework files to save
; space is what breaks an installation on a PC that has never had Qt.
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Tech Aim Electronic Target Control"; Filename: "{app}\TechAim.exe"; WorkingDir: "{app}"; IconFilename: "{app}\TechAim.exe"; IconIndex: 0
Name: "{group}\Collect support logs"; Filename: "{app}\Collect-Logs.cmd"; WorkingDir: "{app}"
Name: "{group}\Uninstall Tech Aim"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Tech Aim"; Filename: "{app}\TechAim.exe"; WorkingDir: "{app}"; IconFilename: "{app}\TechAim.exe"; IconIndex: 0; Tasks: desktopicon

[Run]
Filename: "{app}\TechAim.exe"; Description: "Launch Tech Aim"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Only the directories Setup itself created and that the customer cannot have
; put anything into. Match records (.tch) written next to the executable are
; NOT listed, and Inno removes only files it installed, so a saved match
; survives an uninstall - as does everything under AppData.
Type: dirifempty; Name: "{app}"
