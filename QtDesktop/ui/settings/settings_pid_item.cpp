#include "settings_pid_item.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QCoreApplication>

REGISTER_SETTINGS_ITEM(SettingsPidItem, "pid")

SettingsPidItem::SettingsPidItem(QWidget *parent)
    : SettingsItemWidget(parent)
{
    setFixedHeight(52);

    QWidget *rightWidget = new QWidget(this);
    QHBoxLayout *rightLayout = new QHBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(6);

    m_input = new QLineEdit(this);
    m_input->setFixedWidth(100);
    m_input->setPlaceholderText("PID");
    m_input->setStyleSheet(
        "QLineEdit {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  background: #f5f5f5;"
        "  font-size: 12px;"
        "}"
    );

    m_pickBtn = new QPushButton(this);
    m_pickBtn->setFixedSize(28, 28);
    m_pickBtn->setToolTip("Pick a window");
    m_pickBtn->setStyleSheet(
        "QPushButton {"
        "  border: 1px solid #c0c0c0;"
        "  border-radius: 4px;"
        "  background: #f5f5f5;"
        "  font-size: 11px;"
        "}"
        "QPushButton:hover {"
        "  background: #e0e0e0;"
        "}"
    );
    m_pickBtn->setText("+");

    rightLayout->addWidget(m_input);
    rightLayout->addWidget(m_pickBtn);

    QHBoxLayout *lay = qobject_cast<QHBoxLayout *>(layout());
    if (lay) {
        lay->addWidget(rightWidget);
    }

    connect(m_input, &QLineEdit::textChanged, this, &SettingsPidItem::valueChanged);
    connect(m_pickBtn, &QPushButton::pressed, this, &SettingsPidItem::enterPickMode);
}

SettingsPidItem::~SettingsPidItem()
{
    if (m_picking)
        leavePickMode();
}

QString SettingsPidItem::pidText() const
{
    return m_input->text();
}

void SettingsPidItem::setPidText(const QString &text)
{
    m_input->setText(text);
    m_defaultPid = text;
}

void SettingsPidItem::apply()
{
    m_defaultPid = m_input->text();
    m_modified = false;
}

void SettingsPidItem::reset()
{
    m_input->setText(m_defaultPid);
    m_modified = false;
}

bool SettingsPidItem::isModified() const
{
    return m_input->text() != m_defaultPid;
}

void SettingsPidItem::configure(const QVariantMap &props)
{
    SettingsItemWidget::configure(props);
    if (props.contains("placeholder"))
        m_input->setPlaceholderText(QCoreApplication::translate("SettingsDialog",
            props["placeholder"].toString().toUtf8().constData()));
}

void SettingsPidItem::enterPickMode()
{
    m_picking = true;
    m_pickBtn->setDown(true);
    SetCapture(reinterpret_cast<HWND>(winId()));
}

void SettingsPidItem::leavePickMode()
{
    if (m_picking) {
        m_picking = false;
        ReleaseCapture();
        m_pickBtn->setDown(false);
    }
}

void SettingsPidItem::capturePidAtCursor()
{
    POINT pt;
    GetCursorPos(&pt);

    HWND hwnd = WindowFromPoint(pt);
    if (hwnd) {
        hwnd = GetAncestor(hwnd, GA_ROOT);
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid != 0)
            m_input->setText(QString::number(pid));
    }
}

bool SettingsPidItem::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    if (eventType != "windows_generic_MSG")
        return false;

    MSG *msg = static_cast<MSG *>(message);

    switch (msg->message) {
    case WM_SETCURSOR:
        if (m_picking) {
            SetCursor(LoadCursor(nullptr, IDC_CROSS));
            *result = TRUE;
            return true;
        }
        break;

    case WM_LBUTTONUP:
        if (m_picking) {
            capturePidAtCursor();
            leavePickMode();
            *result = 0;
            return true;
        }
        break;

    case WM_CAPTURECHANGED:
        if (m_picking && reinterpret_cast<HWND>(msg->lParam) != reinterpret_cast<HWND>(winId())) {
            leavePickMode();
        }
        break;

    case WM_KEYDOWN:
        if (m_picking && msg->wParam == VK_ESCAPE) {
            leavePickMode();
            *result = 0;
            return true;
        }
        break;
    }

    return QWidget::nativeEvent(eventType, message, result);
}
