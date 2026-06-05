#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi(this);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi(QMainWindow *MainWindow)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // setMouseTracking(true);

    setMinimumSize(450, 100);
    resize(800, 140);

    QWidget *centralWidget = new QWidget(this);
    // centralWidget->setMouseTracking(true);

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 15, 15, 15);
    mainLayout->setSpacing(10);

    subtitleWidget = new SubtitleWidget(centralWidget);
    subtitleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QString widgetStyle =
        "SubtitleWidget {"
        "  border: 1px solid #D3D3D3;"
        "  background-color: transparent;"
        "}";
    subtitleWidget->setStyleSheet(widgetStyle);


    QPushButton *settingsBtn = new QPushButton("⚙", this);
    QPushButton *closeBtn = new QPushButton("✕", this);

    settingsBtn->setFixedSize(30, 30);
    closeBtn->setFixedSize(30, 30);

    QString btnStyle =
        "QPushButton { border: none; font-size: 16px; color: #555555; background: transparent; }"
        "QPushButton:hover { color: #000000; background-color: #e0e0e0; border-radius: 4px; }";
    settingsBtn->setStyleSheet(btnStyle);
    closeBtn->setStyleSheet(btnStyle);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    settingsDialog = new SettingsDialog(this);

    connect(settingsBtn, &QPushButton::clicked, this, [this]() {
        settingsDialog->show();
        settingsDialog->raise();
        settingsDialog->activateWindow();
    });

    QHBoxLayout *btnLayout = new QHBoxLayout(); 
    btnLayout->setContentsMargins(0, 0, 0, 0);
    btnLayout->setSpacing(5);
    btnLayout->addWidget(settingsBtn);
    btnLayout->addWidget(closeBtn);
    
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addLayout(btnLayout);
    rightLayout->addStretch(); 

    mainLayout->addWidget(subtitleWidget);
    mainLayout->addLayout(rightLayout);

    setCentralWidget(centralWidget);
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); 

    QPainterPath path;
    path.addRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);

    painter.fillPath(path, QColor(250, 250, 250));  // 填充背景色
    painter.setPen(QPen(QColor(180, 180, 180), 1)); // 边框颜色与宽度
    painter.drawPath(path);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        if (resizeDir != None)
        {
            isResizing = true;
        }
        else
        {
            isDragging = true; 
            dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 1. 拖拽移动状态
    if (isDragging && (event->buttons() & Qt::LeftButton))
    {
        move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
        return;
    }

    // 2. 边缘缩放状态
    if (isResizing && (event->buttons() & Qt::LeftButton))
    {
        QRect geo = geometry();
        QPoint globalPos = event->globalPosition().toPoint();

        if (resizeDir == Right || resizeDir == BottomRight) {
            geo.setWidth(qMax(minimumWidth(), globalPos.x() - geo.x()));
        }
        if (resizeDir == Bottom || resizeDir == BottomRight) {
            geo.setHeight(qMax(minimumHeight(), globalPos.y() - geo.y()));
        }

        setGeometry(geo);
        event->accept();
        return;
    }

    // 3. 悬停状态 
    if (!isResizing && !isDragging)
    {
        QPoint pos = event->pos();
        int margin = 8; 

        bool inWindow = rect().contains(pos);
        bool onRight = inWindow && pos.x() >= width() - margin;
        bool onBottom = inWindow && pos.y() >= height() - margin;

        if (onRight && onBottom) {
            resizeDir = BottomRight;
            setCursor(Qt::SizeFDiagCursor); 
        } else if (onRight) {
            resizeDir = Right;
            setCursor(Qt::SizeHorCursor); 
        } else if (onBottom) {
            resizeDir = Bottom;
            setCursor(Qt::SizeVerCursor); 
        } else {
            resizeDir = None;
            setCursor(Qt::ArrowCursor); 
        }
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::LeftButton) {
        // 切断正在进行的拖拽/缩放行为
        isResizing = false; 
        isDragging = false; 

        QPoint pos = event->pos();
        int margin = 8;
        bool inWindow = rect().contains(pos);
        bool onRight = inWindow && pos.x() >= width() - margin;
        bool onBottom = inWindow && pos.y() >= height() - margin;

        if (onRight && onBottom) {
            resizeDir = BottomRight;
            setCursor(Qt::SizeFDiagCursor);
        } else if (onRight) {
            resizeDir = Right;
            setCursor(Qt::SizeHorCursor);
        } else if (onBottom) {
            resizeDir = Bottom;
            setCursor(Qt::SizeVerCursor);
        } else {
            resizeDir = None;
            setCursor(Qt::ArrowCursor);
        }

        event->accept();
    }
}