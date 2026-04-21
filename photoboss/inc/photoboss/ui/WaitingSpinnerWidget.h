#pragma once

#include <QWidget>
#include <QTimer>
#include <QColor>

class WaitingSpinnerWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaitingSpinnerWidget(QWidget *parent = nullptr);
    ~WaitingSpinnerWidget() = default;

public slots:
    void start();
    void stop();

public:
    void setColor(const QColor &color);
    void setRoundness(qreal roundness);
    void setMinimumTrailOpacity(qreal minimumTrailOpacity);
    void setTrailFadePercentage(qreal trail);
    void setRevolutionsPerSecond(qreal revolutionsPerSecond);
    void setNumberOfLines(int lines);
    void setLineLength(int length);
    void setLineWidth(int width);
    void setInnerRadius(int radius);
    void setText(const QString &text);

    QColor color() const;
    qreal roundness() const;
    qreal minimumTrailOpacity() const;
    qreal trailFadePercentage() const;
    qreal revolutionsPerSecond() const;
    int numberOfLines() const;
    int lineLength() const;
    int lineWidth() const;
    int innerRadius() const;

    bool isSpinning() const;

signals:
    void stateChanged(bool spinning);

private slots:
    void rotate();

protected:
    void paintEvent(QPaintEvent *paintEvent) override;

private:
    static int lineCountDistanceFromPrimary(int current, int primary, int totalNrOfLines);
    static QColor currentLineColor(int distance, int totalNrOfLines,
                               qreal trailFadePerc, qreal minOpacity,
                               const QColor &color);

    void initialize();
    void updateSize();
    void updateTimer();

private:
    QColor  m_color;
    qreal   m_roundness;
    qreal   m_minimumTrailOpacity;
    qreal   m_trailFadePercentage;
    qreal   m_revolutionsPerSecond;
    int     m_numberOfLines;
    int     m_lineLength;
    int     m_lineWidth;
    int     m_innerRadius;

    QTimer *m_timer;
    int     m_currentCounter;
    bool    m_isSpinning;
};