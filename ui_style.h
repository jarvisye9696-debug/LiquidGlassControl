// Small GDI renderer: no continuous UI animation loop, GPU polling or web runtime.
HFONT headingFont{},actionFont{};HBRUSH cardBrush{};int fontLanguage=-1;
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
bool drawModernControl(DRAWITEMSTRUCT* item){
    if(!item)return false;HDC dc=item->hDC;RECT r=item->rcItem;
    if(item->CtlType==ODT_LISTBOX){int saved=SaveDC(dc);bool selected=(item->itemState&ODS_SELECTED)!=0;HBRUSH b=CreateSolidBrush(selected?RGB(228,238,255):RGB(255,255,255));FillRect(dc,&r,b);DeleteObject(b);if(item->itemID<entries.size()){auto&e=entries[item->itemID];int iconSize=MulDiv(28,dpi,96),gap=MulDiv(10,dpi,96);if(e.icon)DrawIconEx(dc,r.left+gap,r.top+(r.bottom-r.top-iconSize)/2,e.icon,iconSize,iconSize,0,nullptr,DI_NORMAL);RECT text=r;text.left+=iconSize+2*gap;text.top+=MulDiv(3,dpi,96);text.bottom=text.top+MulDiv(23,dpi,96);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(31,52,82));SelectObject(dc,actionFont);DrawTextW(dc,e.exe.c_str(),-1,&text,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);text.top+=MulDiv(21,dpi,96);text.bottom=r.bottom;SetTextColor(dc,RGB(99,116,138));SelectObject(dc,font);DrawTextW(dc,e.title.c_str(),-1,&text,DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);}if(item->itemState&ODS_FOCUS){InflateRect(&r,-2,-2);DrawFocusRect(dc,&r);}RestoreDC(dc,saved);return true;}
    bool tab=item->CtlType==ODT_TAB;if(!tab&&item->CtlType!=ODT_BUTTON)return false;int saved=SaveDC(dc);
    bool primary=tab?(int)item->itemID==(item->hwndItem==settingsTabs?settingsPage:currentPage):(item->CtlID==Apply||item->CtlID==NativeApply);
    if(!tab){if(item->CtlID==VolumeOn)primary=volumeOn;if(item->CtlID==VolumeOff)primary=!volumeOn;if(item->CtlID==BrightnessOn)primary=brightnessOn;if(item->CtlID==BrightnessOff)primary=!brightnessOn;}
    bool disabled=(item->itemState&ODS_DISABLED)!=0,pressed=(item->itemState&ODS_SELECTED)!=0,hover=GetPropW(item->hwndItem,L"LG.Hover")!=nullptr;
    HBRUSH base=CreateSolidBrush(tab?RGB(246,248,252):RGB(255,255,255));FillRect(dc,&r,base);DeleteObject(base);InflateRect(&r,-2,-2);
    COLORREF fill=primary?RGB(44,103,221):RGB(239,243,249),edge=primary?RGB(44,103,221):RGB(222,230,240);
    if(hover&&!disabled){fill=primary?RGB(31,88,198):RGB(227,235,247);edge=primary?fill:RGB(187,204,229);}if(pressed&&!disabled)fill=primary?RGB(23,72,169):RGB(215,226,241);if(disabled)fill=edge=RGB(239,241,245);
    roundFill(dc,r,fill,edge,MulDiv(14,dpi,96));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?RGB(153,164,180):primary?RGB(255,255,255):RGB(43,61,85));SelectObject(dc,actionFont?actionFont:font);
    wchar_t text[256]{};if(tab){TCITEMW entry{};entry.mask=TCIF_TEXT;entry.pszText=text;entry.cchTextMax=256;TabCtrl_GetItem(item->hwndItem,(int)item->itemID,&entry);}else GetWindowTextW(item->hwndItem,text,256);
    if(pressed)OffsetRect(&r,0,1);DrawTextW(dc,text,-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS|DT_NOPREFIX);
    if((item->itemState&ODS_FOCUS)&&!tab){InflateRect(&r,-5,-5);DrawFocusRect(dc,&r);}RestoreDC(dc,saved);return true;
}
void paintChrome(HWND h){
    PAINTSTRUCT ps;HDC output=BeginPaint(h,&ps);RECT r;GetClientRect(h,&r);if(r.right<=0||r.bottom<=0){EndPaint(h,&ps);return;}
    HDC dc=CreateCompatibleDC(output);HBITMAP bitmap=CreateCompatibleBitmap(output,r.right,r.bottom);auto old=SelectObject(dc,bitmap);FillRect(dc,&r,background);
    auto px=[](int v){return MulDiv(v,dpi,96);};RECT card={px(16),px(96),r.right-px(16),px(544)};roundFill(dc,card,RGB(255,255,255),RGB(227,233,242),px(22));
    HICON icon=(HICON)GetClassLongPtrW(h,GCLP_HICON);if(icon)DrawIconEx(dc,px(24),px(17),icon,px(32),px(32),0,nullptr,DI_NORMAL);
    BitBlt(output,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);EndPaint(h,&ps);
}
