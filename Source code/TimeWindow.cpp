#include "TimeWindow.h"
#include <QApplication>
#include <QScreen>

TimeWindow::TimeWindow(QWidget* parent)
    : QWidget(parent),
    timeLabel(nullptr), dateLabel(nullptr), weekdayLabel(nullptr),
    datetimeTimer(nullptr),
    m_dragging(false), m_movable(true), m_dragPosition(0, 0)  // 默认可移动
{
    // 设置无边框窗口和透明背景
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置窗口位置和大小
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int windowWidth = 400;
    int windowHeight = 150;
    int xPos = screenGeometry.width() - windowWidth - 125; // 在课程表窗口左侧125像素
    setGeometry(xPos, 0, windowWidth, windowHeight);

    // 设置透明背景
    setStyleSheet("background: transparent;");

    // 创建布局和标签
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 时间标签
    timeLabel = new QLabel(this);
    timeLabel->setStyleSheet("font-size: 80px; font-weight: bold; color: black; background: transparent;");
    timeLabel->setAlignment(Qt::AlignLeft);
    timeLabel->setMinimumHeight(90);
    layout->addWidget(timeLabel);

    // 日期和星期标签
    QHBoxLayout* dateLayout = new QHBoxLayout();
    dateLayout->setContentsMargins(0, 0, 0, 0);
    dateLayout->setSpacing(20);

    dateLabel = new QLabel(this);
    dateLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: black; background: transparent;");

    weekdayLabel = new QLabel(this);
    weekdayLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: black; background: transparent;");

    dateLayout->addWidget(dateLabel);
    dateLayout->addWidget(weekdayLabel);
    dateLayout->addStretch();

    layout->addLayout(dateLayout);

    // 启动定时器
    datetimeTimer = new QTimer(this);
    connect(datetimeTimer, &QTimer::timeout, this, &TimeWindow::updateDateTime);
    datetimeTimer->start(1000);

    // 立即更新一次
    updateDateTime();

    // 确保窗口显示
    show();
    raise();
}

TimeWindow::~TimeWindow()
{
    if (datetimeTimer) {
        datetimeTimer->stop();
        datetimeTimer->deleteLater();
    }
}

void TimeWindow::updateDateTime()
{
    QDateTime now = QDateTime::currentDateTime();

    dateLabel->setText(now.toString("  yyyy年MM月dd日"));
    timeLabel->setText(now.toString("HH:mm:ss"));

    QStringList chineseWeekdays = { "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };
    int weekday = now.date().dayOfWeek() - 1;
    if (weekday >= 0 && weekday < chineseWeekdays.size()) {
        weekdayLabel->setText(chineseWeekdays[weekday]);
    }
}

// 设置透明度
void TimeWindow::setTransparency(double transparency)
{
    setWindowOpacity(transparency);
}

// 设置是否可移动
void TimeWindow::setMovable(bool movable)
{
    m_movable = movable;
}

// 鼠标按下事件 - 开始拖动
void TimeWindow::mousePressEvent(QMouseEvent* event)
{
    if (!m_movable) return; // 如果不可移动，直接返回

    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        m_dragPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
#else
        m_dragPosition = event->globalPos() - frameGeometry().topLeft();
#endif
        event->accept();
    }
}

// 鼠标移动事件 - 处理拖动
void TimeWindow::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_movable) return; // 如果不可移动，直接返回

    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        move(event->globalPosition().toPoint() - m_dragPosition);
#else
        move(event->globalPos() - m_dragPosition);
#endif
        event->accept();
    }
}

// 鼠标释放事件 - 结束拖动
void TimeWindow::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_movable) return; // 如果不可移动，直接返回

    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        event->accept();
    }
}