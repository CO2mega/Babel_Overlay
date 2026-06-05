#ifndef SCROLLINGTEXT_H
#define SCROLLINGTEXT_H

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

class ScrollingText : public QWidget
{
    Q_OBJECT

public:
    explicit ScrollingText(QWidget *parent = nullptr);
    void setText(const QString &text);
    QString text() const;

    void startScroll(int intervalMs = 30);
    void stopScroll();
    void setScrollStep(int pixels);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QString m_text;
    int m_scrollOffset = 0;
    int m_scrollStep = 1;
    int m_textWidth = 0;
    QTimer m_scrollTimer;
    QElapsedTimer m_pauseTimer;
    bool m_paused = false;
    int m_pauseDuration = 2000;
};

#endif // SCROLLINGTEXT_H
