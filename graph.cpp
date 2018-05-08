#include "graph.h"

Graph::Graph(int x, int y, int w, int h, QLabel* l, QWidget *parent) : QWidget(parent)
{
    this->setGeometry(x, y, w, h);
    this->setMouseTracking(true);
    this->show();

    val = l;
    graph = new QPixmap(w, h);
    paint = new QPainter;
    out = new QLabel(this);
    out->setFrameShape(out->Panel);
    out->setFrameShadow(out->Sunken);
    out->setAlignment(Qt::AlignRight);
    out->setGeometry(0, 0, w, h);
    out->setMouseTracking(true);
    out->show();

    for (last = 0; last < 4000; last++)
        values[last] = 255;
    last = 0;
    percent = (double)h / 100.0;
    spectrate = false;
    count = 0;
}
Graph::~Graph()
{
    delete val;
    delete out;
    delete graph;
    delete paint;
}
void Graph::AddValue(int val)
{
    if (last > 4000)
        last = 0;
    values[last++] = val;
    count++;
    x = out->width();
    y = out->height();

    paint->begin(graph);
    paint->eraseRect(0, 0, out->width(), out->height());
    paint->setPen(QPen(Qt::red, 1));

    int i;
    int value;
    for (i = last; i >= 0; i--)
    {
        value = values[i];
        if (value != 255)
            paint->drawLine(x, y - percent * value, x--, y);
    }
    for (i = 49; i >= last; i--)
    {
        value = values[i];
        if (value != 255)
            paint->drawLine(x, y - percent * value, x--, y);
    }


    paint->end();
    out->setPixmap(*graph);
}



void Graph::mouseMoveEvent(QMouseEvent* e)
{
    spectrate = true;

    if (e->x() < 0 || e->x() > out->width() || e->y() < 0 || e->y() > out->height())
    {
        spectrate = false;
        return;
    }

    int pos = out->width() - e->x();
    if (pos > count)
        return;

    if (pos > last)
    {
        pos -= last + 3;
        val->setText("@ " + QString::number(values[4000 - pos]));
    }
    else
        val->setText("@ " + QString::number(values[last - pos - 3]));
}

