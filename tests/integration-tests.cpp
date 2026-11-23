// Exercises the production icon renderer and production logger in an isolated folder.
#define wWinMain obUnusedEntry
#include "../src/main.cpp"
#undef wWinMain
#include <cstdlib>
static void require(bool ok,const char* message){if(!ok){printf("FAIL: %s\n",message);exit(1);}}
int main(){
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    HICON warm=makeIcon(L"100",RGB(255,255,255),false); DestroyIcon(warm);
    warm=makeIcon(L"Zzz",RGB(145,145,145),true); DestroyIcon(warm);
    for(bool dark : {false,true}) {
        for(auto kind : {ob::Artwork::app,ob::Artwork::sleep,ob::Artwork::disconnected,ob::Artwork::error}) {
            for(int size : {16,20,24,32,40,48,64}) {
                HICON a=ob::loadArtwork(kind,size,dark);
                require(a!=nullptr,"embedded icon loads at each DPI size");
                ICONINFO ii{};require(GetIconInfo(a,&ii),"icon metadata");
                BITMAP bm{};GetObjectW(ii.hbmColor,sizeof(bm),&bm);
                require(bm.bmWidth==size && bm.bmHeight==size,"embedded icon has exact requested dimensions");
                DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DestroyIcon(a);
            }
        }
    }

    require(ob::loadNumberFont(),"embedded font loads privately");
    HDC fontDc=CreateCompatibleDC(nullptr);HFONT selected=ob::digitFont(64);
    auto previousFont=SelectObject(fontDc,selected);wchar_t face[64]{};
    GetTextFaceW(fontDc,64,face);
    require(std::wstring(face)==L"JetBrains Mono ExtraBold","actual selected family is JetBrains Mono");
    SelectObject(fontDc,previousFont);DeleteObject(selected);DeleteDC(fontDc);
    for(bool dark : {false,true})for(int value : {0,29,30,69,70,100}){
        COLORREF c=ob::batteryColor(value,dark);
        require(value>=70?GetGValue(c)>GetRValue(c):
            value>=30?(GetRValue(c)>GetGValue(c) && GetGValue(c)>GetBValue(c)):
            (GetRValue(c)>GetGValue(c) && GetRValue(c)>GetBValue(c)),
            "battery colors match semantic thresholds");
        for(int size : {16,20,24,32}){
            wchar_t text[8];swprintf(text,8,L"%d",value);
            HICON number=ob::numberIcon(text,c,size);require(number!=nullptr,"numeric icon renders");
            ICONINFO ii{};require(GetIconInfo(number,&ii),"numeric icon metadata");
            HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO info{};
            info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=size;
            info.bmiHeader.biHeight=-size;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
            std::vector<DWORD> pixels(size*size);
            require(GetDIBits(dc,ii.hbmColor,0,size,pixels.data(),&info,DIB_RGB_COLORS)!=0,"numeric alpha pixels available");
            bool any=false,partial=false;
            for(int y=0;y<size;++y)for(int x=0;x<size;++x){
                DWORD alpha=pixels[y*size+x]>>24;any|=alpha>0;partial|=alpha>0 && alpha<255;
                if(y==0 || y==size-1 || (wcslen(text)<3 && (x==0 || x==size-1)))
                    require(alpha==0,"digits keep vertical padding; 100 may use the full width");
            }
            require(any && partial,"numeric icon has visible anti-aliased ink");
            DeleteDC(dc);DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DestroyIcon(number);
        }
    }

    DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    for(int i=0;i<1000;++i){
        HICON a=i%4==0?makeIcon(L"100",RGB(255,255,255)):
            ob::loadArtwork(i%4==1?ob::Artwork::sleep:i%4==2?ob::Artwork::disconnected:ob::Artwork::error,32,i%2==0);
        require(a!=nullptr,"icon creation");DestroyIcon(a);
    }
    DWORD after=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
    printf("GDI before=%lu after=%lu\n",before,after); require(after<=before+2,"GDI handles stable after 1000 icons");
    wchar_t cwd[32768];GetCurrentDirectoryW(32768,cwd);
    folder=std::wstring(cwd)+L"\\build\\logger-test";logFolder=folder+L"\\Logs";
    ensureFolders();
    for(int i=0;i<5;++i)DeleteFileW((logFolder+L"\\debug-"+std::to_wstring(i)+L".log").c_str());
    ob::Reading r{ob::State::reading,37,false,0};
    debug=false;logReading(r);
    require(GetFileAttributesW((logFolder+L"\\debug-0.log").c_str())==INVALID_FILE_ATTRIBUTES,"normal mode does not log");
    debug=true;
    for(int i=0;i<60000;++i)logReading(r);
    int count=0;
    for(int i=0;i<5;++i){
        WIN32_FILE_ATTRIBUTE_DATA d{};
        if(GetFileAttributesExW((logFolder+L"\\debug-"+std::to_wstring(i)+L".log").c_str(),GetFileExInfoStandard,&d)){
            ++count;require(d.nFileSizeHigh==0 && d.nFileSizeLow<=1000000,"each log <=1MB");
        }
    }
    require(count==5,"log rotation retains five files");
    require(GetFileAttributesW((logFolder+L"\\debug-5.log").c_str())==INVALID_FILE_ATTRIBUTES,"no sixth log");
    ob::unloadNumberFont();
    printf("PASS: private JetBrains font, thresholds and alpha; 56 embedded icon variants; 1000 icon cycles, GDI %lu -> %lu; logging disabled by default; bounded five-file rotation\n",before,after);
}
