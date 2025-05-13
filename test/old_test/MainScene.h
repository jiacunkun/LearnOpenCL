#ifndef __MAINSCENE_H__
#define __MAINSCENE_H__
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QList>
#include <QKeyEvent>
#include <QPointF>
#include <opencv2/opencv.hpp>

#include "SingleImageDeniseHandle.h" // 调用降噪算法接口

class CGrayItem;
class CMainScene : public QGraphicsScene
{
public:
	CMainScene(QObject *parent);
	~CMainScene();

	int ImageDenoiseHandle(QString srcImage, BASE_SINGLE_IMAGE_PARAM param);

	bool OnOpen(QString v_strFileName);
	bool OnOpen(QImage v_image);
    bool CloseImage();

	void saveImage(QString m_strSaveFileName);
	void OnSave(QImage v_Image, QString v_strFileName);
    void showImage(bool bIsShowSrcImg, bool bCompareImg, bool bIsShowDetailImg = 0, int nGainIndex = 0, int nShowScale = 0);
    void store();
public:
	void EnabledDrag(bool v_bEnabledDrag = true);
	void EnabledSelected(bool v_bEnabledSelected = true);
	QGraphicsRectItem* GetRectItem();
	void SetMatch();
    cv::Mat QImageToMat(QImage image);
    QImage MatToQImage(const cv::Mat& mat);
    
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

private:
	bool m_bEnabledDrag;
	QString m_strFileName;
	QPointF m_ptStartpoint;
//    QString m_strSavePath;
    // 中间图像
    QImage m_srcQImage;
    QImage m_dstQImage;
//    QImage m_detailImage;
    QImage m_lastDstQImage;
    cv::Mat m_matBPFBuffer;
    cv::Mat m_matL1Buffer;
    cv::Mat m_matL2Buffer;
    QString m_strSaveFileName;
	QGraphicsPixmapItem* m_pItem;	//
	QGraphicsRectItem* m_pRectItem;
    bool m_bEnable;
};
#endif // !__MAINSCENE_H__
