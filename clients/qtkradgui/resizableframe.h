#ifndef RESIZABLEFRAME_H
#define RESIZABLEFRAME_H

#include <QFrame>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QPainter>
#include <QApplication>
#include <QGraphicsProxyWidget>

//namespace Ui {
//class ResizableFrame;
//}

class ResizableFrame : public QFrame
{
    Q_OBJECT

public:
    explicit ResizableFrame(QWidget *parent = 0);
    ~ResizableFrame();

    enum startPositions {topleft, left, bottomleft, bottom, bottomright, right, topright, top, move} startPositions;
    void setProxy(QGraphicsProxyWidget *proxy);

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent * event);

private:
    QPoint dragStartPosition;
    QRect  dragStartGeometry;
    enum startPositions startPos;
    QLabel *opacityLabel;
    double opacity;
    QGraphicsProxyWidget *proxy;

signals:
    void geometryChanged(QRect newGeometry);
    void opacityChanged(float newOpacity);
    void rotationChanged(float newAngle);
};

#endif // RESIZABLEFRAME_H
