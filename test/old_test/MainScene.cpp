#include "MainScene.h"
#include "common.h"
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include "QtDebug"
#include "MainItem.h"
#include <opencv2/opencv.hpp>
#include <cstring>
#include <QTextStream>
#include <iostream>
#include <iomanip>

using namespace std;
CMainScene::CMainScene(QObject *parent)
        : QGraphicsScene(parent)
{
    m_strFileName = "";
    m_bEnabledDrag = false;
    m_ptStartpoint = QPointF(0.0, 0.0);
    //    mtTuningTool = new MTTuningTool();
    m_pItem = new QGraphicsPixmapItem();
    // 	m_pItem->setFlag(QGraphicsItem::ItemIsMovable);
    addItem(m_pItem);
    m_pRectItem = new QGraphicsRectItem();
    // 	m_pRectItem->setVisible(true);
    // 	m_pRectItem->setZValue(999.0f);
    // 	m_pRectItem->setPen(QPen(Qt::blue));
    addItem(m_pRectItem);
    m_bEnable = 0;
    // 初始化去噪和去彩噪
}

CMainScene::~CMainScene()
{
    clear();
}



int CMainScene::ImageDenoiseHandle(QString srcImage, BASE_SINGLE_IMAGE_PARAM param)
{
    int ret = 0;
//    BASE_SINGLE_IMAGE_PARAM param;
//    {
//        param.lUVReprocIntensity = UV_Intensity;
//        param.lUVReprocMethod = UV_Method;
//        param.lYReprocIntensity = Y_Intensity;
//        param.lYReprocMethod = Y_Method;
//        param.lYDetailLuma = Y_DetailLuma;
//        param.lSharpenIntensity = Sharpen_Intensity;
//    }
    SingleImageDenoiseHandle denoiseHandle;
    ret = denoiseHandle.single_image_denoise_handle(srcImage, param);

    m_srcQImage = MatToQImage(denoiseHandle.m_srcMatImage);
    m_dstQImage = MatToQImage(denoiseHandle.m_dstMatImage);

    return ret;
}


bool CMainScene::OnOpen(QString v_strFileName)
{
    if (m_pItem) {
        qDebug() << v_strFileName;
        QImage image(v_strFileName);
        int nWidth = image.width();
        int nHeight = image.height();
        qDebug() << nHeight << nWidth;
        return OnOpen(image);
    }

    return false;
}


bool CMainScene::OnOpen(QImage v_image)
{
    QList<QGraphicsItem*> items = this->items();
    for (int i = 0; i < items.size(); i++) {
        QGraphicsItem* pItem = items.at(i);
        if ((pItem->type() == CMainItem::Type) || (pItem->type() == QGraphicsItemGroup::Type)) {
            this->removeItem(pItem);
        }
    }
    if (m_pItem) {
        m_pItem->setPixmap(QPixmap::fromImage(v_image));
        setSceneRect(m_pItem->pixmap().rect());
        //        m_pItem->setFlag(QGraphicsItem::ItemIsSelectable);
        m_pItem->setAcceptHoverEvents(true);
        update();

        return true;
    }

    return false;
}

bool CMainScene::CloseImage()
{
    if (m_pItem) {
        QPixmap pixmap;
        m_pItem->setPixmap(pixmap);
        update();
        return  true;
    }
    return  false;
}

void CMainScene::OnSave(QImage v_Image, QString v_strFileName)
{
    // QImage image(width(), height(), QImage::Format_RGB888);
    // image = image.convertToFormat(QImage::Format_RGB888, Qt::NoFormatConversion);
    // QPainter painter(&v_Image);
    // painter.setRenderHint(QPainter::Antialiasing);
    // this->render(&painter);   //关键函数
    // painter.end();

    v_Image.save(v_strFileName, nullptr, 100);
}


void CMainScene::EnabledDrag(bool v_bEnabledDrag /*= true*/)
{
    m_bEnabledDrag = v_bEnabledDrag;
    if (m_pItem) {
        m_pItem->setFlag(QGraphicsItem::ItemIsMovable, v_bEnabledDrag);
    }
}

void CMainScene::EnabledSelected(bool v_bEnabledSelected /*= true*/)
{
    if (m_pRectItem) {
        m_pRectItem->setVisible(v_bEnabledSelected);
    }
}

QGraphicsRectItem* CMainScene::GetRectItem()
{
    return m_pRectItem;
}

void CMainScene::SetMatch()
{
    if (m_pItem) {
        m_pItem->setPos(0, 0);
    }
}

void CMainScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    m_ptStartpoint = event->pos();
    event->ignore();
    return QGraphicsScene::mousePressEvent(event);
}

void CMainScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_pItem) {
            if (m_pRectItem) {
                m_pRectItem->setRect(QRectF(m_ptStartpoint, event->pos()));
            }
        }
    }
    event->ignore();
    return QGraphicsScene::mouseMoveEvent(event);
}


void CMainScene::store()
{
    //保存上一次图像结果
    m_lastDstQImage = m_dstQImage;
}

void CMainScene::saveImage(QString m_strSaveFileName)
{
    //传入图像路径
    QString imageName = m_strSaveFileName;
    qDebug() << imageName;
    //保存处理后结果图
    //m_dstQImage.load(imageName);
    OnSave(m_dstQImage, imageName);

}

void CMainScene::showImage(bool bIsShowSrcImg, bool bCompareImg, bool bIsShowDetailImg, int nGainIndex, int nShowScale)
{
    if (bIsShowSrcImg)
    {
        m_pItem->setPixmap(QPixmap::fromImage(m_srcQImage));
    }
    else
    {
        if (bCompareImg)
        {
            m_pItem->setPixmap(QPixmap::fromImage(m_lastDstQImage));
        }
        else
        {
            m_pItem->setPixmap(QPixmap::fromImage(m_dstQImage));
        }
    }
}

cv::Mat CMainScene::QImageToMat(QImage image)
{
    cv::Mat mat;
    switch (image.format())
    {
        case QImage::Format_ARGB32:
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
            break;
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);//COLOR_BGR2RGB
            break;
        case QImage::Format_Indexed8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
            break;
        default:
            break;
    }
    return mat;
}

QImage CMainScene::MatToQImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
        // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        //        qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        //        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}
