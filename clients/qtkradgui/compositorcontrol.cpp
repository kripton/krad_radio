#include "compositorcontrol.h"

CompositorControl::CompositorControl(QRect size, QWidget *parent) :
    QGraphicsView(parent)
{
    qDebug() << "New CompositorControl:" << size;
    scene = new QGraphicsScene(0, 0, size.width(), size.height(), this);
    setGeometry(size);

    frameList = new QList<QGraphicsProxyWidget>();
}
