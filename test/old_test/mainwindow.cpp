#include "mainwindow.h"
#include "MainScene.h"
#include "common.h"

#include <fstream>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>
#include <QtWidgets>
#include <stdio.h>


CMainWindow::CMainWindow(QWidget *parent)
        : QMainWindow(parent)
{
    ui.setupUi(this);
    setWindowTitle(tr("Denoise Tool"));
    ui.graphicsView->setAcceptDrops(false);
    setAcceptDrops(true);
    m_pScene = new CMainScene(this);
    m_pButtonGroup = NULL;
    ui.graphicsView->setScene(m_pScene);
    m_nIndex = -1;
    m_nCount = 0;

    // 控件初始化
    init();
    m_curPath = QCoreApplication::applicationDirPath();
    qDebug()<<QDir::currentPath();
    qDebug()<< QDir::homePath();
    qDebug()<<QDir::rootPath();
    qDebug()<<QCoreApplication::applicationDirPath();

    delegate();
    //initStyle();
}

CMainWindow::~CMainWindow()
{
    m_listFilePath.clear();
    SAFE_DELETE(m_pScene);
    SAFE_DELETE(m_pButtonGroup);

}

void CMainWindow::init()
{
    // 算法模块初始化
    m_bIsShowSrcImg = false;
    m_bIsDenoise = false;
    m_bIsChromeDenoise = false;
    m_bCompareImg = false;

    m_nYMethod = 3;
    m_nYIntensity = 5;
    m_nUVMethod = 1;
    m_nUVIntensity = 0;
    m_nSharpenIntensity = 0;

    m_param.lYReprocMethod = 3;
    m_param.lYReprocIntensity = 5;
    m_param.lYDetailLuma = 50;
    m_param.lUVReprocMethod = 1;
    m_param.lUVReprocIntensity = 0;
    m_param.lSharpenIntensity = 0;

}

void CMainWindow::delegate()
{
    // 按钮
    connect(ui.radioButton1_src, SIGNAL(toggled(bool)), this, SLOT(radioBtnToggled(bool))); // 原图显示
    //connect(ui.radioButton_1, SIGNAL(toggled(bool)), this, SLOT(radioBtnToggled(bool))); //
    //connect(ui.radioButton_2, SIGNAL(toggled(bool)), this, SLOT(radioBtnToggled(bool))); //

    connect(ui.BtnOpen, SIGNAL(clicked()), this, SLOT(OnBtnClicked()));
    connect(ui.BtnSave, SIGNAL(clicked()), this, SLOT(OnBtnClicked()));
    connect(ui.SaveImgButton, SIGNAL(clicked()), this, SLOT(OnBtnClicked())); // 暂存
    connect(ui.BtnRotate_1, SIGNAL(clicked()), this, SLOT(OnBtnClicked()));
    connect(ui.BtnRotate_2, SIGNAL(clicked()), this, SLOT(OnBtnClicked()));

    // 多态
    connect(ui.CompareButton, SIGNAL(stateChanged(int)), this, SLOT(OnCheckBoxChanged(int))); // 三态复选， 暂存对比
    connect(ui.checkBox, SIGNAL(stateChanged(int)), this, SLOT(OnCheckBoxChanged(int))); // 三态复选
    connect(ui.checkBox_1, SIGNAL(stateChanged(int)), this, SLOT(OnCheckBoxChanged(int))); // 三态复选
    connect(ui.checkBox_2, SIGNAL(stateChanged(int)), this, SLOT(OnCheckBoxChanged(int))); // 三态复选


    // 下拉
    connect(ui.comboBox_1, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurrentIndexChanged(int)));
    connect(ui.comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(OnCurrentIndexChanged(int)));

    // 滑杆
    connect(ui.SliderCNR1, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR2, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR3, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR8, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR4, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR5, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR6, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderCNR7, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));
    connect(ui.SliderDetailLuma, SIGNAL(valueChanged(int)), this, SLOT(OnSliderChanged(int)));

    connect(ui.SliderCNR1, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp())); // 鼠标按键释放后响应
    connect(ui.SliderCNR2, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR3, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR8, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR4, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR5, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR6, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderCNR7, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));
    connect(ui.SliderDetailLuma, SIGNAL(sliderReleased()), this, SLOT(OnSliderBtnUp()));

    // 文本框
    connect(ui.EdtCNR1, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR2, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR3, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR8, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR4, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR5, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtCNR6, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));
    connect(ui.EdtDetailLuma, SIGNAL(returnPressed()), this, SLOT(OnReturnPressed()));



    connect(this, SIGNAL(positionYChanged(int, float)), this, SLOT(OnPositionYChanged(int, float)));
}

void CMainWindow::initStyle()
{
    //加载样式表
    QFile file(":/qss/psblack.css");
    //QFile file(":/qss/flatwhite.css");
    //QFile file(":/qss/lightblue.css");
    if (file.open(QFile::ReadOnly)) {
        QString qss = QLatin1String(file.readAll());
        QString paletteColor = qss.mid(20, 7);
        qApp->setPalette(QPalette(QColor(paletteColor)));
        qApp->setStyleSheet(qss);
        file.close();
    }


    // 浮点型 范围：[-360, 360] 精度：小数点后2位
    QDoubleValidator* pDoubleValidator = new QDoubleValidator(this);
    pDoubleValidator->setNotation(QDoubleValidator::StandardNotation);
    pDoubleValidator->setDecimals(2);


    QIntValidator* pIntValidator = new QIntValidator(this);
    pIntValidator->setRange(0, 20);
    ui.EdtCNR1->setValidator(pIntValidator);
    ui.EdtCNR2->setValidator(pIntValidator);
    ui.EdtCNR3->setValidator(pIntValidator);
    ui.EdtCNR4->setValidator(pIntValidator);
    ui.EdtCNR5->setValidator(pIntValidator);
    ui.EdtCNR6->setValidator(pIntValidator);
}
/**
 CMainWindow::openBtnEvent() : open Buttoon 的事件函数
 */
void CMainWindow::openBtnEvent()
{
    QSettings setting("./Setting.ini", QSettings::IniFormat); // 记住上次打开的路径
    QString strLastPath = setting.value("LastFilePath").toString();
    m_strFilePath = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("打开图片"), strLastPath, tr("Images (*.jpg *.png *.bmp *.gif)"));
    //    QFileInfo fi(m_strFilePath);
    //    QString strFilePath = fi.path();// 获取文件所在目录
    //
    //    if (!strFilePath.isEmpty()) {
    //        m_listFilePath.clear();
    //        m_nCount = 0;
    //        m_nIndex = 0;
    //        FindFile(strFilePath);
    //    }

    if (!m_strFilePath.isEmpty())
    {
        open();
    }
    // UpdateStatusBar();
}


void CMainWindow::open()
{
    QFileInfo fi(m_strFilePath);
    m_strFileName = fi.baseName().toStdString();// 取文件名
    qDebug() << m_strFileName.c_str();

    if (fi.suffix() == "txt")
    {
        //readTxt();
    }
    else
    {
        ui.graphicsView->OpenImage(m_strFilePath);
    }

    UpdateStatusBar();
}



void CMainWindow::save()
{
    QString strFileName = m_strFilePath.mid(m_strFilePath.lastIndexOf("/") + 1);
    strFileName = strFileName.left(strFileName.lastIndexOf(".jpg"));
    qDebug() << strFileName;

    m_strSavePath = m_strFilePath.left(m_strFilePath.lastIndexOf("/") + 1);
    qDebug() << m_strSavePath;

    m_strSavePath = m_strSavePath + strFileName + "_nr.jpg";
    qDebug() << m_strSavePath;


    //    m_strSavePath = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("保存图像和文档"), m_strFilePath);
    m_strSavePath = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存"), m_strSavePath, QString(), nullptr, QFileDialog::ShowDirsOnly);
    qDebug() << m_strSavePath;


    ui.graphicsView->saveImage(m_strSavePath);
}


void CMainWindow::OnCheckBoxChanged(int state)
{
    Q_UNUSED(state);

    QCheckBox* pSender = qobject_cast<QCheckBox*>(sender());

    if (pSender == ui.checkBox_1)
    {
        if (state > 0)
        {
            m_bIsDenoise = true;
        }
        else
        {
            m_bIsDenoise = false;
        }

        if (m_bIsDenoise || m_bIsChromeDenoise)
        {
            ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
            ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
        }

    }
    else if (pSender == ui.checkBox_2)
    {
        if (state > 0)
        {
            m_bIsChromeDenoise = true;
        }
        else
        {
            m_bIsChromeDenoise = false;
        }

        if (m_bIsDenoise || m_bIsChromeDenoise)
        {
            ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
            ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
        }
    }
    else if (pSender == ui.CompareButton) // 暂存比较
    {
        if (state > 0)
        {
            m_bCompareImg = true;
            ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
        }
        else
        {
            m_bCompareImg = false;
            ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
        }
    }


}

void CMainWindow::OnBtnClicked()
{
    QPushButton* pSender = qobject_cast<QPushButton*>(sender());
    if (pSender)
    {
        if (pSender == ui.BtnOpen)
        {
            openBtnEvent();
        }
        else if (pSender == ui.BtnSave)
        {
            save();
        }
        else if (pSender == ui.SaveImgButton) // 暂存
        {
            ui.graphicsView->store();
        }
        else if (pSender == ui.BtnRotate_1)
        {
            ui.graphicsView->setRotate(-1);
        }
        else if (pSender == ui.BtnRotate_2)
        {
            ui.graphicsView->setRotate(1);
        }
    }
}

void CMainWindow::radioBtnToggled(bool v_bChecked)
{
    Q_UNUSED(v_bChecked);
    QRadioButton* pSender = qobject_cast<QRadioButton*>(sender());
    if (pSender)
    {
        if (pSender == ui.radioButton1_src)
        {
            m_bIsShowSrcImg = v_bChecked;
            if (ui.graphicsView)
            {
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);

            }
        }
//        else if (pSender == ui.radioButton_1)
//        {
//            printf("%d\n", v_bChecked);
//        }
//        else if (pSender == ui.radioButton_2)
//        {
//            printf("%d\n", v_bChecked);
//        }
    }
}

bool FindTxtFile(QString v_strFilePath, QStringList& liFilePath)
{
    QDir dir(v_strFilePath);
    if (!dir.exists())
        return false;
    dir.setFilter(QDir::Dirs | QDir::Files);//除了目录或文件，其他的过滤掉
    // 	dir.setSorting(QDir::DirsFirst);//优先显示目录
    QFileInfoList list = dir.entryInfoList();//获取文件信息列表
    for (int i = 0; i < list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        QString strFileName = fileInfo.fileName();
        if (strFileName == "." || strFileName == "..") {
            continue;
        }
        QString strFilePath = fileInfo.filePath();
        if (fileInfo.isDir()) {
            FindTxtFile(strFilePath, liFilePath);
        }
        else {
            QString strSuffix = fileInfo.suffix();
            if (strSuffix.compare("txt") == 0) {
                liFilePath.push_back(strFilePath);
            }
        }
    }
    return true;
}

void CMainWindow::OnCurrentIndexChanged(int v_nIndex)
{
    Q_UNUSED(v_nIndex);
    QComboBox* pSender = qobject_cast<QComboBox*>(sender());
    if (pSender)
    {
        int nValue = pSender->currentIndex();
        if (pSender == ui.comboBox_1)
        {
            m_param.lYReprocMethod = nValue;

            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        } else if (pSender == ui.comboBox_2)
        {
            m_param.lUVReprocMethod = nValue;

            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        }
    }
}

void CMainWindow::OnCurrentIndexChanged(QString v_strText)
{
    Q_UNUSED(v_strText);
    QComboBox* pSender = qobject_cast<QComboBox*>(sender());
    if (pSender) {
        printf("OnCurrentIndexChanged = %d", 1);
    }
}

void CMainWindow::OnCheckBoxStateChanged(int v_nState)
{
    Q_UNUSED(v_nState);
    QCheckBox* pSender = qobject_cast<QCheckBox*>(sender());
    if (pSender) {
        printf("OnCheckBoxStateChanged = %d", 1);
    }
}


void CMainWindow::OnSliderChanged(int v_nValue)
{
    Q_UNUSED(v_nValue);
    QSlider* pSender = qobject_cast<QSlider*>(sender());
    if (pSender) {
        int nValue = pSender->value();
        if (pSender == ui.SliderCNR1) {
            ui.EdtCNR1->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR2) {
            ui.EdtCNR2->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR3) {
            ui.EdtCNR3->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR8) {
            ui.EdtCNR8->setText(QString::number(float(nValue)/10.0));
        }
        else if (pSender == ui.SliderCNR4) {
            ui.EdtCNR4->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR5) {
            ui.EdtCNR5->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR6) {
            ui.EdtCNR6->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderCNR7) {
            ui.EdtCNR7->setText(QString::number(nValue));
        }
        else if (pSender == ui.SliderDetailLuma) {
            ui.EdtDetailLuma->setText(QString::number(nValue));
        }
    }
}


/**
 滑块鼠标弹起事件： 执行 RPE
 */
void CMainWindow::OnSliderBtnUp()
{
    QSlider* pSender = qobject_cast<QSlider*>(sender());
    if (pSender)
    {
        if (pSender == ui.SliderCNR1)
        {
            m_param.lYReprocIntensity = ui.SliderCNR1->value();
            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        }
        else if (pSender == ui.SliderCNR2)
        {
            m_param.lUVReprocIntensity = ui.SliderCNR2->value();
            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        }
        else if (pSender == ui.SliderCNR3)
        {
            m_param.lSharpenIntensity = ui.SliderCNR3->value();
            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        }
        else if (pSender == ui.SliderDetailLuma)
        {
            m_param.lYDetailLuma = ui.SliderDetailLuma->value();
            if (m_bIsDenoise || m_bIsChromeDenoise)
            {
                ui.graphicsView->ImageDenoiseHandle(m_strFilePath, m_param);
                ui.graphicsView->showImage(m_bIsShowSrcImg, m_bCompareImg);
            }
        }
    }
}




void CMainWindow::OnReturnPressed()
{
    QLineEdit* pSender = qobject_cast<QLineEdit*>(sender());
    if (pSender) {
        if (pSender == ui.EdtCNR1) {
            ui.SliderCNR1->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR2) {
            ui.SliderCNR2->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR3) {
            ui.SliderCNR3->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR8) {
            ui.SliderCNR8->setValue(pSender->text().toFloat()*10);
        }
        else if (pSender == ui.EdtCNR4) {
            ui.SliderCNR4->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR5) {
            ui.SliderCNR5->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR6) {
            ui.SliderCNR6->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtCNR7) {
            ui.SliderCNR7->setValue(pSender->text().toInt());
        }
        else if (pSender == ui.EdtDetailLuma) {
            ui.SliderDetailLuma->setValue(pSender->text().toInt());
        }



    }
}

void CMainWindow::OnPointsReturnPressed()
{
    QLineEdit* pSender = qobject_cast<QLineEdit*>(sender());
    if (pSender) {

    }
}

void CMainWindow::OnEdtTextChanged(QString v_strText)
{
    //    float fValue = v_strText.toFloat();
    for (int i = 0; i < 64; i++)
    {
        if (m_pEdtYValue[i])
        {
            float fValue = m_pEdtYValue[i]->text().toFloat();
            if (m_nGainIndex == 0)
            {

            }
        }

    }


}

void CMainWindow::OnPositionYChanged(int v_nIndex, float v_fYValue)
{
    if (m_pEdtYValue[v_nIndex])
    {
        float fValue = m_pEdtYValue[v_nIndex]->text().toFloat();
        if (m_nGainIndex == 0)
        {

        }

    }
}

void CMainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("text/uri-list")) {
        event->acceptProposedAction();
    }
}

void CMainWindow::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    QString tempStrFilePath = urls.first().toLocalFile();
    if (tempStrFilePath.isEmpty()) {
        return;
    }
    QFileInfo fi(tempStrFilePath);
    qDebug() << tempStrFilePath;
    QString strFilePath = fi.path();// 获取文件所在目录

    if (!strFilePath.isEmpty()) {
        FindFile(strFilePath);
    }
    if (!tempStrFilePath.isEmpty())
    {
        if (fi.suffix() == "txt")
        {

        }
        else
        {
            m_strFilePath = tempStrFilePath;
            ui.graphicsView->OpenImage(m_strFilePath);
        }
    }
    //    UpdateStatusBar();
    return;
}

bool CMainWindow::FindFile(QString v_strFilePath)
{
    QDir dir(v_strFilePath);
    if (!dir.exists())
        return false;

    dir.setFilter(/*QDir::Dirs | */QDir::Files);//除了目录或文件，其他的过滤掉
    // 	dir.setSorting(QDir::DirsFirst);//优先显示目录
    QFileInfoList list = dir.entryInfoList();//获取文件信息列表
    m_nIndex = 0;
    for (int i = 0; i<list.size(); i++) {
        QFileInfo fileInfo = list.at(i);
        QString strFileName = fileInfo.filePath();
        QString strSuffix = fileInfo.suffix();
        if (strSuffix.compare("jpg") == 0 || 0 == strSuffix.compare("png") || 0 == strSuffix.compare("gif") || 0 == strSuffix.compare("bmp"))
        {
            if (m_strFilePath == strFileName) {
                m_nIndex = m_listFilePath.size();
            }
            m_listFilePath.push_back(strFileName);
        }
    }
    m_nCount = m_listFilePath.size();

    return true;
}

void CMainWindow::UpdateStatusBar()
{
    QString strState;

    if(0 ==  m_listFilePath.size())
    {
        strState.sprintf("-- / --");
    }
    else
    {
        strState.sprintf("%d / %d", m_nIndex + 1, m_nCount);
    }
}

void CMainWindow::Back()
{
    if (m_listFilePath.size() < 1) {
        return;
    }
    m_nIndex--;
    if (m_nIndex < 0) {
        m_nIndex = m_nCount - 1;
    }
    m_strFilePath = m_listFilePath.at(m_nIndex);

    if (!m_strFilePath.isEmpty())
    {
        open();   //打开当前地址的下的Raw
    }

    UpdateStatusBar();
}

void CMainWindow::Next()
{
    if (m_listFilePath.size() < 1) return;
    m_nIndex++;
    if (m_nIndex > m_nCount-1) {
        m_nIndex = 0;
    }
    m_strFilePath = m_listFilePath.at(m_nIndex);

    if (!m_strFilePath.isEmpty())
    {
        open();  //打开当前地址的下的Raw
    }

    UpdateStatusBar();
}
