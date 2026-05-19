#pragma once

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QPropertyAnimation>
#include <QColor>
#include <QOpenGLFunctions_3_3_Core>

class ShaderSpinnerWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT
    Q_PROPERTY(float angle READ angle WRITE setAngle)

public:
    explicit ShaderSpinnerWidget(QWidget* parent = nullptr);

    void start();
    void stop();

    void setColor(const QColor& c);
    QColor color() const { return m_color_; }

    float angle() const { return m_angle_; }
    void setAngle(float a);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QOpenGLShaderProgram m_program_;
    QPropertyAnimation* m_anim_ = nullptr;

    GLuint m_vao_ = 0;
    GLuint m_vbo_ = 0;

    QColor m_color_ = Qt::black;
    float m_angle_ = 0.0f;

    // Spinner parameters (relative)
    float m_radius_ = 0.9f;
    float m_thickness_ = 0.25f;
    float m_trail_ = 0.7f;
    float m_speed_ = 1.5f; // revolutions per second
};
