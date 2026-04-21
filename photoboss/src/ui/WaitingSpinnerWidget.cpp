#include "ui/WaitingSpinnerWidget.h"

#include <cmath>
#include <algorithm>

#include <QPainter>

WaitingSpinnerWidget::WaitingSpinnerWidget(QWidget *parent)
    : QWidget(parent),
      m_color(Qt::black),
      m_roundness(100.0),
      m_minimumTrailOpacity(15.0),
      m_trailFadePercentage(80.0),
      m_revolutionsPerSecond(1.0),
      m_numberOfLines(20),
      m_lineLength(10),
      m_lineWidth(2),
      m_innerRadius(10),
      m_currentCounter(0),
      m_isSpinning(false) {
    initialize();
}

void WaitingSpinnerWidget::initialize() {
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(rotate()));
    updateSize();
    updateTimer();
    hide();
}

void WaitingSpinnerWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_currentCounter >= m_numberOfLines) {
        m_currentCounter = 0;
    }

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < m_numberOfLines; ++i) {
        painter.save();
        painter.translate(m_innerRadius + m_lineLength,
                     m_innerRadius + m_lineLength);

        qreal rotateAngle = static_cast<qreal>(360 * i) / static_cast<qreal>(m_numberOfLines);
        painter.rotate(rotateAngle);
        painter.translate(m_innerRadius, 0);

        int distance = lineCountDistanceFromPrimary(i, m_currentCounter, m_numberOfLines);
        QColor color = currentLineColor(distance, m_numberOfLines, m_trailFadePercentage,
                                        m_minimumTrailOpacity, m_color);
        painter.setBrush(color);

        painter.drawRoundedRect(
            QRect(0, -m_lineWidth / 2, m_lineLength, m_lineWidth),
            m_roundness, m_roundness, Qt::RelativeSize);

        painter.restore();
    }
}

void WaitingSpinnerWidget::start() {
    m_isSpinning = true;
    show();
    emit stateChanged(true);

    if (!m_timer->isActive()) {
        m_timer->start();
        m_currentCounter = 0;
    }
}

void WaitingSpinnerWidget::stop() {
    m_isSpinning = false;
    hide();
    emit stateChanged(false);

    if (m_timer->isActive()) {
        m_timer->stop();
        m_currentCounter = 0;
    }
}

void WaitingSpinnerWidget::setNumberOfLines(int lines) {
    m_numberOfLines = lines;
    m_currentCounter = 0;
    updateTimer();
}

void WaitingSpinnerWidget::setLineLength(int length) {
    m_lineLength = length;
    updateSize();
}

void WaitingSpinnerWidget::setLineWidth(int width) {
    m_lineWidth = width;
    updateSize();
}

void WaitingSpinnerWidget::setInnerRadius(int radius) {
    m_innerRadius = radius;
    updateSize();
}

QColor WaitingSpinnerWidget::color() const {
    return m_color;
}

qreal WaitingSpinnerWidget::roundness() const {
    return m_roundness;
}

qreal WaitingSpinnerWidget::minimumTrailOpacity() const {
    return m_minimumTrailOpacity;
}

qreal WaitingSpinnerWidget::trailFadePercentage() const {
    return m_trailFadePercentage;
}

qreal WaitingSpinnerWidget::revolutionsPerSecond() const {
    return m_revolutionsPerSecond;
}

int WaitingSpinnerWidget::numberOfLines() const {
    return m_numberOfLines;
}

int WaitingSpinnerWidget::lineLength() const {
    return m_lineLength;
}

int WaitingSpinnerWidget::lineWidth() const {
    return m_lineWidth;
}

int WaitingSpinnerWidget::innerRadius() const {
    return m_innerRadius;
}

bool WaitingSpinnerWidget::isSpinning() const {
    return m_isSpinning;
}

void WaitingSpinnerWidget::setRoundness(qreal roundness) {
    m_roundness = std::max(0.0, std::min(100.0, roundness));
}

void WaitingSpinnerWidget::setColor(const QColor &color) {
    m_color = color;
}

void WaitingSpinnerWidget::setRevolutionsPerSecond(qreal revolutionsPerSecond) {
    m_revolutionsPerSecond = revolutionsPerSecond;
    updateTimer();
}

void WaitingSpinnerWidget::setTrailFadePercentage(qreal trail) {
    m_trailFadePercentage = trail;
}

void WaitingSpinnerWidget::setMinimumTrailOpacity(qreal minimumTrailOpacity) {
    m_minimumTrailOpacity = minimumTrailOpacity;
}

void WaitingSpinnerWidget::setText(const QString &) {
    // Not implemented - placeholder for future text support
}

void WaitingSpinnerWidget::rotate() {
    ++m_currentCounter;
    if (m_currentCounter >= m_numberOfLines) {
        m_currentCounter = 0;
    }
    update();
}

void WaitingSpinnerWidget::updateSize() {
    int size = (m_innerRadius + m_lineLength) * 2;
    setFixedSize(size, size);
}

void WaitingSpinnerWidget::updateTimer() {
    m_timer->setInterval(1000 / (m_numberOfLines * m_revolutionsPerSecond));
}

int WaitingSpinnerWidget::lineCountDistanceFromPrimary(int current, int primary, int totalNrOfLines) {
    int distance = primary - current;
    if (distance < 0) {
        distance += totalNrOfLines;
    }
    return distance;
}

QColor WaitingSpinnerWidget::currentLineColor(int countDistance, int totalNrOfLines,
                                           qreal trailFadePerc, qreal minOpacity,
                                           const QColor &color) {
    if (countDistance == 0) {
        return color;
    }

    const qreal minAlphaF = minOpacity / 100.0;
    int distanceThreshold = static_cast<int>(std::ceil((totalNrOfLines - 1) * trailFadePerc / 100.0));

    QColor resultColor = color;
    if (countDistance > distanceThreshold) {
        resultColor.setAlphaF(minAlphaF);
    } else {
        qreal alphaDiff = color.alphaF() - minAlphaF;
        qreal gradient = alphaDiff / static_cast<qreal>(distanceThreshold + 1);
        qreal resultAlpha = color.alphaF() - gradient * countDistance;
        resultAlpha = std::min(1.0, std::max(0.0, resultAlpha));
        resultColor.setAlphaF(resultAlpha);
    }

    return resultColor;
}