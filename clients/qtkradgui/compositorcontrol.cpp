#include "compositorcontrol.h"

CompositorControl::CompositorControl(QRect size, QWidget *parent) :
    QGraphicsView(parent)
{
    qDebug() << "New CompositorControl:" << size;
    scene = new QGraphicsScene(0, 0, size.width(), size.height(), this);
    setGeometry(size);
    setScene(scene);

    frameList = new QHash<QString, QGraphicsProxyWidget*>();

    // TODO: Remove these dummies ;)
    addFrame("Krippi");
    addFrame("Krippi2");
    removeFrame("Krippi");
}

void CompositorControl::addFrame(QString id)
{
    ResizableFrame *frame = new ResizableFrame();
    frame->setGeometry(50,50,100,100); // DUMMY, we need to get the real geometry later
    QGraphicsProxyWidget *proxy = scene->addWidget(frame);
    frame->setProxy(proxy);
    frame->setId(id);

    frameList->insert(id, proxy);
}

void CompositorControl::removeFrame(QString id)
{
    QGraphicsProxyWidget *proxy = frameList->value(id);
    scene->removeItem(proxy);
    proxy->deleteLater();
}
