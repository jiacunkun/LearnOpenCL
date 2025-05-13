#include "MainView.h"
#include <QWheelEvent>
#include <QPixmap>
#include <QDebug>
#include <QRectF>
#include <QtOpenGL/QtOpenGL>
#include "mainwindow.h"
#include <math.h>


#define VIEW_WIDTH  viewport()->rect().width()
#define VIEW_HEIGHT viewport()->rect().height()

CMainView::CMainView(QWidget* parent /*= nullptr*/) : QGraphicsView(parent)
{
	setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setAttribute(Qt::WA_AcceptTouchEvents);
    Init();
	m_fScale = 0.5f;
	m_nScaleStep = 10;
    m_currentStepScaleFactor = 1.0f;
    m_fTranslateX = 0.0f;
    m_fTranslateY = 0.0f;
    m_fRotationAngle = 0.0f;
    
	m_bEnabledScale = true;
	m_pScene = NULL;
	m_ptStartPoint = QPointF(0, 0);
	m_pRectItem = NULL;
	setMouseTracking(true);
    m_bPicker = false;
    m_nPickType = 0;
    m_cursor = QCursor(QPixmap(":/images/cur.png"));
    m_nCommandType = eToolType_Arrow;
}

CMainView::~CMainView()
{
}

void CMainView::Init()
{
    grabGesture(Qt::PinchGesture);// 捏合
    grabGesture(Qt::PanGesture);// 平移
    grabGesture(Qt::SwipeGesture);// 滑动
}

void CMainView::OpenImage(QString v_strFileName)
{
	if (m_pScene)
	{
		m_pScene->OnOpen(v_strFileName);
 		centerOn(m_pScene->width() / 2, m_pScene->height() / 2);// 设置场景的中点在窗口中居中
		fitInView(0, 0, m_pScene->width(), m_pScene->height(), Qt::KeepAspectRatio);// 设置自适应

        
        m_fScale = 0.5f;
        m_nScaleStep = 10;
        m_currentStepScaleFactor = 1.0f;
        m_fTranslateX = 0.0f;
        m_fTranslateY = 0.0f;
        m_fRotationAngle = 0.0f;
		QMatrix matrix = this->matrix();
		m_fScale = matrix.m11();
		CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
		if (pParent)
		{
			emit pParent->scaleChanged(m_fScale*100);
		}
	}
	setMouseTracking(true);
}

void CMainView::OpenImage(QImage v_Image)
{
	if (m_pScene) {
		m_pScene->OnOpen(v_Image);
		centerOn(m_pScene->width() / 2, m_pScene->height() / 2);// 设置场景的中点在窗口中居中
		fitInView(0, 0, m_pScene->width(), m_pScene->height(), Qt::KeepAspectRatio);// 设置自适应

        m_fScale = 0.5f;
        m_nScaleStep = 10;
        m_currentStepScaleFactor = 1.0f;
        m_fTranslateX = 0.0f;
        m_fTranslateY = 0.0f;
        m_fRotationAngle = 0.0f;
		QMatrix matrix = this->matrix();
		m_fScale = matrix.m11();
		CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
		if (pParent) {
			emit pParent->scaleChanged(m_fScale*100);
		}
	}
	setMouseTracking(true);
}

int CMainView::ImageDenoiseHandle(QString srcImage, BASE_SINGLE_IMAGE_PARAM param)
{
    if (m_pScene)
    {
        m_pScene->ImageDenoiseHandle( srcImage, param);
    }
	return 0;
}

void CMainView::CloseImage()
{
    if (m_pScene) {
        m_pScene->CloseImage();
    }
}

void CMainView::OnSave(QImage v_Image, QString v_strFileName)
{
	if (m_pScene) {
		return m_pScene->OnSave(v_Image, v_strFileName);
	}
}

void CMainView::setRadius(int nValue)
{
    qDebug() << nValue;
	if (m_pScene) {
// 		m_pScene->setRadius(nValue);
	}
}

void CMainView::setMatrix()
{
    const qreal iw = m_pScene->width();
    const qreal ih = m_pScene->height();
    const qreal wh = height();
    const qreal ww = width();
    
	qreal fScale = m_fScale * m_currentStepScaleFactor;
	QMatrix matrix;
    matrix.translate(ww/2, wh/2);
    matrix.translate(m_fTranslateX, m_fTranslateY);
    matrix.rotate(m_fRotationAngle);
	matrix.scale(fScale, fScale);
    matrix.translate(-iw/2, -ih/2);
    if (fScale < 0.5) {
        if (!m_pRectItem) m_pRectItem = m_pScene->GetRectItem();
        if (m_pRectItem) {
            QPen pen = m_pRectItem->pen();
            pen.setWidth(1 / fScale);
            pen.setStyle(Qt::DashLine);
            m_pRectItem->setPen(pen);
        }
    }
	CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
	if (pParent) {
		emit pParent->scaleChanged(m_fScale*100);
	}

    QGraphicsView::setMatrix(matrix);
}

void CMainView::zoomIn() // 放大
{
    m_fScale += m_nScaleStep/100.0f;

    if (m_fScale > 1.0)
    {
        m_fScale *= 1.5; // 大图后加大缩放比例
    }

	setMatrix();
}

void CMainView::zoomOut() // 缩小
{
	m_fScale -= m_nScaleStep/100.0f;

    if (m_fScale > 1.0)
    {
        m_fScale /= 1.5;
    }

	if (m_fScale < 0.01f) {
		m_fScale = 0.01f;
	}
	setMatrix();
}

void CMainView::setRotate(int n)
{
    m_fRotationAngle += n * 90;
    setMatrix();
}


void CMainView::setScale(int v_nScale)
{
	m_fScale = v_nScale / 100.0f;
	setMatrix();
}

void CMainView::setScene(QGraphicsScene *scene)
{
	QBrush brush(QColor(40, 40, 40));
	setBackgroundBrush(brush);// 修改View背景色
    QGraphicsView::setScene(scene);
	m_pScene = (CMainScene*)(this->scene());
}

void CMainView::EnabledScale(bool v_bEnabledScale /*= false*/)
{
	m_bEnabledScale = v_bEnabledScale;
}

void CMainView::EnabledDrag(bool v_bEnabledDrag /*= true*/)
{
    v_bEnabledDrag = true;
	if (m_pScene) {
		m_pScene->EnabledDrag(v_bEnabledDrag);
	}
}

void CMainView::EnabledSelected(bool v_bEnabledSelected)
{
	//m_bEnabledSelected = v_bEnabledSelected;
	if (m_pScene) {
		m_pScene->EnabledSelected(v_bEnabledSelected);
	}
}

void CMainView::SetScaleStep(int v_nScaleStep)
{
	m_nScaleStep = v_nScaleStep;
}

void CMainView::store()
{
    m_pScene->store();
}

void CMainView::showImage(bool bIsShowSrcImg, bool bCompareImg, bool bIsShowDetailImg, int nGainIndex, int nShowScale)
{
    if (m_pScene)
    {
        m_pScene->showImage(bIsShowSrcImg, bCompareImg, bIsShowDetailImg, nGainIndex, nShowScale);
    }
}

void CMainView::saveImage(QString strSavePath)
{
    m_pScene->saveImage(strSavePath);
}

void CMainView::setMatch()
{
	if (m_pScene) {
		centerOn(m_pScene->width() / 2, m_pScene->height() / 2);// 设置场景的中点在窗口中居中
		fitInView(0, 0, m_pScene->width(), m_pScene->height(), Qt::KeepAspectRatio);// 设置自适应
		m_pScene->SetMatch();

		QMatrix matrix = this->matrix();
		m_fScale = matrix.m11();
		CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
		if (pParent) {
			emit pParent->scaleChanged(m_fScale*100);
		}
	}
	update();
}

void CMainView::setMaster()
{
	if (m_pScene) {
		centerOn(m_pScene->width() / 2, m_pScene->height() / 2);// 设置场景的中点在窗口中居中
		fitInView(0, 0, m_pScene->width(), m_pScene->height(), Qt::KeepAspectRatio);// 设置自适应
		m_pScene->SetMatch();

		QMatrix matrix = this->matrix();
		m_fScale = matrix.m11();
		CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
		if (pParent) {
			emit pParent->scaleChanged(m_fScale*100);
		}
	}
	update();
}

QRectF CMainView::getSelectedArea()
{
    QRectF rc(0, 0, 0, 0);
    if (m_pRectItem) {
        rc = m_pRectItem->rect();
    }
    QString strToolTip;
    strToolTip.sprintf("area: (%.2f, %.2f, %.2f, %.2f)", rc.x(), rc.y(), rc.width(), rc.height());
    this->setToolTip(strToolTip);
    return  rc;
}

void CMainView::ClearCurSelectedArea()
{
    if (m_pRectItem)
    {
        m_pRectItem->setRect(QRect(0, 0, 0, 0));
    }
}

void CMainView::setCommandType(int v_nCommandType)
{
    m_nCommandType = v_nCommandType;
}

//bool CMainView::viewportEvent(QEvent* event)
//{
//    qDebug() << "event type: " << event->type();
//    if (event->type() == QEvent::Gesture) {
//        return this->gestrueEvent(static_cast<QGestureEvent*>(event));
//    }
//    return QGraphicsView::viewportEvent(event);
//}

bool CMainView::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
        return gestrueEvent(static_cast<QGestureEvent*>(event));
    return QGraphicsView::event(event);
}

bool CMainView::gestrueEvent(QGestureEvent* event)
{
    // Qt demo imagegesturess
    if (QGesture *pan = event->gesture(Qt::PanGesture))
        panTriggered(static_cast<QPanGesture *>(pan));
    else if (QGesture *pinch = event->gesture(Qt::PinchGesture))
        pinchTriggered(static_cast<QPinchGesture *>(pinch));
    else if (QGesture* swipe = event->gesture(Qt::SwipeGesture))
        swipeGesture(static_cast<QSwipeGesture*>(swipe));
    else {
    }
    
    return true;
}

void CMainView::panTriggered(QPanGesture* gesture)
{
#ifndef QT_NO_CURSOR
    switch (gesture->state()) {
        case Qt::GestureStarted:
        case Qt::GestureUpdated:
            setCursor(Qt::SizeAllCursor);
            break;
        default:
            setCursor(Qt::ArrowCursor);
    }
#endif
    QPointF delta = gesture->delta();
    m_fTranslateX += delta.x();
    m_fTranslateY += delta.y();
    setMatrix();
    update();
}
void CMainView::pinchTriggered(QPinchGesture* gesture)
{
    QPinchGesture::ChangeFlags changeFlags = gesture->changeFlags();
    if (changeFlags & QPinchGesture::RotationAngleChanged) {
        qreal rotationDelta = gesture->rotationAngle() - gesture->lastRotationAngle();
        m_fRotationAngle += rotationDelta;
        setMatrix();
    }
    if (changeFlags & QPinchGesture::ScaleFactorChanged) {
        m_currentStepScaleFactor = gesture->totalScaleFactor();
        setMatrix();
    }
    if (gesture->state() == Qt::GestureFinished) {
        m_fScale = m_fScale * m_currentStepScaleFactor;
        m_currentStepScaleFactor = 1.0f;
        setMatrix();
    }
    update();
}

void CMainView::swipeGesture(QSwipeGesture* swipe)
{
    if (swipe->state() == Qt::GestureFinished) {
        if (swipe->horizontalDirection() == QSwipeGesture::Left || swipe->verticalDirection() == QSwipeGesture::Up) {
            qDebug() << "swipeTriggered(): swipe to previous";
//            goPrevImage();
        } else {
            qDebug() << "swipeTriggered(): swipe to next";
//            goNextImage();
        }
//        update();
    }
}

void CMainView::wheelEvent(QWheelEvent *event)
{
    if (!m_bEnabledScale) return;

    if (event->angleDelta().y() > 0)
        this->zoomIn();
    else
        this->zoomOut();
}

void CMainView::mousePressEvent(QMouseEvent *event)
{
    // 当光标底下没有 item 时，才能移动
	m_bMouseTranslate = true;
    QPointF point = mapToScene(event->pos());
    if (scene()->itemAt(point, transform()) == NULL)  {
        
        m_lastMousePos = event->pos();
    }
    return QGraphicsView::mousePressEvent(event);
}

void CMainView::mouseReleaseEvent(QMouseEvent *event)
{
    m_bMouseTranslate = false;

    return QGraphicsView::mousePressEvent(event);
}



void CMainView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_bMouseTranslate)
    {
        QPointF mouseDelta = mapToScene(event->pos()) - mapToScene(m_lastMousePos);
        // 根据缩放调整
        mouseDelta *= m_fScale;

        // 根据旋转调整
        int nCase = 0;
        float fx = mouseDelta.x();
        float fy = mouseDelta.y();

        mouseDelta.setX(fx*cos(m_fRotationAngle*3.1415926/180.0) - fy*sin(m_fRotationAngle*3.1415926/180.0));
        mouseDelta.setY(fx*sin(m_fRotationAngle*3.1415926/180.0) + fy*cos(m_fRotationAngle*3.1415926/180.0));


        // view 根据鼠标下的点作为锚点来定位 scene
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        QPoint newCenter(VIEW_WIDTH / 2 - mouseDelta.x(),  VIEW_HEIGHT / 2 - mouseDelta.y());
        centerOn(mapToScene(newCenter));

        // scene 在 view 的中心点作为锚点
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
    }

    m_lastMousePos = event->pos();
    return QGraphicsView::mouseMoveEvent(event);
}


void CMainView::startPicker(int v_nType)
{
    m_bPicker = true;
    m_nPickType = v_nType;
    m_oldCursor = cursor();
    setCursor(m_cursor);
}

void CMainView::endPicker()
{
    m_bPicker = false;
    unsetCursor();
}

void CMainView::pickColor(QPointF v_ptMouse)
{
    QPointF pt = mapToScene(v_ptMouse.x(), v_ptMouse.y());
    QRectF rc(pt.x() - 40, pt.y() - 40, 80, 80);
    rc.intersects(scene()->sceneRect());
//    qDebug() << rc;
    CMainWindow* pParent = static_cast<CMainWindow*>(this->parent()->parent()->parent());
    if (pParent) {
//        pParent->pickColor(rc, m_nPickType);
    }
}
