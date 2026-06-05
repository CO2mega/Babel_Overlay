#include "scrollingtext.h"
#include <QPainter>
#include <QFontMetrics>
#include <QResizeEvent>

ScrollingText::ScrollingText(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QFont f = font();
    f.setPointSize(12);
    setFont(f);

    connect(&m_scrollTimer, &QTimer::timeout, this, [this]() {
        if (m_paused) {
            if (m_pauseTimer.hasExpired(m_pauseDuration)) {
                m_paused = false;
            } else {
                return;
            }
        }

        if (m_textWidth > width()) {
            m_scrollOffset += m_scrollStep;
            if (m_scrollOffset > m_textWidth) {
                m_scrollOffset = -width();
                m_paused = true;
                m_pauseTimer.start();
            }
        }
        update();
    });
}

void ScrollingText::setText(const QString &text)
{
    m_text = text;
    QFontMetrics fm(font());
    m_textWidth = fm.horizontalAdvance(text) + 20;
    m_scrollOffset = 0;
    m_paused = true;
    m_pauseTimer.start();
    update();
}

QString ScrollingText::text() const
{
    return m_text;
}

void ScrollingText::startScroll(int intervalMs)
{
    m_scrollTimer.start(intervalMs);
}

void ScrollingText::stopScroll()
{
    m_scrollTimer.stop();
}

void ScrollingText::setScrollStep(int pixels)
{
    m_scrollStep = pixels;
}

void ScrollingText::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(font());
    painter.setPen(Qt::black);

    if (m_textWidth > width()) {
        QRect textRect(-m_scrollOffset, 0, m_textWidth, height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
    } else {
        QRect textRect(0, 0, width(), height());
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
    }
}

void ScrollingText::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_scrollOffset = 0;
    m_paused = true;
    m_pauseTimer.start();
}
