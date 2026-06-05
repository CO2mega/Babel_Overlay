#include "subtitlewidget.h"
#include <QPainter>
#include <QVBoxLayout>

SubtitleWidget::SubtitleWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    m_originalText = new ScrollingText(this);
    m_translation = new ScrollingText(this);

    layout->addWidget(m_originalText);
    layout->addWidget(m_translation);
}

void SubtitleWidget::setOriginalText(const QString &text)
{
    m_originalText->setText(text);
    m_originalText->startScroll();
}

void SubtitleWidget::setTranslation(const QString &text)
{
    m_translation->setText(text);
    m_translation->startScroll();
}

void SubtitleWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int margin = 20;
    int y = height() / 2;

    painter.setPen(QPen(QColor(0xD3, 0xD3, 0xD3), 1));
    painter.drawLine(margin, y, width() - margin, y);
}
