#pragma once
#include <windows.h>
namespace ob {
enum class Artwork { app, sleep, disconnected, error };
inline HICON loadArtwork(Artwork kind,int size,bool dark){
    int id=100;
    if(kind==Artwork::sleep)id=101;
    else if(kind==Artwork::disconnected)id=102;
    else if(kind==Artwork::error)id=103;
    if(kind!=Artwork::app && !dark)id+=10;
    return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(id),
        IMAGE_ICON,size,size,LR_DEFAULTCOLOR));
}
}
