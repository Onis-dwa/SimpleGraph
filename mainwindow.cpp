#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>


// cpu gpu ram percent
#include <thread>
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define DIV 1024
#define WIDTH 7

static float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
   static unsigned long long _previousTotalTicks = 0;
   static unsigned long long _previousIdleTicks = 0;

   unsigned long long totalTicksSinceLastTime = totalTicks-_previousTotalTicks;
   unsigned long long idleTicksSinceLastTime  = idleTicks-_previousIdleTicks;

   float ret = 1.0f-((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime)/totalTicksSinceLastTime : 0);

   _previousTotalTicks = totalTicks;
   _previousIdleTicks  = idleTicks;
   return ret;
}
static unsigned long long FileTimeToInt64(const FILETIME & ft) {
    return (((unsigned long long)(ft.dwHighDateTime))<<32)|((unsigned long long)ft.dwLowDateTime);
}
// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad()
{
   FILETIME idleTime, kernelTime, userTime;
   return GetSystemTimes(&idleTime, &kernelTime, &userTime) ? CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime)+FileTimeToInt64(userTime)) : -1.0f;
}
// respect http://eliang.blogspot.nl/2011/05/getting-nvidia-gpu-usage-in-c.html
// magic numbers, do not change them
#define NVAPI_MAX_PHYSICAL_GPUS   64
#define NVAPI_MAX_USAGES_PER_GPU  34
// function pointer types
typedef int *(*NvAPI_QueryInterface_t)(unsigned int offset);
typedef int  (*NvAPI_Initialize_t)();
typedef int  (*NvAPI_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int  (*NvAPI_GPU_GetUsages_t)(int *handle, unsigned int *usages);
//void MainClient::GetLoad()
void GetLoad(MainWindow* M)
{
    HMODULE hmod = LoadLibraryA("nvapi.dll");
    if (hmod == NULL)
    {
        qDebug() << "Couldn't find nvapi.dll";
        return;
    }

    // nvapi.dll internal function pointers
    NvAPI_QueryInterface_t      NvAPI_QueryInterface     = NULL;
    NvAPI_Initialize_t          NvAPI_Initialize         = NULL;
    NvAPI_EnumPhysicalGPUs_t    NvAPI_EnumPhysicalGPUs   = NULL;
    NvAPI_GPU_GetUsages_t       NvAPI_GPU_GetUsages      = NULL;

    // nvapi_QueryInterface is a function used to retrieve other internal functions in nvapi.dll
    NvAPI_QueryInterface = (NvAPI_QueryInterface_t) GetProcAddress(hmod, "nvapi_QueryInterface");

    // some useful internal functions that aren't exported by nvapi.dll
    NvAPI_Initialize = (NvAPI_Initialize_t) (*NvAPI_QueryInterface)(0x0150E828);
    NvAPI_EnumPhysicalGPUs = (NvAPI_EnumPhysicalGPUs_t) (*NvAPI_QueryInterface)(0xE5AC921F);
    NvAPI_GPU_GetUsages = (NvAPI_GPU_GetUsages_t) (*NvAPI_QueryInterface)(0x189A1FDF);

    if (NvAPI_Initialize == NULL || NvAPI_EnumPhysicalGPUs == NULL ||
    NvAPI_EnumPhysicalGPUs == NULL || NvAPI_GPU_GetUsages == NULL)
    {
        qDebug() << "Couldn't get functions in nvapi.dll";
        hmod = NULL;
        return;
    }

    // initialize NvAPI library, call it once before calling any other NvAPI functions
    (*NvAPI_Initialize)();

    int          gpuCount = 0;
    int         *gpuHandles[NVAPI_MAX_PHYSICAL_GPUS] = { NULL };
    unsigned int gpuUsages[NVAPI_MAX_USAGES_PER_GPU] = { 0 };

    // gpuUsages[0] must be this value, otherwise NvAPI_GPU_GetUsages won't work
    gpuUsages[0] = (NVAPI_MAX_USAGES_PER_GPU * 4) | 0x10000;

    (*NvAPI_EnumPhysicalGPUs)(gpuHandles, &gpuCount);

    while (!M->stopped)
    {
        // CPU
        M->SetCPU(roundf(100.0 * GetCPULoad()));

        // GPU
        (*NvAPI_GPU_GetUsages)(gpuHandles[0], gpuUsages);
        M->SetGPU(gpuUsages[3]);

        // RAM
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx(&statex);
        M->SetRAM(statex.dwMemoryLoad);

        Sleep(500);
    }
}







MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
	this->setMouseTracking(true);

	cpu = new Graph(112, 9, 311, 31, ui->CPU, this);
	gpu = new Graph(112, 45, 311, 31, ui->GPU, this);
	ram = new Graph(112, 85, 311, 31, ui->RAM, this);

    connect(ui->pushButton, SIGNAL(clicked(bool)), SLOT(stop(bool)));

    stopped = false;
    std::thread tload(GetLoad, this);
    tload.detach();
}
MainWindow::~MainWindow()
{
    delete cpu;
    delete gpu;
    delete ram;

    delete ui;
}
void MainWindow::SetCPU(int val)
{
    if (!cpu->spectrate)
        ui->CPU->setText(QString::number(val) + "%");
    cpu->AddValue(val);
}
void MainWindow::SetGPU(unsigned int val)
{
    if (!gpu->spectrate)
        ui->GPU->setText(QString::number(val) + "%");
    gpu->AddValue(val);
}
void MainWindow::SetRAM(int val)
{
    if (!ram->spectrate)
        ui->RAM->setText(QString::number(val) + "%");
    ram->AddValue(val);
}

void MainWindow::setpos(int w, int h)
{
    ui->label_3->setText(QString::number(w));
    ui->label_4->setText(QString::number(h));
}

void MainWindow::stop(bool)
{
    if (stopped)
    {
        ui->pushButton->setText("Stop");

        stopped = false;
        std::thread tload(GetLoad, this);
        tload.detach();
    }
    else
    {
        ui->pushButton->setText("Start");
        stopped = true;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *)
{
    if (cpu->spectrate)
        cpu->spectrate = false;

    if (gpu->spectrate)
        gpu->spectrate = false;

    if (ram->spectrate)
        ram->spectrate = false;
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    int w = e->size().width() - 123;

    cpu->rs(w, 31);
    gpu->rs(w, 31);
    ram->rs(w, 31);

    ui->label->setText(QString::number(cpu->width()));
    ui->label_2->setText(QString::number(cpu->height()));
}
