#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include "subtitlewidget.h"
#include "settingsdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    SubtitleWidget *subtitleWidget;
    SettingsDialog *settingsDialog;
    QPoint dragPosition;

    // 用于记录缩放状态的枚举和变量
    enum ResizeDirection { None, Right, Bottom, BottomRight };
    ResizeDirection resizeDir = None;
    bool isResizing = false;
    bool isDragging = false;

    void setupUi(QMainWindow *MainWindow);
};
#endif // MAINWINDOW_H
