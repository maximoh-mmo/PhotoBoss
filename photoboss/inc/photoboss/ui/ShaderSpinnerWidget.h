#pragma once

#include <QOpenGLWidget>
#include <QOpenGLShaderProgram>
#include <QPropertyAnimation>
#include <QColor>

class ShaderSpinnerWidget : public QOpenGLWidget {
    Q_OBJECT
    Q_PROPERTY(float angle READ angle WRITE setAngle)

public:
    explicit ShaderSpinnerWidget(QWidget* parent = nullptr);

    void start();
    void stop();

    void setColor(const QColor& c);
    QColor color() const { return m_color; }

    float angle() const { return m_angle; }
    void setAngle(float a);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    QOpenGLShaderProgram m_program;
    QPropertyAnimation* m_anim = nullptr;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    QColor m_color = Qt::black;
    float m_angle = 0.0f;

    // Spinner parameters (relative)
    float m_radius = 0.9f;
    float m_thickness = 0.25f;
    float m_trail = 0.7f;
    float m_speed = 1.5f; // revolutions per second
};
