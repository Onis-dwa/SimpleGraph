#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "graph.h"
#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void SetCPU(int);
    void SetGPU(unsigned int);
    void SetRAM(int);

private:
    Ui::MainWindow *ui;

    Graph* cpu;
    Graph* gpu;
    Graph* ram;

protected:
    virtual void mouseMoveEvent(QMouseEvent*);
};

#endif // MAINWINDOW_H
