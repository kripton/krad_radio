#ifndef COMPOSITORCONTROL_H
#define COMPOSITORCONTROL_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include "resizableframe.h"

#include <QDebug>

class CompositorControl : public QGraphicsView
{
    Q_OBJECT
public:
    explicit CompositorControl(QRect size, QWidget *parent = 0);

signals:

public slots:

private:
    QGraphicsScene *scene;
    QList<QGraphicsProxyWidget> *frameList;

};

#endif // COMPOSITORCONTROL_H
