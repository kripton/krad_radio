#include "resizableframe.h"

#include <QDebug>

ResizableFrame::ResizableFrame(QWidget *parent) :
    QFrame(parent)
{
    setMouseTracking(true);

    setFrameStyle(QFrame::Panel);
    setFrameShadow(QFrame::Plain);
    setLineWidth(0);
    setMidLineWidth(0);

    layout = new QBoxLayout(QBoxLayout::TopToBottom, this);

    idLabel = new QLabel(this);
    //idLabel->setText("ID LABEL");
    layout->addWidget(idLabel);

    opacity = 1.0f;
    opacityLabel = new QLabel(this);
    opacityLabel->setText(QString("Opacity: %1").arg(opacity, 0, 'f', 3));
    layout->addWidget(opacityLabel);

    connect(this, SIGNAL(geometryChanged(QRect)), this, SLOT(updateGeometry(QRect)));
    connect(this, SIGNAL(opacityChanged(float)), this, SLOT(updateOpacity(float)));
}

ResizableFrame::~ResizableFrame()
{
}

void ResizableFrame::setProxy(QGraphicsProxyWidget *proxy)
{
    this->proxy = proxy;
    qDebug() << "Our new proxy is" << proxy;
    connect(this, SIGNAL(rotationChanged(float)), this, SLOT(updaterotation(float)));
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

        if (event->x() <= BORDER_RANGE && event->y() <= BORDER_RANGE) {
            startPos = topleft;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= BORDER_RANGE && event->y() >= height() - BORDER_RANGE) {
            startPos = bottomleft;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - BORDER_RANGE && event->y() <= BORDER_RANGE) {
            startPos = topright;
            setCursor(Qt::SizeBDiagCursor);
        } else if (event->x() >= width() - BORDER_RANGE && event->y() >= height() - BORDER_RANGE) {
            startPos = bottomright;
            setCursor(Qt::SizeFDiagCursor);
        } else if (event->x() <= BORDER_RANGE) {
            startPos = left;
            setCursor(Qt::SizeHorCursor);
        } else if (event->x() >= width() - BORDER_RANGE) {
            startPos = right;
            setCursor(Qt::SizeHorCursor);
        } else if (event->y() <= BORDER_RANGE) {
            startPos = top;
            setCursor(Qt::SizeVerCursor);
        } else if (event->y() >= height() - BORDER_RANGE) {
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

    emit geometryChanged(geometry());
}

void ResizableFrame::wheelEvent(QWheelEvent *event)
{
    event->accept();
    if (event->orientation() != Qt::Vertical) return;

    //qDebug() << "WHEEL:" << event->delta() << "Modifiers:" <<  QApplication::keyboardModifiers();

    // Rotate
    if ((proxy != NULL) &&  (QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        proxy->setTransformOriginPoint(geometry().width()/2, geometry().height()/2);
        proxy->setRotation(proxy->rotation() - event->delta() / 60.0);
        emit rotationChanged(proxy->rotation());
    } else {
        // Change opacity
        opacity = opacity + (event->delta() / 4800.0);
        if (opacity < 0.0) opacity = 0.0;
        if (opacity > 1.0) opacity = 1.0;
        opacityLabel->setText(QString("Opacity: %1").arg(opacity, 0, 'f', 3));
        emit opacityChanged(opacity);
    }
}

void ResizableFrame::updateGeometry(QRect geometry)
{
    setGeometry(geometry);
}

void ResizableFrame::updateOpacity(float opacity)
{
    this->opacity = opacity;
    if (this->opacity < 0.0) this->opacity = 0.0;
    if (this->opacity > 1.0) this->opacity = 1.0;
    opacityLabel->setText(QString("Opacity: %1").arg(this->opacity, 0, 'f', 3));

}

void ResizableFrame::updaterotation(float angle)
{
    if (rotation != angle) {
        rotation = angle;
        proxy->setTransformOriginPoint(geometry().width()/2, geometry().height()/2);
        proxy->setRotation(angle);
    }
}

void ResizableFrame::setId(QString id)
{
    qDebug() << "setID" << id << "Text pre" << idLabel->text();
    idLabel->setText(QString("ID: %1").arg(id));
    qDebug() << "setID" << id << "Text post" << idLabel->text();
}
