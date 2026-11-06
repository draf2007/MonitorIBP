; Установщик «Мониторинг ИБП» для Inno Setup 7.
; Сначала соберите папку dist\MonitorIBP скриптом make_dist.sh,
; затем: ISCC.exe deploy\MonitorIBP.iss  (или шаг 8 make_dist.sh сделает это сам).
; Файл должен оставаться в кодировке UTF-8.

#ifndef AppVersion
  #define AppVersion "1.1.0"
#endif
#define AppName      "Мониторинг ИБП"
#define AppExe       "MonitorIBP.exe"
#define AppPublisher "Ruselt"
#define FirewallRule "MonitorIBP SNMP traps"

[Setup]
; AppId не менять: по нему установщик узнаёт уже установленную программу при обновлении
AppId={{19318501-F769-42FE-A910-E03D0CD685F2}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\MonitorIBP
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=MonitorIBP-{#AppVersion}-setup
SetupIconFile=..\resources\icons\app.ico
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
; Программа 64-битная
SetupArchitecture=x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; По умолчанию «только для меня» (без прав администратора),
; в начале установщик спросит: для всех пользователей или только для текущего
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Закрыть запущенную программу перед обновлением или удалением
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"

[Tasks]
Name: "desktopicon"; Description: "Создать значок на рабочем столе"; Flags: unchecked
Name: "autostart"; Description: "Запускать при входе в Windows (свёрнутой в трей)"

[Files]
Source: "..\dist\MonitorIBP\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; HKA = HKLM при установке для всех, HKCU при установке для себя
Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; \
    ValueName: "MonitorIBP"; ValueData: """{app}\{#AppExe}"" --minimized"; \
    Flags: uninsdeletevalue; Tasks: autostart

[Run]
; Правило брандмауэра для приёма трапов (UDP 162). Нужны права администратора,
; поэтому только при установке для всех. Сначала удаляем старое, чтобы не плодить дубли.
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""{#FirewallRule}"""; \
    Flags: runhidden; Check: IsAdminInstallMode
Filename: "{sys}\netsh.exe"; \
    Parameters: "advfirewall firewall add rule name=""{#FirewallRule}"" dir=in action=allow protocol=UDP localport=162 program=""{app}\{#AppExe}"""; \
    Flags: runhidden; Check: IsAdminInstallMode
Filename: "{app}\{#AppExe}"; Description: "Запустить {#AppName}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{sys}\netsh.exe"; Parameters: "advfirewall firewall delete rule name=""{#FirewallRule}"""; \
    Flags: runhidden; RunOnceId: "DelFirewallRule"; Check: IsAdminInstallMode

[UninstallDelete]
; Настройки (QSettings: %LOCALAPPDATA%\Ruselt\IBP) и папка Ruselt, если опустела
Type: filesandordirs; Name: "{localappdata}\Ruselt\IBP"
Type: dirifempty;     Name: "{localappdata}\Ruselt"
; Временные файлы журнала и трапов
Type: files; Name: "{%TEMP}\ibp_log_*.txt"
Type: files; Name: "{%TEMP}\ibp_traps_*.csv"
