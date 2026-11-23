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
    printf("PASS: 56 embedded icon variants; 1000 icon cycles, GDI %lu -> %lu; logging disabled by default; bounded five-file rotation\n",before,after);
}
