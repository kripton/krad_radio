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
    void addFrame(QString id);
    void removeFrame(QString id);

private:
    QGraphicsScene *scene;
    QHash<QString, QGraphicsProxyWidget*> *frameList;

};

#endif // COMPOSITORCONTROL_H
