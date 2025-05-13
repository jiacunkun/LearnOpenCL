#ifndef __MAINWINDOW_H__
#define __MAINWINDOW_H__

#include <QtWidgets/QMainWindow>
#include <QButtonGroup>
#include "ui_mainwindow.h"


QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QLabel;
QT_END_NAMESPACE

enum ToolType {
    eToolType_Arrow = 0,
    eToolType_Drag,
    eToolType_Rectangle,
    eToolType_Scale,
    eToolType_Picker,
    eToolType_Count
};

class CMainScene;
class CMainWindow : public QMainWindow
{
Q_OBJECT

public:
    CMainWindow(QWidget *parent = Q_NULLPTR);
    ~CMainWindow();
    void init();
    void delegate();
    void initStyle();

signals:
    void scaleChanged(int);     // 自定义信号
    void positionYChanged(int v_nIndex, float v_fYValue);

public slots:
    void openBtnEvent();

    void save();
    void OnBtnClicked();// 按钮单击事件
    void radioBtnToggled(bool v_bChecked);// radiobutton togged
    void OnCurrentIndexChanged(int v_nIndex);// 下拉框选中改变
    void OnCurrentIndexChanged(QString v_strText);
    void OnCheckBoxStateChanged(int v_nState);// 复选框选中改变

    void OnSliderChanged(int v_nValue);  // 滑动条改变事件
    void OnSliderBtnUp();                // 滑动条鼠标弹起事件

    void OnEdtTextChanged(QString v_strText);
    void OnReturnPressed();// 编辑框回车键按下
    void OnPointsReturnPressed();
    void OnPositionYChanged(int v_nIndex, float v_fYValue);
    //    void InitEdtLut();// 初始化EdtLut的值
    void OnCheckBoxChanged(int state);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void open();
    bool FindFile(QString v_strFilePath);
    void UpdateStatusBar();
    void Back();
    void Next();


private:
    CMainScene* m_pScene;
    Ui::MainWindowClass ui;
    QButtonGroup* m_pButtonGroup;
    QLineEdit* m_pEdtYValue[64];
    QLineEdit* m_pEdtRadialValue[8];

    int m_nIndex;// 当前显示的图片索引
    int m_nCount;// 总图片数
    // 算法逻辑
    bool m_bIsShowSrcImg;
    bool m_bIsDenoise;
    bool m_bCompareImg;
    bool m_bIsChromeDenoise;
    int m_nYMethod;
    int m_nYIntensity;
    int m_nYDetailLuma;
    int m_nUVMethod;
    int m_nUVIntensity;
    int m_nSharpenIntensity;
    BASE_SINGLE_IMAGE_PARAM m_param;

    // 之前算法逻辑
    bool m_bIsShowDetailImg;
    bool m_bEnable;
    int m_nGainIndex;
    int m_nKernelIndexL1;
    int m_nKernelIndexL2;
    int m_nKernelIndexBpf;
    float m_fRadialLut[8];
    int m_nShowScale;
    int m_nLuma;
    int m_nContrast;
    int m_nDetail;
    float m_nSmooth;
    int m_nChroma;
    int m_nCDtail;
    int m_nCSmooth;
    int m_nStep;
    float m_fL1Factor;
    float m_fL2Factor;
    float m_fBPFFactorL1;
    float m_fBPFFactorL2;
    float m_fL1ClampMax;
    float m_fL1ClampMin;
    float m_fL2ClampMax;
    float m_fL2ClampMin;
    float m_fGainLevelLut[192];
    float m_fGainWeightL1Lut[192];
    float m_fGainWeightL2Lut[192];

    // 文件名和路径
    std::string m_strFileName;	// 文件名，不含后缀
    QString m_strFilePath;		// 当前打开的图片
    QString m_strJsonName;		// Json文件名
    QStringList m_listFilePath;	// 图片文件列表
    QString m_curPath;
    QString m_strSavePath;
};

#endif // !__MAINWINDOW_H__
