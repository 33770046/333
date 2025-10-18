#ifndef TIMEWINDOW_H
#define TIMEWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QDateTime>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

class TimeWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TimeWindow(QWidget* parent = nullptr);
    ~TimeWindow();

    // 设置透明度
    void setTransparency(double transparency);

    // 设置是否可移动
    void setMovable(bool movable);

private slots:
    void updateDateTime();

protected:
    // 鼠标事件处理
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QLabel* timeLabel;
    QLabel* dateLabel;
    QLabel* weekdayLabel;
    QTimer* datetimeTimer;

    // 拖动相关变量
    bool m_dragging;
    bool m_movable; // 控制是否可移动
    QPoint m_dragPosition;
};

#endif // TIMEWINDOW_H