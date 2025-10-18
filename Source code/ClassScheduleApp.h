#ifndef CLASS_SCHEDULE_APP_H
#define CLASS_SCHEDULE_APP_H

#include <QMainWindow>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include <map>
#include <algorithm>

// 前向声明
class TimeWindow;

struct TimeRange {
    QString start;
    QString end;

    TimeRange(const QString& s = "", const QString& e = "") : start(s), end(e) {}
};

struct ScheduleSettings {
    double transparency = 1.0;
    int dateFontSize = 16;
    int timeFontSize = 48;
    int courseFontSize = 28;
    std::vector<TimeRange> topmostTimeRanges;
    std::map<QString, QStringList> schedules;
};

class ClassScheduleApp : public QMainWindow
{
    Q_OBJECT

public:
    ClassScheduleApp(QWidget* parent = nullptr);
    ~ClassScheduleApp();

private slots:
    void updateDateTime();
    void restartApp();
    void checkTopmostStatus();
    void updateFontSizes();
    void pixelShift();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void createCourseList();
    void toggleDisplayMode(bool isTopmost);
    bool shouldBeTopmost();
    void startTimers();
    void setAutoStart();
    void createDefaultSettings();

    // UI 组件
    QWidget* centralWidget;
    QVBoxLayout* mainLayout;

    // 设置时间窗口透明度
    void setTimeWindowTransparency(double transparency);

    // 课程列表相关
    QWidget* courseListWidget;
    QVBoxLayout* courseListLayout;
    QScrollArea* courseScrollArea;

    // 控制按钮
    QPushButton* restartBtn;
    QPushButton* closeBtn;

    // 定时器
    QTimer* datetimeTimer;
    QTimer* topmostCheckTimer;
    QTimer* pixelShiftTimer;

    // 时间窗口
    TimeWindow* timeWindow;

    // 应用状态
    ScheduleSettings settings;
    bool currentTopmostState;
    int currentWeekday;
    int pixelShiftCount;
    const int maxPixelShift = 3;
};

#endif // CLASS_SCHEDULE_APP_H