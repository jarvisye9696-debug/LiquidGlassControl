void fitTabItems(HWND tab){
    RECT client{};GetClientRect(tab,&client);int count=TabCtrl_GetItemCount(tab);if(!count)return;
    int width=std::max(1,(int(client.right)-MulDiv(24,dpi,96))/count-MulDiv(8,dpi,96));
    // Button tabs add their own spacing. Measure the final item in device pixels.
    for(int attempt=0;attempt<16;++attempt){
        TabCtrl_SetItemSize(tab,width,MulDiv(34,dpi,96));RECT last{};TabCtrl_GetItemRect(tab,count-1,&last);
        int overflow=last.right-(client.right-MulDiv(4,dpi,96));if(overflow<=0)break;
        width=std::max(1,width-(overflow+count-1)/count-1);
    }
    InvalidateRect(tab,nullptr,TRUE);
}
void localize(){
    SetWindowTextW(mainWindow,tr(L"Liquid Glass · v0.9.1",L"Liquid Glass · v0.9.1"));
    struct Text{int id;const wchar_t*zh;const wchar_t*en;};
    const Text texts[]={
      {Header,L"Liquid Glass",L"Liquid Glass"},{Refresh,L"刷新窗口",L"Refresh windows"},{Apply,L"应用",L"Apply"},{Restore,L"还原",L"Restore"},{Pause,L"隐藏至托盘",L"Hide to tray"},
      {Note,L"选择应用后点击「应用」，点击「还原」撤销效果。\r\n资源管理器有整窗后端，其他应用的内容区仍取决于兼容性。",L"Choose an app and Apply; Restore reverts the effect.\r\nExplorer has a whole-window backend. Other app content depends on compatibility."},
      {VolumeHeading,L"音量提示",L"Volume indicator"},{BrightnessHeading,L"亮度提示",L"Brightness indicator"},
      {VolumeOn,L"On · 开启",L"On"},{VolumeOff,L"Off · 关闭",L"Off"},{BrightnessOn,L"On · 开启",L"On"},{BrightnessOff,L"Off · 关闭",L"Off"},
      {MotionEnabled,L"原生提示条轻量动画（实验性）",L"Gentle native indicator motion (experimental)"},
      {LanguageLabel,L"界面语言",L"Interface language"},{ColorThemeLabel,L"界面配色",L"Interface color"},{StartupAnimation,L"程序启动时淡入并轻轻上移",L"Fade and gently slide the app in at startup"},
      {SettingsNote,L"语言、配色和启动动画偏好自动保存。\r\n关闭设置窗口会继续在托盘运行。",L"Language, color and startup animation preferences are saved automatically.\r\nClosing settings keeps the app running in the tray."},
      {MotionText,L"程序启动\r\n180 毫秒淡入与轻移，只播放一次，结束后停止定时器。\r\n\r\n原生音量与亮度提示\r\n使用 XAML 入场与重定位过渡，触发取决于 Windows 控件生命周期。\r\n连续按键时的数值动画仍由系统控制。两种动画都遵循系统减少动画设置。",L"App startup\r\nA 180 ms fade/slide runs once, then its timer stops.\r\n\r\nNative volume and brightness\r\nXAML entrance/reposition transitions depend on the shell control lifecycle.\r\nWindows still controls numeric animation during repeated key presses.\r\nBoth animations respect the system's reduced-motion preference."},
      {CompatText,L"资源管理器：需要已安装并启用 File Explorer Styler。\r\n其他应用：外框材质可以尝试，不保证不透明内容区变玻璃。\r\n\r\n原生音量与亮度：使用 Taskbar Styler 的提示条控件，不切换任务栏主题。\r\nOn 为玻璃，单项 Off 为普通不透明背景；两项都 Off 时恢复原配置。\r\n改变原生提示条样式需要管理员授权。",L"Explorer requires installed, enabled File Explorer Styler.\r\nOther apps can try frame materials; opaque content may remain unchanged.\r\n\r\nNative indicators use Taskbar Styler without switching the taskbar theme.\r\nOn uses glass; individual Off uses an opaque background. Both Off restores originals.\r\nChanging native indicator styles requires administrator approval."},
      {AboutText,L"Liquid Glass 0.9.1\r\n原生 Windows 应用，桌面 EXE、窗口和托盘使用统一图标。\r\n\r\n窗口事件合并处理，已应用材质有缓存。界面不持续重绘。\r\n不接管媒体键、不改变实际音量与亮度，不修改任务栏位置和大小。\r\n效果尚不包含完整光学折射。",L"Liquid Glass 0.9.1\r\nNative Windows app with one icon for the EXE, window and tray.\r\n\r\nWindow events are coalesced; applied materials and app icons are cached.\r\nNo continuous idle repainting, media-key interception or actual level changes.\r\nTaskbar position/size stay intact. Full optical refraction is not implemented."},
      {ExitApp,L"还原并退出",L"Restore and exit"}
    };
    for(auto&t:texts)SetWindowTextW(GetDlgItem(mainWindow,t.id),tr(t.zh,t.en));
    const wchar_t*names[]={tr(L"应用玻璃",L"App glass"),tr(L"音量与亮度",L"Volume and brightness"),tr(L"设置",L"Settings")};
    for(int i=0;i<3;++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(names[i]);TabCtrl_SetItem(tabs,i,&item);}
    const wchar_t*subnames[]={tr(L"常规",L"General"),tr(L"动画",L"Motion"),tr(L"兼容性",L"Compatibility"),tr(L"关于",L"About")};
    for(int i=0;i<4;++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(subnames[i]);TabCtrl_SetItem(settingsTabs,i,&item);}
    int s=selection(scope),m=selection(material),c=selection(corners);
    combo(scope,{tr(L"所选应用（按进程名）",L"Selected app (by executable name)"),tr(L"资源管理器窗口",L"File Explorer windows"),tr(L"全部应用（内容区不保证）",L"All apps (content not guaranteed)")},s);
    combo(material,{L"Acrylic",L"Mica",L"Mica Alt",tr(L"关闭背景材质",L"No backdrop")},m);combo(corners,{tr(L"圆角",L"Rounded"),tr(L"小圆角",L"Small rounded"),tr(L"直角",L"Square")},c);
    combo(GetDlgItem(mainWindow,ColorTheme),{tr(L"经典蓝",L"Classic blue"),tr(L"流光紫",L"Violet"),tr(L"青绿",L"Jade"),tr(L"暖橙",L"Warm orange"),tr(L"玫瑰",L"Rose"),tr(L"湖蓝",L"Lake blue"),tr(L"石墨灰",L"Graphite")},colorTheme);
    lstrcpynW(tray.szTip,tr(L"Liquid Glass · 原生提示条",L"Liquid Glass · Native indicators"),128);if(tray.hWnd)Shell_NotifyIconW(NIM_MODIFY,&tray);
    createFonts();fitTabItems(tabs);fitTabItems(settingsTabs);refresh();updateNativeInfo();
    message(explorerJob||nativeJob?tr(L"正在处理，请稍候……",L"Working, please wait..."):tr(L"选择应用或切换分页。语言切换已移至设置 → 常规。",L"Choose an app or a page. Language is under Settings > General."));
}
HWND control(const wchar_t*cls,const wchar_t*text,DWORD style,int id,int x,int y,int w,int h){
    if(_wcsicmp(cls,L"BUTTON")==0&&((style&BS_TYPEMASK)==BS_PUSHBUTTON||(style&BS_TYPEMASK)==BS_DEFPUSHBUTTON))style=(style&~BS_TYPEMASK)|BS_OWNERDRAW;
    auto px=[](int a){return MulDiv(a,dpi,96);};HWND result=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,px(x),px(y),px(w),px(h),mainWindow,reinterpret_cast<HMENU>((INT_PTR)id),GetModuleHandleW(nullptr),nullptr);
    SendMessageW(result,WM_SETFONT,reinterpret_cast<WPARAM>(id==Header?headingFont:font),TRUE);SetWindowTheme(result,L"Explorer",nullptr);if(_wcsicmp(cls,L"BUTTON")==0&&(style&BS_TYPEMASK)==BS_OWNERDRAW)SetWindowSubclass(result,buttonHover,1,0);if(buildingPage>=0)pageControls[buildingPage].push_back(result);if(buildingSettings>=0)settingsControls[buildingSettings].push_back(result);return result;
}
void tabItem(HWND tab,int count,int){for(int i=0;i<count;++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(L" ");TabCtrl_InsertItem(tab,i,&item);}fitTabItems(tab);}
void buildUI(){
    createFonts();control(L"STATIC",L"",0,Header,68,14,450,38);
    DWORD tabStyle=WS_TABSTOP|TCS_OWNERDRAWFIXED|TCS_FIXEDWIDTH|TCS_BUTTONS|TCS_FLATBUTTONS;
    tabs=control(WC_TABCONTROLW,L"",tabStyle,Pages,24,54,742,38);tabItem(tabs,3,240);
    buildingPage=0;
    scope=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,Scope,24,104,332,180);material=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,Material,368,104,180,180);corners=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,Corners,560,104,206,150);
    list=control(L"LISTBOX",L"",LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|WS_BORDER|WS_VSCROLL|WS_TABSTOP,List,24,148,742,190);SendMessageW(list,LB_SETITEMHEIGHT,0,MulDiv(48,dpi,96));
    compatibility=control(L"STATIC",L"",0,Compatibility,24,346,742,42);
    control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,Refresh,24,398,212,36);control(L"BUTTON",L"",BS_DEFPUSHBUTTON|WS_TABSTOP,Apply,250,398,250,36);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,Restore,514,398,252,36);note=control(L"STATIC",L"",0,Note,24,454,742,82);
    buildingPage=1;
    control(L"STATIC",L"",0,VolumeHeading,32,146,170,32);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,VolumeOn,214,120,250,88);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,VolumeOff,482,120,270,88);
    control(L"STATIC",L"",0,BrightnessHeading,32,266,170,32);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,BrightnessOn,214,240,250,88);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,BrightnessOff,482,240,270,88);
    control(L"BUTTON",L"",BS_AUTOCHECKBOX|WS_TABSTOP,MotionEnabled,32,364,720,32);SendMessageW(GetDlgItem(mainWindow,MotionEnabled),BM_SETCHECK,motionEnabled?BST_CHECKED:BST_UNCHECKED,0);nativeInfo=control(L"STATIC",L"",0,NativeInfo,32,430,720,90);
    buildingPage=2;
    settingsTabs=control(WC_TABCONTROLW,L"",tabStyle,SettingsPages,32,112,720,38);tabItem(settingsTabs,4,176);
    buildingSettings=0;control(L"STATIC",L"",0,LanguageLabel,40,184,210,28);language=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP,Language,272,178,440,150);combo(language,{L"简体中文",L"English"},english?1:0);
    control(L"STATIC",L"",0,ColorThemeLabel,40,244,210,28);control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_TABSTOP|WS_VSCROLL,ColorTheme,272,238,440,260);
    control(L"BUTTON",L"",BS_AUTOCHECKBOX|WS_TABSTOP,StartupAnimation,40,304,690,34);SendMessageW(GetDlgItem(mainWindow,StartupAnimation),BM_SETCHECK,startupAnimation?BST_CHECKED:BST_UNCHECKED,0);control(L"STATIC",L"",0,SettingsNote,40,368,680,110);
    buildingSettings=1;control(L"STATIC",L"",0,MotionText,40,182,680,310);
    buildingSettings=2;control(L"STATIC",L"",0,CompatText,40,182,680,310);
    buildingSettings=3;control(L"STATIC",L"",0,AboutText,40,182,680,230);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,ExitApp,40,446,460,42);
    buildingSettings=-1;buildingPage=-1;
    status=control(L"STATIC",L"",0,Status,24,552,570,64);control(L"BUTTON",L"",BS_PUSHBUTTON|WS_TABSTOP,Pause,614,570,152,36);localize();selectPage(0);
}
