// Included inside main.cpp's application namespace. All model mutation is on
// the UI thread; the worker receives a snapshot of selected identities.
constexpr UINT devicesId=106;
constexpr int listId=201,enabledId=202,notifyId=203,nameId=204,badgeId=205,saveId=206,forgetId=207,refreshDeviceId=208,extendedId=209,colorId=210;
bool auxPending=false,refreshQueued=false;
bool blinkTimerActive=false,blinkDim=false;
std::vector<ob::Device> devices,auxResult;
std::vector<std::wstring> pollSelected,pollExtended;
bool pollAux=true,pollAj=true;
std::wstring requestedDevice;
struct TrayEntry {std::wstring id;UINT number;HICON icon=nullptr;std::wstring signature;bool numeric=false;HICON dimIcon=nullptr;};
std::vector<TrayEntry> trays;
UINT nextTray=2;
HWND devicesWindow=nullptr,deviceList=nullptr,detailsLabel=nullptr,nameEdit=nullptr,badgeEdit=nullptr,enabledCheck=nullptr,notifyCheck=nullptr,extendedCheck=nullptr,colorButton=nullptr;
HFONT uiFont=nullptr;
UINT devicesDpi=96;
bool fillingDevices=false;
void refresh();
void updateAllIcons(bool add=false);
void refreshDeviceWindow();
void refreshOne(const std::wstring& id){requestedDevice=id;refresh();}
std::uint64_t fileTimeNow(){FILETIME ft;GetSystemTimeAsFileTime(&ft);return (std::uint64_t(ft.dwHighDateTime)<<32)|ft.dwLowDateTime;}
std::wstring timeText(std::uint64_t time){
    if(!time)return L"Never";FILETIME ft{DWORD(time),DWORD(time>>32)},local{};SYSTEMTIME st{};
    FileTimeToLocalFileTime(&ft,&local);FileTimeToSystemTime(&local,&st);
    wchar_t s[64];swprintf(s,64,L"%04u-%02u-%02u %02u:%02u:%02u",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute,st.wSecond);return s;
}
ob::Device* getDevice(const std::wstring& id){for(auto& d:devices)if(d.id==id)return &d;return nullptr;}
std::wstring selectedDeviceId(){
    if(!deviceList)return L"";LRESULT i=SendMessageW(deviceList,LB_GETCURSEL,0,0);
    return i>=0 && std::size_t(i)<devices.size()?devices[i].id:L"";
}
std::wstring deviceDescription(const ob::Device& d){
    std::wstring s=d.name()+(d.badge?L" ["+std::to_wstring(d.badge)+L"]":L"")+L" — ";
    if(d.reading.state==ob::State::reading)s+=(d.reading.approximate?L"Approx. ":L"")+std::to_wstring(d.reading.percent)+L"%";
    else s+=ob::stateText(d.reading.state);
    if(d.reading.chargingKnown && d.reading.charging)s+=L" · Charging";
    if(d.lastRead){s+=d.reading.state==ob::State::reading?L"\nChecked: ":L"\nLast: "+std::wstring(d.lastApproximate?L"approx. ":L"")+std::to_wstring(d.lastPercent)+L"% at ";s+=timeText(d.lastRead);}
    return s;
}
NOTIFYICONDATAW deviceTrayData(UINT number){auto data=trayData();data.uID=number;return data;}
void removeTray(TrayEntry& t){auto d=deviceTrayData(t.number);Shell_NotifyIconW(NIM_DELETE,&d);if(t.icon)DestroyIcon(t.icon);if(t.dimIcon)DestroyIcon(t.dimIcon);t.icon=nullptr;t.dimIcon=nullptr;}
void syncBlinkTimer(){
    bool needed=false;for(const auto& d:devices)needed|=d.shouldBlink();
    if(needed && !blinkTimerActive){blinkDim=false;blinkTimerActive=SetTimer(window,5,1000,nullptr)!=0;}
    else if(!needed){if(blinkTimerActive)KillTimer(window,5);blinkTimerActive=false;blinkDim=false;}
}
void updateBlinkFrames(){
    for(const auto& t:trays){auto device=getDevice(t.id);if(!device || !device->shouldBlink() || !t.dimIcon)continue;
        auto data=deviceTrayData(t.number);data.uFlags=NIF_ICON;data.hIcon=blinkDim?t.dimIcon:t.icon;Shell_NotifyIconW(NIM_MODIFY,&data);
    }
}
void updateAllIcons(bool add){
    if(!multiMode)return;
    auto aj=getDevice(L"aj179");if(aj)aj->reading=current;
    std::vector<std::wstring> visible;for(const auto& d:devices)if(d.enabled)visible.push_back(d.id);
    if(visible.empty())visible.push_back(L"app");
    syncBlinkTimer();
    for(auto it=trays.begin();it!=trays.end();){
        if(std::find(visible.begin(),visible.end(),it->id)==visible.end() || it->numeric){removeTray(*it);it=trays.erase(it);}else ++it;
    }
    for(const auto& id:visible){
        auto it=std::find_if(trays.begin(),trays.end(),[&](const TrayEntry& t){return t.id==id;});
        bool create=it==trays.end();if(create){trays.push_back({id,nextTray++,nullptr,L"",false});it=trays.end()-1;}
        int size=std::max(16,GetSystemMetrics(SM_CXSMICON));const auto* device=getDevice(id);
        std::wstring signature=std::to_wstring(size)+L":"+std::to_wstring(darkTheme());
        if(device)signature+=L":"+std::to_wstring(int(device->reading.state))+L":"+std::to_wstring(device->reading.percent)+L":"+std::to_wstring(device->identityColor())+L":"+std::to_wstring(device->shouldBlink())+L":"+std::to_wstring(device->reading.chargingKnown && device->reading.charging);
        bool redraw=!it->icon || signature!=it->signature;HICON next=it->icon;
        HICON dim=it->dimIcon;
        if(redraw){next=device?ob::ringIcon(*device,size,darkTheme()):ob::loadArtwork(ob::Artwork::app,size,darkTheme());dim=device && device->shouldBlink()?ob::ringIcon(*device,size,darkTheme(),true):nullptr;}
        if(!next)continue;
        auto data=deviceTrayData(it->number);data.uFlags=NIF_ICON|NIF_TIP|NIF_MESSAGE|NIF_SHOWTIP;data.hIcon=device && device->shouldBlink() && blinkDim && dim?dim:next;data.uCallbackMessage=trayMessage;
        std::wstring tip=device?deviceDescription(*device):L"Open Battery — click to choose devices";wcsncpy(data.szTip,tip.c_str(),127);data.szTip[127]=0;
        if(!Shell_NotifyIconW(create||add?NIM_ADD:NIM_MODIFY,&data) && !create && !add)Shell_NotifyIconW(NIM_ADD,&data);
        if(create||add){data.uVersion=NOTIFYICON_VERSION_4;Shell_NotifyIconW(NIM_SETVERSION,&data);}
        if(redraw){if(it->icon)DestroyIcon(it->icon);if(it->dimIcon)DestroyIcon(it->dimIcon);it->icon=next;it->dimIcon=dim;it->signature=signature;}
    }
}
void receiveOtherDevices(){
    if(!auxPending)return;
    std::wstring selected=selectedDeviceId();
    ob::mergeDevices(devices,auxResult,fileTimeNow());
    for(auto& d:devices)if(!d.configured){ob::loadDevice(d);}
    auxResult.clear();auxPending=false;
    if(deviceList){SendMessageW(deviceList,LB_RESETCONTENT,0,0);
        for(const auto& d:devices)SendMessageW(deviceList,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(d.name().c_str()));
        auto it=std::find_if(devices.begin(),devices.end(),[&](const ob::Device& d){return d.id==selected;});SendMessageW(deviceList,LB_SETCURSEL,it==devices.end()?0:it-devices.begin(),0);}
    for(auto& d:devices){if(d.id!=L"aj179"){auto key=ob::storageId(d.id);std::string label(key.begin(),key.end());logReading(d.reading,label.c_str());}}refreshDeviceWindow();
}
void initializeDevices(){
    devices=ob::rememberedDevices();
    if(!getDevice(L"aj179")){
        ob::Device d;d.id=L"aj179";d.defaultName=L"AJ179 Pro";d.kind=ob::DeviceKind::mouse;d.connection=L"2.4 GHz";d.source=L"AJ179 receiver status";
        d.enabled=true;d.alert.warned=alert.warned;d.configured=true;ob::loadDevice(d);devices.insert(devices.begin(),d);
    }
    if(auto aj=getDevice(L"aj179")){aj->kind=ob::DeviceKind::mouse;aj->connection=L"2.4 GHz";aj->source=L"AJ179 receiver status";}
}
void refreshDeviceWindow(){
    if(!devicesWindow)return;
    auto d=getDevice(selectedDeviceId());fillingDevices=true;
    EnableWindow(colorButton,d!=nullptr);InvalidateRect(colorButton,nullptr,TRUE);
    EnableWindow(extendedCheck,d && d->kind==ob::DeviceKind::controller);
    EnableWindow(enabledCheck,d!=nullptr);EnableWindow(notifyCheck,d!=nullptr);EnableWindow(nameEdit,d!=nullptr);EnableWindow(badgeEdit,d!=nullptr);
    if(d){
        std::wstring detail=std::wstring(ob::kindText(d->kind))+L" · "+d->connection+L"\r\n"+deviceDescription(*d)+L"\r\nCharging: "+
            (d->reading.chargingKnown?(d->reading.charging?L"Yes":L"No"):L"Unknown");
        if(!d->source.empty())detail+=L"\r\nSource: "+d->source;
        if(d->reading.error)detail+=L"\r\nError: "+std::to_wstring(d->reading.error);
        SetWindowTextW(detailsLabel,detail.c_str());if(GetFocus()!=nameEdit && GetFocus()!=badgeEdit){SetWindowTextW(nameEdit,d->alias.c_str());SetWindowTextW(badgeEdit,std::to_wstring(d->badge).c_str());}
        SendMessageW(extendedCheck,BM_SETCHECK,d->extendedReports?BST_CHECKED:BST_UNCHECKED,0);
        SendMessageW(enabledCheck,BM_SETCHECK,d->enabled?BST_CHECKED:BST_UNCHECKED,0);SendMessageW(notifyCheck,BM_SETCHECK,d->blinkLow?BST_CHECKED:BST_UNCHECKED,0);
    }else {SetWindowTextW(detailsLabel,L"No devices found. Connect a paired device and choose Refresh.");SetWindowTextW(nameEdit,L"");SetWindowTextW(badgeEdit,L"0");}
    fillingDevices=false;
}
HWND control(HWND parent,const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int w,int h,int id=0){
    HWND c=CreateWindowExW(cls==std::wstring(L"EDIT")?WS_EX_CLIENTEDGE:0,cls,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,parent,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
    if(uiFont)SendMessageW(c,WM_SETFONT,reinterpret_cast<WPARAM>(uiFont),TRUE);return c;
}
LRESULT CALLBACK deviceWindowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
    switch(msg){
    case WM_CREATE:{
        UINT dpi=GetDpiForWindow(h);if(!dpi)dpi=96;devicesDpi=dpi;
        uiFont=CreateFontW(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        // Coordinates are DPI scaled once, with no background layout timer.
        auto c=[&](const wchar_t* cls,const wchar_t* text,DWORD style,int x,int y,int width,int height,int id=0){return control(h,cls,text,style,MulDiv(x,dpi,96),MulDiv(y,dpi,96),MulDiv(width,dpi,96),MulDiv(height,dpi,96),id);};
        c(L"STATIC",L"Choose devices to show in the system tray",0,18,14,580,24);
        deviceList=c(L"LISTBOX",L"",WS_TABSTOP|WS_BORDER|WS_VSCROLL|LBS_NOTIFY|LBS_NOINTEGRALHEIGHT,18,48,206,330,listId);
        detailsLabel=c(L"STATIC",L"",0,242,48,388,176);
        enabledCheck=c(L"BUTTON",L"Show in tray",WS_TABSTOP|BS_AUTOCHECKBOX,242,228,360,24,enabledId);
        notifyCheck=c(L"BUTTON",L"Blink at 20% or less",WS_TABSTOP|BS_AUTOCHECKBOX,242,258,360,24,notifyId);
        extendedCheck=c(L"BUTTON",L"DualShock 4: enable full Bluetooth reports",WS_TABSTOP|BS_AUTOCHECKBOX,242,288,390,24,extendedId);
        c(L"STATIC",L"Custom name (blank = default)",0,242,324,270,20);nameEdit=c(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,242,348,250,26,nameId);SendMessageW(nameEdit,EM_SETLIMITTEXT,48,0);
        c(L"STATIC",L"Number (tooltip)",0,506,324,130,20);badgeEdit=c(L"EDIT",L"0",WS_TABSTOP|ES_NUMBER,506,348,110,26,badgeId);SendMessageW(badgeEdit,EM_SETLIMITTEXT,1,0);
        c(L"BUTTON",L"Save",WS_TABSTOP|BS_DEFPUSHBUTTON,242,384,108,28,saveId);
        c(L"BUTTON",L"Forget device",WS_TABSTOP,362,384,120,28,forgetId);
        c(L"BUTTON",L"Refresh",WS_TABSTOP,494,384,122,28,refreshDeviceId);
        colorButton=c(L"BUTTON",L"Ring color...",WS_TABSTOP|BS_OWNERDRAW,242,420,260,28,colorId);
        c(L"STATIC",L"Forget removes Open Battery settings only. Bluetooth pairing stays in Windows.",0,18,468,610,34);
        c(L"STATIC",L"Full Bluetooth reports may affect older games until the controller reconnects.",0,18,510,610,24);
        for(const auto& d:devices)SendMessageW(deviceList,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(d.name().c_str()));SendMessageW(deviceList,LB_SETCURSEL,0,0);return 0;
    }
    case WM_COMMAND:{
        int id=LOWORD(w);if(id==listId && HIWORD(w)==LBN_SELCHANGE){refreshDeviceWindow();return 0;}
        if(id!=saveId && id!=enabledId && id!=notifyId && id!=forgetId && id!=refreshDeviceId && id!=extendedId && id!=colorId)return 0;
        auto d=getDevice(selectedDeviceId());if(!d)return 0;
        if(id==saveId || id==enabledId || id==notifyId || id==extendedId){
            if(fillingDevices)return 0;
            bool selected=SendMessageW(enabledCheck,BM_GETCHECK,0,0)==BST_CHECKED;
            d->extendedReports=SendMessageW(extendedCheck,BM_GETCHECK,0,0)==BST_CHECKED;d->enabled=selected;d->blinkLow=SendMessageW(notifyCheck,BM_GETCHECK,0,0)==BST_CHECKED;
            if(id==saveId){wchar_t name[64]{},badge[4]{};GetWindowTextW(nameEdit,name,64);GetWindowTextW(badgeEdit,badge,4);d->alias=name;d->badge=std::min(unsigned(wcstoul(badge,nullptr,10)),9u);}
            d->configured=true;ob::saveDevice(*d);std::wstring chosen=d->id;
            auto index=SendMessageW(deviceList,LB_GETCURSEL,0,0);SendMessageW(deviceList,LB_DELETESTRING,index,0);SendMessageW(deviceList,LB_INSERTSTRING,index,reinterpret_cast<LPARAM>(d->name().c_str()));SendMessageW(deviceList,LB_SETCURSEL,index,0);
            updateAllIcons();if((id==enabledId || id==extendedId) && selected)refreshOne(chosen);
        }else if(id==forgetId){
            ob::forgetDevice(*d);
            // Keep a disabled tombstone for the legacy AJ default across restart.
            if(d->id==L"aj179"){d->configured=true;ob::saveDevice(*d);}
            auto index=SendMessageW(deviceList,LB_GETCURSEL,0,0);SendMessageW(deviceList,LB_DELETESTRING,index,0);SendMessageW(deviceList,LB_INSERTSTRING,index,reinterpret_cast<LPARAM>(d->name().c_str()));SendMessageW(deviceList,LB_SETCURSEL,index,0);updateAllIcons();
         }else if(id==colorId){
            static COLORREF custom[16]{};CHOOSECOLORW picker{};picker.lStructSize=sizeof(picker);picker.hwndOwner=h;picker.rgbResult=COLORREF(d->identityColor());picker.lpCustColors=custom;picker.Flags=CC_FULLOPEN|CC_RGBINIT;
            if(ChooseColorW(&picker)){d->ringColor=picker.rgbResult;d->customRingColor=true;d->configured=true;ob::saveDevice(*d);updateAllIcons();InvalidateRect(colorButton,nullptr,TRUE);}
        }else if(id==refreshDeviceId){refreshOne(d->id);}
        refreshDeviceWindow();return 0;
    }
    case WM_DRAWITEM:{
        auto item=reinterpret_cast<DRAWITEMSTRUCT*>(l);if(item->CtlID!=colorId)break;
        FillRect(item->hDC,&item->rcItem,GetSysColorBrush(COLOR_BTNFACE));DrawEdge(item->hDC,&item->rcItem,(item->itemState&ODS_SELECTED)?EDGE_SUNKEN:EDGE_RAISED,BF_RECT);
        auto device=getDevice(selectedDeviceId());COLORREF color=device?COLORREF(device->identityColor()):RGB(128,128,128);
        RECT swatch{item->rcItem.left+8,item->rcItem.top+6,item->rcItem.left+26,item->rcItem.bottom-6};HBRUSH brush=CreateSolidBrush(color);FillRect(item->hDC,&swatch,brush);DeleteObject(brush);
        wchar_t text[80];swprintf(text,80,L"Ring color: #%02X%02X%02X",GetRValue(color),GetGValue(color),GetBValue(color));RECT label=item->rcItem;label.left+=34;
        SetBkMode(item->hDC,TRANSPARENT);SetTextColor(item->hDC,GetSysColor(COLOR_BTNTEXT));auto old=SelectObject(item->hDC,uiFont);DrawTextW(item->hDC,text,-1,&label,DT_LEFT|DT_VCENTER|DT_SINGLELINE);SelectObject(item->hDC,old);
        if(item->itemState&ODS_FOCUS){RECT focus=item->rcItem;InflateRect(&focus,-3,-3);DrawFocusRect(item->hDC,&focus);}return TRUE;
    }
    case WM_DPICHANGED:{
        UINT next=HIWORD(w);if(!next)next=96;
        RECT* target=reinterpret_cast<RECT*>(l);SetWindowPos(h,nullptr,target->left,target->top,target->right-target->left,target->bottom-target->top,SWP_NOZORDER|SWP_NOACTIVATE);
        HFONT previous=uiFont;uiFont=CreateFontW(-MulDiv(14,next,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
        struct Scale {HWND parent;UINT oldDpi,newDpi;HFONT font;};Scale scale{h,devicesDpi,next,uiFont};
        EnumChildWindows(h,[](HWND child,LPARAM data)->BOOL{auto& s=*reinterpret_cast<Scale*>(data);RECT r;GetWindowRect(child,&r);MapWindowPoints(nullptr,s.parent,reinterpret_cast<POINT*>(&r),2);
            SetWindowPos(child,nullptr,MulDiv(r.left,s.newDpi,s.oldDpi),MulDiv(r.top,s.newDpi,s.oldDpi),MulDiv(r.right-r.left,s.newDpi,s.oldDpi),MulDiv(r.bottom-r.top,s.newDpi,s.oldDpi),SWP_NOZORDER|SWP_NOACTIVATE);
            SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(s.font),TRUE);return TRUE;},reinterpret_cast<LPARAM>(&scale));
        devicesDpi=next;if(previous)DeleteObject(previous);return 0;
    }
    case WM_CLOSE:DestroyWindow(h);return 0;
    case WM_DESTROY:devicesWindow=nullptr;deviceList=nullptr;if(uiFont)DeleteObject(uiFont);uiFont=nullptr;return 0;
    }
    return DefWindowProcW(h,msg,w,l);
}
void showDevices(const std::wstring& id=L""){
    if(!devicesWindow){
        WNDCLASSW cls{};cls.lpfnWndProc=deviceWindowProc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"OpenBattery.Devices";cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&cls);
        UINT dpi=GetDpiForSystem();RECT r{0,0,MulDiv(650,dpi,96),MulDiv(552,dpi,96)};AdjustWindowRectExForDpi(&r,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,0,dpi);
        devicesWindow=CreateWindowExW(0,cls.lpszClassName,L"Open Battery — Devices",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,cls.hInstance,nullptr);
    }
    auto it=std::find_if(devices.begin(),devices.end(),[&](const ob::Device& d){return d.id==id;});if(it!=devices.end())SendMessageW(deviceList,LB_SETCURSEL,it-devices.begin(),0);
    refreshDeviceWindow();
#ifndef OB_OFFSCREEN_TEST
    ShowWindow(devicesWindow,SW_SHOWNORMAL);SetForegroundWindow(devicesWindow);
#endif
}
std::wstring trayDeviceId(UINT number){for(const auto& t:trays)if(t.number==number)return t.id;return L"";}
