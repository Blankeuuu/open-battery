#define wWinMain obUnusedEntry
#include "../src/main.cpp"
#undef wWinMain
#include <atomic>
static std::atomic<int> polls{0};
static int staleReplies=0;
static HANDLE gate=nullptr;
static ob::Reading fixture(){
    int n=++polls;
    if(n==1 && gate)WaitForSingleObject(gate,1000);
    return n<=staleReplies?ob::Reading{ob::State::inactive,-1,false,0}:ob::Reading{ob::State::reading,73,false,0};
}
static void check(bool yes,const char* text){if(!yes){printf("FAIL: %s\n",text);exit(1);}}
static void drain(){
    MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){if(m.message!=WM_QUIT){TranslateMessage(&m);DispatchMessageW(&m);}}
}
static bool readingWithin(DWORD milliseconds){
    ULONGLONG end=GetTickCount64()+milliseconds;
    while(GetTickCount64()<end){drain();if(current.state==ob::State::reading)return true;Sleep(1);}
    return false;
}
static void reset(){
    check(worker==nullptr,"previous read completed");
    wakeInput.stop();KillTimer(window,2);KillTimer(window,3);
    wakeRequested=false;wakeAttempts=0;nextWakeAllowed=0;polls=0;staleReplies=0;
    current={ob::State::inactive,-1,false,0};
}
int main(){
    queryBattery=fixture;
    WNDCLASSW wc{};wc.lpfnWndProc=wndProc;wc.lpszClassName=L"OpenBattery.WakeRegression";wc.hInstance=GetModuleHandleW(nullptr);
    RegisterClassW(&wc);
    window=CreateWindowExW(0,wc.lpszClassName,L"Wake regression",0,0,0,0,0,nullptr,nullptr,wc.hInstance,nullptr);
    reset();
    SendMessageW(window,wakeMessage,0,0);
    check(worker!=nullptr,"inactive + target activity starts immediate battery read");
    for(int i=0;i<100;++i)SendMessageW(window,wakeMessage,0,0);
    check(readingWithin(500) && polls==1,"movement burst coalesces to one read and updates within 500ms");
    check(!wakeInput.armed(),"no background input stream while battery reading is current");
    SendMessageW(window,wakeMessage,0,0);check(worker==nullptr && polls==1,"movement while active does not poll");
    reset();staleReplies=2;
    SendMessageW(window,wakeMessage,0,0);
    check(readingWithin(2000) && polls==3,"two stale wake replies recover before the 60s timer");
    reset();staleReplies=1;gate=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    refresh();HANDLE first=worker;SendMessageW(window,wakeMessage,0,0);
    check(worker==first && wakeRequested,"activity during in-flight periodic read is retained");
    SetEvent(gate);check(readingWithin(1000) && polls==2,"in-flight stale read schedules wake retry");
    CloseHandle(gate);gate=nullptr;
    reset();staleReplies=100;
    SendMessageW(window,wakeMessage,0,0);
    ULONGLONG end=GetTickCount64()+3200;while(GetTickCount64()<end){drain();Sleep(1);}
    check(polls==4 && !wakeRequested && nextWakeAllowed>GetTickCount64(),"wake retries bounded with cooldown");
    SendMessageW(window,wakeMessage,0,0);check(polls==4 && !worker,"cooldown prevents request storms");
    check(ob::targetMouseName(L"\\?\\HID#VID_3151&PID_402D&MI_00"),"target mouse identified case-insensitively");
    check(!ob::targetMouseName(L"\\?\\HID#VID_046D&PID_C539"),"other mouse ignored");
    DestroyWindow(window);ob::unloadNumberFont();
    printf("PASS: immediate wake, event coalescing, stale retries, in-flight race, bounded retry, device filtering\n");
}
