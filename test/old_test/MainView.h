#ifndef __MAINVIEW_H__
#define __MAINVIEW_H__
#include <qgraphicsview.h>
#include <QPointF>
#include <QGraphicsRectItem>
#include <QPainterPath>
#include <QGestureEvent>
#include <QPanGesture>
#include <QSwipeGesture>
#include <QPinchGesture>
#include "MainScene.h"


class CMainView : public QGraphicsView
{
	Q_OBJECT
public:
	CMainView(QWidget* parent = nullptr);
	~CMainView();

    void Init();
	void OpenImage(QString v_strFileName);
	void OpenImage(QImage v_Image);
    int ImageDenoiseHandle(QString srcImage, BASE_SINGLE_IMAGE_PARAM param);

	void CloseImage();
	void saveImage(QString strSavePath);
	void OnSave(QImage v_Image, QString v_strFileName);
	void setRadius(int nValue);

	void setRotate(int);
	void setMatrix(); // 只能缩放和旋转
	void zoomIn();
	void zoomOut();
	void setScale(int v_nScale);
	void setScene(QGraphicsScene *scene);


public:
	void EnabledScale(bool v_bEnabledScale = false);
	void EnabledDrag(bool v_bEnabledDrag = true);
	void EnabledSelected(bool v_bEnabledSelected);// 设置是否允许矩形框选
	void SetScaleStep(int v_nScaleStep);
    void showImage(bool bIsShowSrcImg, bool bCompareImg, bool bIsShowDetailImg = 0, int nGainIndex = 0, int nShowScale = 0);
    void store();
	void setMatch();
	void setMaster();

    QRectF getSelectedArea();
    void ClearCurSelectedArea();
    void setCommandType(int v_nCommandType);// 设置操作类型
    
public:
    void startPicker(int v_nType);
    void endPicker();
    void pickColor(QPointF v_ptMouse);

protected:
    void wheelEvent(QWheelEvent *event) override; // 鼠标滚轮缩放
	void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;
//    bool viewportEvent(QEvent *event) override; // 查看事件
    bool gestrueEvent(QGestureEvent* event);
    void panTriggered(QPanGesture* gesture);
    void pinchTriggered(QPinchGesture* gesture);
    void swipeGesture(QSwipeGesture* swipe);
private:
    bool m_bPicker;
    int m_nPickType;
    bool m_bMouseTranslate;
    QPoint m_lastMousePos;
    QCursor m_cursor;
    QCursor m_oldCursor;
    
	QPointF m_ptStartPoint;
    QPointF m_ptLasttPoint;
	qreal m_fScale = 1.0f;
    qreal m_currentStepScaleFactor = 1.0f;
    qreal m_fTranslateX;
    qreal m_fTranslateY;
    qreal m_fRotationAngle;
    
	int m_nScaleStep = 10;
	bool m_bEnabledScale;
	bool m_bEnabledSelected;
    int m_nCommandType;
	CMainScene* m_pScene;
	QGraphicsRectItem* m_pRectItem;

};


#endif // !__MAINVIEW_H__
