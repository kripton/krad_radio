#include "resizableframe.h"

#include <QDebug>

ResizableFrame::ResizableFrame(QWidget *parent) :
    QFrame(parent)
{
    setMouseTracking(true);
    opacity = 1.0f;
    opacityLabel = new QLabel(this);
    opacityLabel->setText(QString("Opacity: %1").arg(opacity, 0, 'f', 3));
}

ResizableFrame::~ResizableFrame()
{
}

void ResizableFrame::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        dragStartPosition = event->pos();
        dragStartGeometry = geometry();
    }
}

void ResizableFrame::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        // No drag, just change the cursor and return

        if (event->x() <= 3 && event->y() <= 3) {
            startPos = topleft;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= 3 && event->y() >= height() - 3) {
            startPos = bottomleft;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - 3 && event->y() <= 3) {
            startPos = topright;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - 3 && event->y() >= height() - 3) {
            startPos = bottomright;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= 3) {
            startPos = left;
            setCursor(Qt::SizeHorCursor);
        } else if (event->x() >= width() - 3) {
            startPos = right;
            setCursor(Qt::SizeHorCursor);
        } else if (event->y() <= 3) {
            startPos = top;
            setCursor(Qt::SizeVerCursor);
        } else if (event->y() >= height() - 3) {
            startPos = bottom;
            setCursor(Qt::SizeVerCursor);
        } else {
            startPos = move;
            setCursor(Qt::SizeAllCursor);
        }
        return;
    }

    switch (startPos) {
    case topleft:
        setGeometry(dragStartGeometry.left() - (dragStartPosition.x() - event->x()),
                    dragStartGeometry.top() - (dragStartPosition.y() - event->y()),
                    dragStartGeometry.width() + (dragStartPosition.x() - event->x()),
                    height() + (dragStartPosition.y() - event->y()));
        dragStartGeometry = geometry();
        break;

    case bottomleft:
        setGeometry(dragStartGeometry.left() - (dragStartPosition.x() - event->x()),
                    dragStartGeometry.top(),
                    dragStartGeometry.width() + (dragStartPosition.x() - event->x()),
                    event->y());
        dragStartGeometry = geometry();
        break;

    case topright:
        setGeometry(dragStartGeometry.left(),
                    dragStartGeometry.top() - (dragStartPosition.y() - event->y()),
                    event->x(),
                    height() + (dragStartPosition.y() - event->y()));
        dragStartGeometry = geometry();
        break;

    case bottomright:
        setGeometry(dragStartGeometry.left(),
                    dragStartGeometry.top(),
                    event->x(),
                    event->y());
        break;

    case left:
        setGeometry(dragStartGeometry.left() - (dragStartPosition.x() - event->x()),
                    dragStartGeometry.top(),
                    dragStartGeometry.width() + (dragStartPosition.x() - event->x()),
                    height());
        dragStartGeometry = geometry();
        break;

    case right:
        setGeometry(dragStartGeometry.left(),
                    dragStartGeometry.top(),
                    event->x(),
                    height());
        break;

    case top:
        setGeometry(dragStartGeometry.left(),
                    dragStartGeometry.top() - (dragStartPosition.y() - event->y()),
                    dragStartGeometry.width(),
                    height() + (dragStartPosition.y() - event->y()));
        dragStartGeometry = geometry();
        break;

    case bottom:
        setGeometry(dragStartGeometry.left(),
                    dragStartGeometry.top(),
                    width(),
                    event->y());
        break;

    case move:
        setGeometry(dragStartGeometry.left() - (dragStartPosition.x() - event->x()),
                    dragStartGeometry.top() - (dragStartPosition.y() - event->y()),
                    width(),
                    height());
        dragStartGeometry = geometry();
        break;

    default:
        break;
    }
}

void ResizableFrame::wheelEvent(QWheelEvent *event)
{
    event->accept();
    if (event->orientation() != Qt::Vertical) return;
    qDebug() << "WHEEL:" << event->delta();

    opacity = opacity + (event->delta() / 4800.0);
    if (opacity < 0.0) opacity = 0.0;
    if (opacity > 1.0) opacity = 1.0;

    opacityLabel->setText(QString("Opacity: %1").arg(opacity, 0, 'f', 3));
}
