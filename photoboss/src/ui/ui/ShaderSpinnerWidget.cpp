#include "ui/ShaderSpinnerWidget.h"
#include <QOpenGLFunctions>

ShaderSpinnerWidget::ShaderSpinnerWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_TranslucentBackground);
    hide();

    m_anim = new QPropertyAnimation(this, "angle");
    m_anim->setStartValue(0.0f);
    m_anim->setEndValue(6.28318530718f); // 2π
    m_anim->setLoopCount(-1);
}

void ShaderSpinnerWidget::start() {
    show();
    m_anim->setDuration(int(1000.0f / m_speed));
    m_anim->start();
}

void ShaderSpinnerWidget::stop() {
    m_anim->stop();
}

void ShaderSpinnerWidget::setColor(const QColor& c) {
    m_color = c;
    update();
}

void ShaderSpinnerWidget::setAngle(float a) {
    m_angle = a;
    update();
}

void ShaderSpinnerWidget::initializeGL() {
    initializeOpenGLFunctions();

    // Fullscreen quad
    float verts[] = {
        -1, -1, 0, 0,
         1, -1, 1, 0,
         1,  1, 1, 1,
        -1, -1, 0, 0,
         1,  1, 1, 1,
        -1,  1, 0, 1
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        R"(
        #version 330 core
        layout(location = 0) in vec2 pos;
        layout(location = 1) in vec2 uv;
        out vec2 v_uv;
        void main() {
            v_uv = uv;
            gl_Position = vec4(pos, 0.0, 1.0);
        }
        )");

    m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        R"(
        #version 330 core
        in vec2 v_uv;
        out vec4 fragColor;

        uniform vec4 u_color;
        uniform float u_angle;
        uniform float u_trail;
        uniform float u_thickness;
        uniform float u_radius;
        uniform float u_aspect;

        void main() {
            vec2 p = v_uv * 2.0 - 1.0;
            p.x *= u_aspect;

            float r = length(p);
            float a = atan(p.y, p.x);
            if (a < 0.0) a += 6.28318530718;

            float maxR = u_radius;
            float minR = maxR * (1.0 - u_thickness);
            if (r < minR || r > maxR)
                discard;

            float d = u_angle - a;
            if (d < 0.0) d += 6.28318530718;

            float maxTrail = 6.28318530718 * u_trail;
            if (d > maxTrail)
                discard;

            float t = 1.0 - d / maxTrail;
            fragColor = vec4(u_color.rgb, u_color.a * t);
        }
        )");

    m_program.link();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void ShaderSpinnerWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    m_program.bind();

    float aspect = float(width()) / float(height());

    m_program.setUniformValue("u_color", m_color);
    m_program.setUniformValue("u_angle", m_angle);
    m_program.setUniformValue("u_trail", m_trail);
    m_program.setUniformValue("u_thickness", m_thickness);
    m_program.setUniformValue("u_radius", m_radius);
    m_program.setUniformValue("u_aspect", aspect);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    m_program.release();
}

void ShaderSpinnerWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}
