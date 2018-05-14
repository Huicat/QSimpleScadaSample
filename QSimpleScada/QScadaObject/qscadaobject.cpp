#include "qscadaobject.h"

#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPen>

#define RESIZE_FIELD_SIZE 10
#define RESIZE_AREA(x, y) ((geometry().width() - RESIZE_FIELD_SIZE) < x) && ((geometry().height() - RESIZE_FIELD_SIZE) < y)

QScadaObject::QScadaObject(QWidget *parent) :
    QWidget(parent),
    mInfo{new QScadaObjectInfo(this)},
    mEffect{new QGraphicsDropShadowEffect},
    mStatus{VObjectStatusNone}
{
    setGeometry(100, 100, 100, 100);
    if (info()->showBackground()) {
        setPalette(QPalette(Qt::transparent));
        setAutoFillBackground(true);
    }

    mEffect->setOffset(0);
    setGraphicsEffect(mEffect);

    dynamicStatusChanged(mInfo);

    setAction(VObjectActionNone);
    setMouseTracking(true);//this not mouseMoveEven is called everytime mouse is moved

    connect(mInfo, SIGNAL(dynamicStatusChanged(QScadaObjectInfo *)), this, SLOT(dynamicStatusChanged(QScadaObjectInfo *)));
}

QScadaObject::~QScadaObject()
{
    delete mInfo;
    delete mEffect;
}

void QScadaObject::setGeometry(int x, int y, int width, int height)
{
    setGeometry(QRect(x, y, width, height));
}

void QScadaObject::setGeometry(const QRect &r)
{
    info()->setGeometry(r);
    QWidget::setGeometry(r);
}

QRect QScadaObject::geometry()
{
    return info()->geometry();
}

void QScadaObject::mouseMoveEvent(QMouseEvent *event)
{
    if (mIsEditable) {
        switch (action()) {
        case VObjectActionMove:{
            move(event->x(), event->y());
            break;
        }
        case VObjectActionResize:{
            resize(event->x(), event->y());
            break;
        }
        case VObjectActionNone: {
            if (RESIZE_AREA(event->x(), event->y())) {
                QApplication::setOverrideCursor(Qt::SizeFDiagCursor);
            } else if (underMouse()) {
                QApplication::setOverrideCursor(Qt::OpenHandCursor);
            }
        }
        }
    } else {
        QWidget::mouseMoveEvent(event);
    }
}

void QScadaObject::mousePressEvent(QMouseEvent *event)
{
    if (mIsEditable) {
        if (event->button() == Qt::LeftButton) {
            int lX = event->x();
            int lY = event->y();

            if (RESIZE_AREA(lX, lY)) {
                setAction(VObjectActionResize);
            } else {
                setSelected(true);
                QApplication::setOverrideCursor(Qt::ClosedHandCursor);
                setAction(VObjectActionMove);

                mPosition.setX(lX);
                mPosition.setY(lY);
            }
        }
    } else {
        QWidget::mousePressEvent(event);
    }
}

void QScadaObject::mouseReleaseEvent(QMouseEvent *event)
{
    if (mIsEditable) {
        (void)event;
        setAction(VObjectActionNone);
        QApplication::setOverrideCursor(Qt::ArrowCursor);
    } else {
        QWidget::mouseReleaseEvent(event);
    }
}

void QScadaObject::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        emit objectDoubleClicked(this);
    }
}

void QScadaObject::paintEvent(QPaintEvent *e)
{
    QPainter lPainter(this);
    QPixmap lMarkerPixmap(info()->imageName(mStatus));
    QPixmap lBackgroundPixmap(info()->backGroundImage());
    QPen lLinepen(Qt::black);
    lLinepen.setCapStyle(Qt::RoundCap);
    lPainter.setRenderHint(QPainter::Antialiasing,true);
    lPainter.setPen(lLinepen);

    //draw object title
    lPainter.drawText(QPoint(10, 20), mInfo->title());
    int lX;
    int lY;

    if (info()->axiesEnabled()) {
        //draw axies

        switch(info()->axisPosition()) {
        case VObjectAxisPositionLeft:
            lX = 12;
            break;
        case VObjectAxisPositionRight:
            lX = geometry().width() - 30;
            break;
        }
        lY = geometry().height() - 10;
        int lWidth = 10;
        int lInner = 4;

        lPainter.drawEllipse(lX-lInner/2,
                             lY-lInner/2,
                             lInner,
                             lInner);
        lPainter.drawText(QPoint(lX - 10, geometry().height()-2), info()->axis().insideAxisString());//inside position

        lPainter.drawLine(lX, lY,
                          lX, lY -lWidth);
        lPainter.drawLine(lX, lY -lWidth-1,
                          lX-3, lY -lWidth+3);
        lPainter.drawLine(lX, lY -lWidth-1,
                          lX+3, lY -lWidth+3);
        lPainter.drawText(QPoint(lX - 3, lY -lWidth-3), info()->axis().upsideAxisString());//up possibtion

        lPainter.drawLine(lX, lY,
                          lX + lWidth, lY);
        lPainter.drawLine(lX + lWidth+1, lY,
                          lX + lWidth-3, lY-3);
        lPainter.drawLine(lX + lWidth+1, lY,
                          lX + lWidth-3, lY+3);
        lPainter.drawText(QPoint(lX + lWidth + 3, lY+3), info()->axis().asideAxisString());//aside position
    }

    //draw resize dots
    lX = geometry().width()-RESIZE_FIELD_SIZE;
    lY = geometry().height();

    lLinepen.setColor(Qt::darkGray);
    lLinepen.setWidth(1);
    lPainter.setPen(lLinepen);

    if (mIsEditable) {
        for (int i=1; i<=RESIZE_FIELD_SIZE; i++) {
            for (int j=1; j<=i; j++) {
                lPainter.drawPoint(QPoint(lX + 2*i, lY - 2*j));
            }
        }
    }

    if (info()->showMarkers()) {
        QSize lSize = lMarkerPixmap.size();
        lPainter.drawPixmap(QRect((width() - lSize.width()) /2,
                                  (height() - lSize.height()) / 2,
                                  lSize.width(),
                                  lSize.height()),
                            lMarkerPixmap);
    }

    if (info()->showBackgroundImage()) {
        lPainter.drawPixmap(QRect(0,
                                  0,
                                  width(),
                                  height()),
                            lBackgroundPixmap.scaled(width(), height(),
                                                     Qt::KeepAspectRatioByExpanding));
    }

    if (info()->isDynamic()) {
        switch(mStatus) {
        case VObjectStatusNone:
            lLinepen.setColor(Qt::darkGray);
            break;
        case VObjectStatusRed:
            lLinepen.setColor(QColor(171, 27, 227, 255));
            break;
        case VObjectStatusYellow:
            lLinepen.setColor(QColor(228, 221, 29, 255));
            break;
        case VObjectStatusGreen:
            lLinepen.setColor(QColor(14, 121, 7, 255));
            break;
        }
    } else {
        lLinepen.setColor(Qt::black);
    }

    if (info()->showBackground()) {
        lLinepen.setWidth(2);
        lPainter.setPen(lLinepen);
        lPainter.drawRoundedRect(0,0,width(), height(),3,3);
    }

    QWidget::paintEvent(e);
}

void QScadaObject::dynamicStatusChanged(QScadaObjectInfo*)
{
    if (info()->isDynamic()) {
        switch(mStatus) {
        case VObjectStatusNone:
            setPalette(QPalette(Qt::lightGray));
            break;
        case VObjectStatusRed:
            setPalette(QPalette(Qt::red));
            break;
        case VObjectStatusYellow:
            setPalette(QPalette(Qt::yellow));
            break;
        case VObjectStatusGreen:
            setPalette(QPalette(Qt::green));
            break;
        }
    } else {
        setPalette(QPalette(Qt::white));
    }

    if (!info()->showBackground()) {
        setPalette(QPalette(Qt::transparent));
    }
}

QScadaObjectStatus QScadaObject::status() const
{
    return mStatus;
}

void QScadaObject::setStatus(const QScadaObjectStatus &status)
{
    mStatus = status;
    dynamicStatusChanged(mInfo);
}

bool QScadaObject::isEditable() const
{
    return mIsEditable;
}

void QScadaObject::setIsEditable(bool isEditable)
{
    mIsEditable = isEditable;
}

void QScadaObject::update()
{
    if (info()->showBackground()) {
        setPalette(QPalette(Qt::transparent));
        setAutoFillBackground(true);
    }

    QWidget::update();

    setGeometry(info()->geometry());
    dynamicStatusChanged(info());
}

bool QScadaObject::selected() const
{
    return mSelected;
}

void QScadaObject::setSelected(bool selected)
{
    mSelected = selected;

    if (mSelected) {
        emit objectSelected(mInfo->id());

        mEffect->setBlurRadius(50);
        raise();
    } else {
        mEffect->setBlurRadius(10);
    }
}

QScadaObjectInfo *QScadaObject::info() const
{
    return mInfo;
}

void QScadaObject::setInfo(QScadaObjectInfo *info)
{
    setGeometry(info->geometry());
    mInfo = info;
}

QScadaObjectAction QScadaObject::action() const
{
    return mAction;
}

void QScadaObject::setAction(const QScadaObjectAction &action)
{
    mAction = action;
}

void QScadaObject::move(int x, int y)
{
    int lX = geometry().x() + x - mPosition.x();
    int lY = geometry().y() + y - mPosition.y();

    setGeometry(lX,
                lY,
                geometry().width(),
                geometry().height());

    if (x != 0 && x != 0) {
        emit objectMove(lX, lY);
    }
}

void QScadaObject::resize(int x, int y)
{
    int lX = x - geometry().width();
    int lY = y - geometry().height();

    setGeometry(geometry().x(),
                geometry().y(),
                geometry().width() + lX,
                geometry().height() + lY);

    repaint();

    if (x != 0 && y != 0) {
        emit objectResize(lX, lY);
    }
}
