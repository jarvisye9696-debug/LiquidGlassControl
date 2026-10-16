// Small GDI renderer: no continuous UI animation loop, GPU polling or web runtime.
HFONT headingFont{},actionFont{};HBRUSH cardBrush{};int fontLanguage=-1;
int colorTheme=0;
struct UiPalette { COLORREF accent,hover,pressed,selection,canvas,surface,edge; };
const UiPalette palettes[]={
    {RGB(44,103,221),RGB(31,88,198),RGB(23,72,169),RGB(228,238,255),RGB(246,248,252),RGB(239,243,249),RGB(222,230,240)},
    {RGB(112,73,218),RGB(94,56,194),RGB(76,42,166),RGB(239,232,255),RGB(249,247,254),RGB(243,239,251),RGB(229,220,244)},
    {RGB(23,137,122),RGB(16,116,104),RGB(12,94,85),RGB(220,244,237),RGB(245,250,248),RGB(234,245,241),RGB(211,232,224)},
    {RGB(206,92,32),RGB(180,74,23),RGB(152,60,17),RGB(255,235,218),RGB(253,249,245),RGB(251,240,230),RGB(239,222,207)},
    {RGB(188,64,114),RGB(164,48,96),RGB(140,36,80),RGB(252,228,239),RGB(253,247,250),RGB(250,237,243),RGB(239,217,228)},
    {RGB(14,125,175),RGB(9,105,151),RGB(6,85,125),RGB(221,242,252),RGB(245,250,253),RGB(232,244,250),RGB(209,230,240)},
    {RGB(83,99,121),RGB(66,81,103),RGB(50,64,84),RGB(231,235,241),RGB(247,248,250),RGB(238,241,245),RGB(220,226,234)}
};
const UiPalette& palette(){return palettes[colorTheme];}
COLORREF tint(COLORREF color,int amount){return RGB((GetRValue(color)*amount+255*(100-amount))/100,(GetGValue(color)*amount+255*(100-amount))/100,(GetBValue(color)*amount+255*(100-amount))/100);}
COLORREF cardColor(){return tint(palette().accent,9);}
COLORREF textColor(){return RGB(GetRValue(palette().pressed)/3,GetGValue(palette().pressed)/3,GetBValue(palette().pressed)/3);}
void themeWindow(HWND h){
    COLORREF caption=palette().selection,text=textColor();
    DwmSetWindowAttribute(h,35,&caption,sizeof(caption));
    DwmSetWindowAttribute(h,36,&text,sizeof(text));
}
void applyColorTheme(int index,bool save){
    colorTheme=index>=0&&index<7?index:0;
    HBRUSH old=background;background=CreateSolidBrush(tint(palette().accent,18));
    HBRUSH oldCard=cardBrush;cardBrush=CreateSolidBrush(cardColor());if(oldCard)DeleteObject(oldCard);
    if(mainWindow){themeWindow(mainWindow);SetClassLongPtrW(mainWindow,GCLP_HBRBACKGROUND,reinterpret_cast<LONG_PTR>(background));RedrawWindow(mainWindow,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_FRAME);}
    if(old)DeleteObject(old);
    if(save){auto value=std::to_wstring(colorTheme);WritePrivateProfileStringW(L"UI",L"ColorTheme",value.c_str(),iniPath.c_str());}
}
void createFonts(){
    if(fontLanguage==(english?1:0)&&font)return;
    HFONT old=font,oldHeading=headingFont,oldAction=actionFont;
    const wchar_t*face=english?L"Segoe UI":L"Microsoft YaHei UI";
    auto make=[&](int size,int weight){return CreateFontW(-MulDiv(size,dpi,96),0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,face);};
    font=make(15,FW_NORMAL);headingFont=make(23,FW_SEMIBOLD);actionFont=make(15,FW_SEMIBOLD);fontLanguage=english?1:0;
    if(!cardBrush)cardBrush=CreateSolidBrush(RGB(255,255,255));
    EnumChildWindows(mainWindow,[](HWND child,LPARAM)->BOOL{int id=GetDlgCtrlID(child);const wchar_t*button=L"Button";wchar_t cls[32]{};GetClassNameW(child,cls,32);HFONT f=id==Header?headingFont:_wcsicmp(cls,button)==0?actionFont:font;SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(f),TRUE);return TRUE;},0);
    if(old)DeleteObject(old);if(oldHeading)DeleteObject(oldHeading);if(oldAction)DeleteObject(oldAction);
}
void destroyFonts(){if(font)DeleteObject(font);if(headingFont)DeleteObject(headingFont);if(actionFont)DeleteObject(actionFont);if(cardBrush)DeleteObject(cardBrush);font=headingFont=actionFont=nullptr;cardBrush=nullptr;}
LRESULT CALLBACK buttonHover(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
    if(m==WM_MOUSEMOVE&&!GetPropW(h,L"LG.Hover")){SetPropW(h,L"LG.Hover",(HANDLE)1);TRACKMOUSEEVENT tracking{};tracking.cbSize=sizeof(tracking);tracking.dwFlags=TME_LEAVE;tracking.hwndTrack=h;TrackMouseEvent(&tracking);InvalidateRect(h,nullptr,FALSE);}
    if(m==WM_MOUSELEAVE){RemovePropW(h,L"LG.Hover");InvalidateRect(h,nullptr,FALSE);}
    if(m==WM_NCDESTROY)RemovePropW(h,L"LG.Hover");
    return DefSubclassProc(h,m,w,l);
}
void roundFill(HDC dc,RECT r,COLORREF fill,COLORREF edge,int radius){
    HBRUSH b=CreateSolidBrush(fill);HPEN p=CreatePen(PS_SOLID,1,edge);auto oldBrush=SelectObject(dc,b),oldPen=SelectObject(dc,p);RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(b);DeleteObject(p);
}
LRESULT CALLBACK compactSwitch(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR){
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){
        PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r{};GetClientRect(h,&r);FillRect(dc,&r,cardBrush);
        auto px=[](int n){return MulDiv(n,dpi,96);};bool on=SendMessageW(h,BM_GETCHECK,0,0)==BST_CHECKED,active=IsWindowEnabled(h);
        RECT track={px(2),px(4),px(54),px(32)};COLORREF fill=!active?RGB(200,206,215):on?palette().accent:RGB(155,165,181);
        roundFill(dc,track,fill,fill,px(28));HBRUSH thumb=CreateSolidBrush(RGB(255,255,255));auto oldBrush=SelectObject(dc,thumb);auto oldPen=SelectObject(dc,GetStockObject(NULL_PEN));int x=on?px(29):px(5);Ellipse(dc,x,px(7),x+px(22),px(29));SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(thumb);
        RECT caption=r;caption.left=px(62);SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,active?textColor():RGB(134,143,157));DrawTextW(dc,on?tr(L"开",L"On"):tr(L"关",L"Off"),-1,&caption,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
        if(GetFocus()==h){InflateRect(&r,-1,-1);DrawFocusRect(dc,&r);}EndPaint(h,&ps);return 0;
    }
    auto result=DefSubclassProc(h,m,w,l);
    if(m==BM_SETCHECK||m==WM_ENABLE||m==WM_SETFOCUS||m==WM_KILLFOCUS||m==WM_KEYUP||m==WM_LBUTTONUP)InvalidateRect(h,nullptr,FALSE);
    return result;
}
LRESULT CALLBACK themedControl(HWND h,UINT m,WPARAM w,LPARAM l,UINT_PTR,DWORD_PTR comboControl){
    if(m==WM_ERASEBKGND){RECT r;GetClientRect(h,&r);FillRect(reinterpret_cast<HDC>(w),&r,background);return 1;}
    if(m==WM_PAINT&&comboControl){
        PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);
        roundFill(dc,r,cardColor(),palette().edge,MulDiv(8,dpi,96));
        RECT text=r;text.left+=MulDiv(8,dpi,96);text.right-=MulDiv(25,dpi,96);
        wchar_t label[256]{};GetWindowTextW(h,label,256);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,textColor());auto oldFont=SelectObject(dc,font);
        DrawTextW(dc,label,-1,&text,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        RECT arrow=r;arrow.left=arrow.right-MulDiv(24,dpi,96);DrawTextW(dc,L"⌄",-1,&arrow,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
        if(GetFocus()==h){InflateRect(&text,-2,-2);DrawFocusRect(dc,&text);}SelectObject(dc,oldFont);EndPaint(h,&ps);return 0;
    }
    auto result=DefSubclassProc(h,m,w,l);
    if(comboControl&&(m==WM_SETFOCUS||m==WM_KILLFOCUS||m==CB_SETCURSEL||m==WM_ENABLE))InvalidateRect(h,nullptr,FALSE);
    return result;
}
bool drawModernControl(DRAWITEMSTRUCT* item){
    if(!item)return false;HDC dc=item->hDC;RECT r=item->rcItem;
    if(item->CtlType==ODT_COMBOBOX){int saved=SaveDC(dc);bool selected=(item->itemState&ODS_SELECTED)!=0;HBRUSH brush=CreateSolidBrush(selected?palette().selection:cardColor());FillRect(dc,&r,brush);DeleteObject(brush);if(item->itemID!=static_cast<UINT>(-1)){wchar_t label[512]{};SendMessageW(item->hwndItem,CB_GETLBTEXT,item->itemID,reinterpret_cast<LPARAM>(label));r.left+=MulDiv(8,dpi,96);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,textColor());SelectObject(dc,font);DrawTextW(dc,label,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);}RestoreDC(dc,saved);return true;}
    if(item->CtlType==ODT_LISTBOX){int saved=SaveDC(dc);bool selected=(item->itemState&ODS_SELECTED)!=0;HBRUSH b=CreateSolidBrush(selected?palette().selection:cardColor());FillRect(dc,&r,b);DeleteObject(b);if(item->itemID<entries.size()){auto&e=entries[item->itemID];int iconSize=MulDiv(28,dpi,96),gap=MulDiv(10,dpi,96);if(e.icon)DrawIconEx(dc,r.left+gap,r.top+(r.bottom-r.top-iconSize)/2,e.icon,iconSize,iconSize,0,nullptr,DI_NORMAL);RECT text=r;text.left+=iconSize+2*gap;text.top+=MulDiv(3,dpi,96);text.bottom=text.top+MulDiv(23,dpi,96);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,textColor());SelectObject(dc,actionFont);DrawTextW(dc,e.exe.c_str(),-1,&text,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);text.top+=MulDiv(21,dpi,96);text.bottom=r.bottom;SetTextColor(dc,palette().pressed);SelectObject(dc,font);DrawTextW(dc,e.title.c_str(),-1,&text,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);}if(item->itemState&ODS_FOCUS){InflateRect(&r,-2,-2);DrawFocusRect(dc,&r);}RestoreDC(dc,saved);return true;}
    bool tab=item->CtlType==ODT_TAB;if(!tab&&item->CtlType!=ODT_BUTTON)return false;int saved=SaveDC(dc);
    bool primary=tab?(int)item->itemID==(item->hwndItem==settingsTabs?settingsPage:currentPage):item->CtlID==Apply;
    if(!tab){if(item->CtlID==VolumeOn)primary=volumeOn;if(item->CtlID==VolumeOff)primary=!volumeOn;if(item->CtlID==BrightnessOn)primary=brightnessOn;if(item->CtlID==BrightnessOff)primary=!brightnessOn;}
    bool disabled=(item->itemState&ODS_DISABLED)!=0,pressed=(item->itemState&ODS_SELECTED)!=0,hover=GetPropW(item->hwndItem,L"LG.Hover")!=nullptr;
    HBRUSH base=CreateSolidBrush(tab?tint(palette().accent,18):cardColor());FillRect(dc,&r,base);DeleteObject(base);InflateRect(&r,-2,-2);
    COLORREF fill=primary?palette().accent:palette().surface,edge=primary?palette().accent:palette().edge;
    if(hover&&!disabled){fill=primary?palette().hover:palette().selection;edge=primary?fill:palette().accent;}if(pressed&&!disabled)fill=primary?palette().pressed:palette().selection;if(disabled)fill=edge=RGB(239,241,245);
    roundFill(dc,r,fill,edge,MulDiv(14,dpi,96));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?RGB(153,164,180):primary?RGB(255,255,255):textColor());SelectObject(dc,actionFont?actionFont:font);
    wchar_t text[256]{};if(tab){TCITEMW entry{};entry.mask=TCIF_TEXT;entry.pszText=text;entry.cchTextMax=256;TabCtrl_GetItem(item->hwndItem,(int)item->itemID,&entry);}else GetWindowTextW(item->hwndItem,text,256);
    if(pressed)OffsetRect(&r,0,1);DrawTextW(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    if((item->itemState&ODS_FOCUS)&&!tab){InflateRect(&r,-5,-5);DrawFocusRect(dc,&r);}RestoreDC(dc,saved);return true;
}
void paintChrome(HWND h){
    PAINTSTRUCT ps;HDC output=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);if(r.right<=0||r.bottom<=0){EndPaint(h,&ps);return;}
    HDC dc=CreateCompatibleDC(output);HBITMAP bitmap=CreateCompatibleBitmap(output,r.right,r.bottom);auto old=SelectObject(dc,bitmap);FillRect(dc,&r,background);
    auto px=[](int v){return MulDiv(v,dpi,96);};RECT card={px(16),px(96),r.right-px(16),px(544)};roundFill(dc,card,cardColor(),palette().edge,px(22));
    HICON icon=(HICON)GetClassLongPtrW(h,GCLP_HICON);if(icon)DrawIconEx(dc,px(24),px(17),icon,px(32),px(32),0,nullptr,DI_NORMAL);
    BitBlt(output,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);EndPaint(h,&ps);
}
