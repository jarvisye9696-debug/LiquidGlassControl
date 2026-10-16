#pragma once
void updateEffectLabels(){
    std::wstring value=std::to_wstring(glassConfig.opacity)+L"%  ·  "+std::to_wstring(glassConfig.radius)+tr(L" 圆角",L" corner radius");
    SetWindowTextW(GetDlgItem(mainWindow,EffectValue),value.c_str());
}
bool migrationNeeded(){return explorerBackupExists()||nativeBackupExists()||shellBackupExists();}
int restoreLegacy(const std::wstring&dir){
    auto ini=dir+L"settings.ini";
    WritePrivateProfileStringW(L"Native",L"VolumeOn",L"0",ini.c_str());WritePrivateProfileStringW(L"Native",L"BrightnessOn",L"0",ini.c_str());
    if(GetFileAttributesW((dir+L"native-osd-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES||GetFileAttributesW((dir+L"native-osd-switches.bin").c_str())!=INVALID_FILE_ATTRIBUTES){int code=full_themes::nativeOperation(0,false,dir,true);if(code)return code;}
    if(GetFileAttributesW((dir+L"explorer-background-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES){int code=explorer_background::run(L"",true,dir);if(code)return code;}
    for(int i=0;i<3;++i){int code=full_themes::operation(i,false,dir);if(code)return code;}
    return shell_materials::cleanupTaskbar(dir);
}
void migrateLegacy(){
    if(migrationJob||!migrationNeeded())return;
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS;info.hwnd=mainWindow;info.lpVerb=L"runas";info.lpFile=exe;info.lpParameters=L"--legacy-restore-all";info.nShow=SW_HIDE;
    if(!ShellExecuteExW(&info)){message(tr(L"旧主题尚未还原。点击「还原」重试；恢复文件已保留。",L"Old theme not yet restored. Click Restore to retry; recovery files retained."));exitAfterRestore=false;return;}
    migrationJob=info.hProcess;updateNativeInfo();SetTimer(mainWindow,2,200,nullptr);
}
void pollMigration(){
    if(!migrationJob||WaitForSingleObject(migrationJob,0)!=WAIT_OBJECT_0)return;
    DWORD code=1;GetExitCodeProcess(migrationJob,&code);CloseHandle(migrationJob);migrationJob=nullptr;KillTimer(mainWindow,2);updateNativeInfo();
    if(code||migrationNeeded()){message(std::wstring(tr(L"旧主题恢复未完成，错误码：",L"Previous theme restore incomplete, code: "))+std::to_wstring(code));exitAfterRestore=false;return;}
    message(tr(L"旧版主题已还原，任务栏使用系统原有布局。",L"Previous theme restored. Taskbar uses the original system layout."));
    if(exitAfterRestore)DestroyWindow(mainWindow);
}
void applyGlass(bool discover){
    if(!engineReady||migrationJob||migrationNeeded()){updateNativeInfo();return;}
    glassConfig.features=(volumeOn?glass::Volume:0)|(brightnessOn?glass::Brightness:0)|(shellOn[0]?glass::Taskbar:0)|(shellOn[1]?glass::Start:0)|(shellOn[2]?glass::Notifications:0)|(explorerOn?glass::Explorer:0);
    SYSTEM_POWER_STATUS power{};GetSystemPowerStatus(&power);glassConfig.economy=batteryFriendly&&(power.ACLineStatus==0||power.SystemStatusFlag==1);
    engine.publish(glassConfig);if(discover)engine.discover();else engine.broadcast();
    observeWindows();
    WritePrivateProfileStringW(L"Glass",L"Features",std::to_wstring(glassConfig.features).c_str(),iniPath.c_str());
    WritePrivateProfileStringW(L"Glass",L"Opacity",std::to_wstring(glassConfig.opacity).c_str(),iniPath.c_str());
    WritePrivateProfileStringW(L"Glass",L"Radius",std::to_wstring(glassConfig.radius).c_str(),iniPath.c_str());
    WritePrivateProfileStringW(L"Glass",L"Tint",std::to_wstring(glassConfig.tint).c_str(),iniPath.c_str());
    WritePrivateProfileStringW(L"Glass",L"BatteryFriendly",batteryFriendly?L"1":L"0",iniPath.c_str());
    updateNativeInfo();updateEffectLabels();
    if(glassConfig.features)message(engine.sessions()?tr(L"材质请求已发送。打开对应区域查看；经典内容区可能保持不透明。",L"Material requests sent. Open the surface to check; classic content may stay opaque."):tr(L"正在等待系统界面建立材质连接；请打开对应区域。",L"Waiting for the shell material connection. Open the selected surface."));
    else message(tr(L"独立材质效果已关闭。",L"Standalone materials turned off."));
}
