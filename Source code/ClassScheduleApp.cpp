#define NOMINMAX
#include "ClassScheduleApp.h"
#include "TimeWindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QScreen>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QProcess>
#include <random>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winreg.h>
#endif

ClassScheduleApp::ClassScheduleApp(QWidget* parent)
    : QMainWindow(parent),
    centralWidget(nullptr), mainLayout(nullptr),
    courseListWidget(nullptr), courseListLayout(nullptr), courseScrollArea(nullptr),
    restartBtn(nullptr), closeBtn(nullptr),
    datetimeTimer(nullptr), topmostCheckTimer(nullptr), pixelShiftTimer(nullptr),
    timeWindow(nullptr),
    currentTopmostState(false), currentWeekday(-1), pixelShiftCount(0)
{
    qDebug() << "=== 应用程序启动 ===";

    // 加载设置
    loadSettings();

    // 设置无边框窗口和透明背景
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // 设置窗口位置和大小
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int windowWidth = 600;
    int windowHeight = screenGeometry.height();
    setGeometry(screenGeometry.width() - windowWidth, 0, windowWidth, windowHeight);

    // 设置透明度
    setWindowOpacity(settings.transparency);

    // 创建时间窗口
    timeWindow = new TimeWindow();

    // 初始检查状态
    bool initialTopmost = shouldBeTopmost();

    // 根据初始状态设置透明度和可移动性
    if (initialTopmost) {
        timeWindow->setTransparency(0.3); // 置顶模式下透明度为0.3
        timeWindow->setMovable(false); // 置顶模式下不可移动
    }
    else {
        timeWindow->setTransparency(settings.transparency); // 使用用户设置的透明度
        timeWindow->setMovable(true); // 非置顶模式下可移动
    }

    // 确保时间窗口显示
    timeWindow->show();
    timeWindow->raise();

    setupUI();

    // 初始检查状态
    toggleDisplayMode(initialTopmost);
    currentTopmostState = initialTopmost;

    startTimers();
    setAutoStart();

    qDebug() << "=== 应用程序初始化完成 ===";
}

ClassScheduleApp::~ClassScheduleApp()
{
    // 停止所有定时器
    if (datetimeTimer) {
        datetimeTimer->stop();
        datetimeTimer->deleteLater();
    }
    if (topmostCheckTimer) {
        topmostCheckTimer->stop();
        topmostCheckTimer->deleteLater();
    }
    if (pixelShiftTimer) {
        pixelShiftTimer->stop();
        pixelShiftTimer->deleteLater();
    }

    // 删除时间窗口
    if (timeWindow) {
        timeWindow->close();
        timeWindow->deleteLater();
    }

    saveSettings();
}

void ClassScheduleApp::setupUI()
{
    try {
        qDebug() << "开始设置UI...";

        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        // 设置透明背景
        centralWidget->setStyleSheet("background: transparent;");

        mainLayout = new QVBoxLayout(centralWidget);
        mainLayout->setContentsMargins(15, 10, 15, 10); // 恢复正常边距
        mainLayout->setSpacing(8); // 恢复正常间距

        // 控制按钮行
        QHBoxLayout* controlLayout = new QHBoxLayout();
        controlLayout->setContentsMargins(0, 0, 0, 0);

        restartBtn = new QPushButton("重启", centralWidget);
        closeBtn = new QPushButton("关闭", centralWidget);

        // 设置按钮样式
        QString buttonStyle = "QPushButton { "
            "background: rgba(255, 255, 255, 180); "
            "border: 1px solid #cccccc; "
            "padding: 6px 12px; "
            "border-radius: 4px; "
            "font-weight: bold;"
            "font-size: 12px;"
            "}"
            "QPushButton:hover { "
            "background: rgba(255, 255, 255, 220); "
            "}";

        restartBtn->setStyleSheet(buttonStyle);
        closeBtn->setStyleSheet(buttonStyle);

        connect(restartBtn, &QPushButton::clicked, this, &ClassScheduleApp::restartApp);
        connect(closeBtn, &QPushButton::clicked, this, &QApplication::quit);

        controlLayout->addStretch();
        controlLayout->addWidget(restartBtn);
        controlLayout->addWidget(closeBtn);

        mainLayout->addLayout(controlLayout);

        // 课程列表区域 - 恢复正常间距
        courseScrollArea = new QScrollArea(centralWidget);
        courseScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        courseScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        courseScrollArea->setWidgetResizable(true);
        courseScrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
            "QScrollBar:vertical { background: rgba(200, 200, 200, 100); width: 8px; border-radius: 4px; }"
            "QScrollBar::handle:vertical { background: rgba(100, 100, 100, 150); border-radius: 4px; min-height: 20px; }");

        // 创建课程列表容器
        courseListWidget = new QWidget();
        courseListWidget->setStyleSheet("background: transparent;");
        courseListLayout = new QVBoxLayout(courseListWidget);
        courseListLayout->setAlignment(Qt::AlignTop);
        courseListLayout->setSpacing(4); // 恢复正常间距
        courseListLayout->setContentsMargins(0, 5, 0, 5); // 恢复正常边距

        courseScrollArea->setWidget(courseListWidget);

        // 设置课程表区域的高度策略，让它占据所有剩余空间
        courseScrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mainLayout->addWidget(courseScrollArea, 1); // 添加拉伸因子

        // 初始创建课程列表
        createCourseList();

        qDebug() << "UI设置完成";

    }
    catch (const std::exception& e) {
        qDebug() << "UI setup error:" << e.what();
        QMessageBox::critical(this, "错误", QString("UI设置失败: %1").arg(e.what()));
    }
    catch (...) {
        qDebug() << "UI setup unknown error";
        QMessageBox::critical(this, "错误", "UI设置发生未知错误");
    }
}

void ClassScheduleApp::loadSettings()
{
    qDebug() << "=== 开始加载设置 ===";

    // 使用应用程序目录的绝对路径
    QString settingsPath = QCoreApplication::applicationDirPath() + "/class_schedule_settings.json";
    QFile file(settingsPath);

    qDebug() << "设置文件路径:" << settingsPath;

    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        qDebug() << "找到设置文件，文件大小:" << file.size();

        QByteArray data = file.readAll();
        file.close();

        if (!data.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isNull()) {
                QJsonObject obj = doc.object();

                settings.transparency = obj.value("transparency").toDouble(1.0);
                settings.dateFontSize = obj.value("date_font_size").toInt(16);
                settings.timeFontSize = obj.value("time_font_size").toInt(48);
                settings.courseFontSize = obj.value("course_font_size").toInt(28);

                qDebug() << "透明度设置:" << settings.transparency;
                qDebug() << "日期字体大小:" << settings.dateFontSize;
                qDebug() << "时间字体大小:" << settings.timeFontSize;
                qDebug() << "课程字体大小:" << settings.courseFontSize;

                // 加载时间段设置
                QJsonArray timeRanges = obj.value("topmost_time_ranges").toArray();
                settings.topmostTimeRanges.clear();
                qDebug() << "时间段数量:" << timeRanges.size();
                for (const QJsonValue& value : timeRanges) {
                    QJsonObject range = value.toObject();
                    TimeRange tr;
                    tr.start = range.value("start").toString("08:00");
                    tr.end = range.value("end").toString("12:00");
                    settings.topmostTimeRanges.push_back(tr);
                    qDebug() << "时间段:" << tr.start << "-" << tr.end;
                }

                // 加载课程表
                QJsonObject schedules = obj.value("schedules").toObject();
                QStringList weekdays = { "Monday", "Tuesday", "Wednesday", "Thursday",
                                       "Friday", "Saturday", "Sunday" };

                qDebug() << "JSON中的课程表键:" << schedules.keys();

                for (const QString& day : weekdays) {
                    if (schedules.contains(day)) {
                        QJsonArray courses = schedules.value(day).toArray();
                        QStringList courseList;
                        for (const QJsonValue& course : courses) {
                            courseList.append(course.toString());
                        }
                        settings.schedules[day] = courseList;
                        qDebug() << day << "的课程数量:" << courseList.size();
                    }
                    else {
                        // 如果没有该星期的课程表，使用默认值
                        qDebug() << day << "没有课程表，使用默认值";
                        QStringList defaultCourses = { "早读", "第一节", "第二节", "第三节", "第四节",
                                                     "第五节", "限时一", "第六节", "第七节", "第八节",
                                                     "限时二", "限时三", "第九节", "第十节", "第十一节" };
                        settings.schedules[day] = defaultCourses;
                    }
                }

                // 应用透明度设置到时间窗口（仅在非置顶模式下）
                if (timeWindow && !currentTopmostState) {
                    timeWindow->setTransparency(settings.transparency);
                }

                QStringList loadedDays;
                for (const auto& pair : settings.schedules) {
                    loadedDays.append(pair.first);
                }
                qDebug() << "已加载的星期:" << loadedDays;

                qDebug() << "设置加载成功";
                return; // 成功加载，直接返回
            }
        }
    }

    // 如果文件不存在或读取失败，创建默认设置
    qDebug() << "使用默认设置";
    createDefaultSettings();
}

void ClassScheduleApp::createDefaultSettings()
{
    settings.transparency = 1.0;
    settings.dateFontSize = 16;
    settings.timeFontSize = 48;
    settings.courseFontSize = 28;

    // 默认时间段
    settings.topmostTimeRanges.clear();
    settings.topmostTimeRanges.push_back({ "08:00", "12:00" });
    settings.topmostTimeRanges.push_back({ "14:00", "18:00" });

    // 默认课程表
    QStringList defaultCourses = { "早读", "第一节", "第二节", "第三节", "第四节",
                                 "第五节", "限时一", "第六节", "第七节", "第八节",
                                 "限时二", "限时三", "第九节", "第十节", "第十一节" };

    QStringList weekdays = { "Monday", "Tuesday", "Wednesday", "Thursday",
                           "Friday", "Saturday", "Sunday" };
    for (const QString& day : weekdays) {
        settings.schedules[day] = defaultCourses;
    }

    // 保存默认设置
    saveSettings();
}

void ClassScheduleApp::saveSettings()
{
    // 使用应用程序目录的绝对路径
    QString settingsPath = QCoreApplication::applicationDirPath() + "/class_schedule_settings.json";
    QFile file(settingsPath);

    qDebug() << "保存设置到:" << settingsPath;

    QJsonObject obj;
    obj["transparency"] = settings.transparency;
    obj["date_font_size"] = settings.dateFontSize;
    obj["time_font_size"] = settings.timeFontSize;
    obj["course_font_size"] = settings.courseFontSize;

    // 保存时间段
    QJsonArray timeRanges;
    for (const TimeRange& range : settings.topmostTimeRanges) {
        QJsonObject rangeObj;
        rangeObj["start"] = range.start;
        rangeObj["end"] = range.end;
        timeRanges.append(rangeObj);
    }
    obj["topmost_time_ranges"] = timeRanges;

    // 保存课程表
    QJsonObject schedules;
    for (const auto& pair : settings.schedules) {
        QJsonArray courses;
        for (const QString& course : pair.second) {
            courses.append(course);
        }
        schedules[pair.first] = courses;
    }
    obj["schedules"] = schedules;

    QJsonDocument doc(obj);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "设置保存成功";
    }
    else {
        qDebug() << "保存设置失败:" << file.errorString();
        qDebug() << "错误详情:" << file.error();
    }
}

void ClassScheduleApp::createCourseList()
{
    qDebug() << "=== 开始创建课程列表 ===";

    if (!courseListLayout) {
        qDebug() << "错误: courseListLayout 为空!";
        return;
    }

    // 清除现有的课程标签
    QLayoutItem* item;
    int removedCount = 0;
    while ((item = courseListLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
            removedCount++;
        }
        delete item;
    }
    qDebug() << "清除了" << removedCount << "个旧课程项";

    QStringList weekdays = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" };
    int currentDay = QDateTime::currentDateTime().date().dayOfWeek() - 1;

    qDebug() << "当前星期索引:" << currentDay;

    if (currentDay < 0 || currentDay >= weekdays.size()) {
        qDebug() << "错误的星期索引:" << currentDay;
        return;
    }

    QString currentWeekday = weekdays[currentDay];
    qDebug() << "当前星期英文:" << currentWeekday;

    // 检查设置中是否有该星期的课程
    QStringList availableDays;
    for (const auto& pair : settings.schedules) {
        availableDays.append(pair.first);
    }
    qDebug() << "可用的星期:" << availableDays;

    if (settings.schedules.find(currentWeekday) != settings.schedules.end()) {
        const QStringList& courses = settings.schedules[currentWeekday];
        qDebug() << "找到课程表，课程数量:" << courses.size();

        for (int i = 0; i < courses.size(); i++) {
            const QString& course = courses[i];
            qDebug() << "课程" << i << ":" << course;

            if (!course.isEmpty()) {
                QLabel* courseLabel = new QLabel(course, courseListWidget);
                courseLabel->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: black; background: transparent;").arg(settings.courseFontSize));
                courseLabel->setAlignment(Qt::AlignRight);
                courseLabel->setMinimumHeight(40); // 恢复正常高度
                courseListLayout->addWidget(courseLabel);
                qDebug() << "已创建课程标签:" << course;
            }
        }
    }
    else {
        qDebug() << "未找到" << currentWeekday << "的课程表，使用默认课程";
        // 创建默认课程表
        QStringList defaultCourses = { "语文", "数学", "英语", "物理", "化学", "生物" };
        for (const QString& course : defaultCourses) {
            QLabel* courseLabel = new QLabel(course, courseListWidget);
            courseLabel->setStyleSheet(QString("font-size: %1px; font-weight: bold; color: black; background: transparent;").arg(settings.courseFontSize));
            courseLabel->setAlignment(Qt::AlignRight);
            courseLabel->setMinimumHeight(40); // 恢复正常高度
            courseListLayout->addWidget(courseLabel);
        }
    }

    // 添加弹性空间
    courseListLayout->addStretch();
    qDebug() << "=== 课程列表创建完成 ===";
}

void ClassScheduleApp::toggleDisplayMode(bool isTopmost)
{
    qDebug() << "切换显示模式: isTopmost =" << isTopmost;

    if (isTopmost) {
        // 置顶模式：隐藏课程表窗口，只显示时间窗口
        hide(); // 隐藏课程表窗口

        if (timeWindow) {
            timeWindow->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
            // 置顶模式下透明度设为0.3，并且不可移动
            timeWindow->setTransparency(0.3);
            timeWindow->setMovable(false); // 禁用移动
            timeWindow->show();
            timeWindow->raise();
        }

        currentTopmostState = true;
        qDebug() << "切换到置顶模式：隐藏课程表窗口，只显示时间窗口，透明度0.3，不可移动";
    }
    else {
        // 正常模式：显示课程表窗口
        show();
        raise();

        // 设置课程表窗口透明度
        setWindowOpacity(settings.transparency);

        if (timeWindow) {
            timeWindow->setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
            timeWindow->setTransparency(settings.transparency); // 使用用户设置的透明度
            timeWindow->setMovable(true); // 启用移动
            timeWindow->show();
            timeWindow->raise();
        }

        currentTopmostState = false;
        qDebug() << "切换到正常模式：显示课程表窗口，透明度" << settings.transparency << "，可移动";
    }
}

bool ClassScheduleApp::shouldBeTopmost()
{
    QTime currentTime = QTime::currentTime();
    qDebug() << "当前时间:" << currentTime.toString("HH:mm:ss");

    for (const TimeRange& range : settings.topmostTimeRanges) {
        QTime startTime = QTime::fromString(range.start, "HH:mm");
        QTime endTime = QTime::fromString(range.end, "HH:mm");

        qDebug() << "检查时间段:" << range.start << "-" << range.end;

        // 检查当前时间是否在时间段内
        if (startTime <= endTime) {
            // 正常时间段（同一天内）
            if (currentTime >= startTime && currentTime <= endTime) {
                qDebug() << "当前时间在置顶时间段内:" << range.start << "-" << range.end;
                return true;
            }
        }
        else {
            // 跨天时间段（例如 22:00 到 06:00）
            if (currentTime >= startTime || currentTime <= endTime) {
                qDebug() << "当前时间在跨天置顶时间段内:" << range.start << "-" << range.end;
                return true;
            }
        }
    }

    qDebug() << "当前时间不在任何置顶时间段内";
    return false;
}

void ClassScheduleApp::startTimers()
{
    // 星期检查定时器 - 用于检查星期变化并更新课程表
    datetimeTimer = new QTimer(this);
    connect(datetimeTimer, &QTimer::timeout, this, &ClassScheduleApp::updateDateTime);
    datetimeTimer->start(60000); // 1分钟检查一次星期变化

    // 置顶状态检查定时器
    topmostCheckTimer = new QTimer(this);
    connect(topmostCheckTimer, &QTimer::timeout, this, &ClassScheduleApp::checkTopmostStatus);
    topmostCheckTimer->start(1000); // 1秒检查一次

    // 防烧屏定时器
    pixelShiftTimer = new QTimer(this);
    connect(pixelShiftTimer, &QTimer::timeout, this, &ClassScheduleApp::pixelShift);
    pixelShiftTimer->start(300000); // 5分钟

    // 立即更新一次
    updateDateTime();
    checkTopmostStatus();
}

void ClassScheduleApp::pixelShift()
{
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(-maxPixelShift, maxPixelShift);

    int shiftX = dis(gen);
    int shiftY = dis(gen);

    int windowWidth = 600;
    int newX = std::max(0, std::min(screenGeometry.width() - windowWidth,
        screenGeometry.width() - windowWidth + shiftX));
    int newY = std::max(0, std::min(0 + shiftY, 10));

    // 只有在显示状态下才移动课程表窗口
    if (isVisible()) {
        setGeometry(newX, newY, windowWidth, screenGeometry.height());
    }

    // 同时移动时间窗口 - 在课程表窗口左侧100像素
    if (timeWindow) {
        int timeWindowX = newX - 100;
        int timeWindowY = std::max(0, std::min(50 + shiftY, 60)); // 稍微下移
        timeWindow->setGeometry(timeWindowX, timeWindowY, windowWidth, 150);
    }

    pixelShiftCount++;

    qDebug() << "防烧屏像素偏移:" << shiftX << "," << shiftY;
}

void ClassScheduleApp::restartApp()
{
    qDebug() << "重启应用程序";
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}

void ClassScheduleApp::setAutoStart()
{
#ifdef Q_OS_WIN
    // Windows 平台的开机自启实现
    QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    QString appName = QFileInfo(appPath).baseName();

    QSettings bootUpSettings("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", QSettings::NativeFormat);

    // 检查是否已经设置了开机自启
    if (bootUpSettings.contains(appName)) {
        qDebug() << "开机自启已设置";
    }
    else {
        // 设置开机自启
        bootUpSettings.setValue(appName, appPath);
        qDebug() << "已设置开机自启，应用路径:" << appPath;
    }
#elif defined(Q_OS_LINUX)
    // Linux 平台的开机自启实现（需要创建 .desktop 文件）
    QString autostartDir = QDir::homePath() + "/.config/autostart";
    QDir dir;
    if (!dir.exists(autostartDir)) {
        dir.mkpath(autostartDir);
    }

    QString desktopFile = autostartDir + "/class_schedule_app.desktop";
    QFile file(desktopFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Exec=" << QCoreApplication::applicationFilePath() << "\n";
        out << "Hidden=false\n";
        out << "NoDisplay=false\n";
        out << "X-GNOME-Autostart-enabled=true\n";
        out << "Name=Class Schedule App\n";
        out << "Comment=Class schedule application\n";
        file.close();
        qDebug() << "Linux 开机自启已设置";
    }
#elif defined(Q_OS_MAC)
    // macOS 平台的开机自启实现
    QString autostartDir = QDir::homePath() + "/Library/LaunchAgents";
    QDir dir;
    if (!dir.exists(autostartDir)) {
        dir.mkpath(autostartDir);
    }

    QString plistFile = autostartDir + "/com.yourcompany.class_schedule_app.plist";
    QFile file(plistFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
        out << "<plist version=\"1.0\">\n";
        out << "<dict>\n";
        out << "    <key>Label</key>\n";
        out << "    <string>com.yourcompany.class_schedule_app</string>\n";
        out << "    <key>ProgramArguments</key>\n";
        out << "    <array>\n";
        out << "        <string>" << QCoreApplication::applicationFilePath() << "</string>\n";
        out << "    </array>\n";
        out << "    <key>RunAtLoad</key>\n";
        out << "    <true/>\n";
        out << "</dict>\n";
        out << "</plist>\n";
        file.close();
        qDebug() << "macOS 开机自启已设置";
    }
#endif
}

void ClassScheduleApp::checkTopmostStatus()
{
    try {
        bool requireTopmost = shouldBeTopmost();
        qDebug() << "检查置顶状态: 当前状态 =" << currentTopmostState << ", 需要状态 =" << requireTopmost;

        if (requireTopmost != currentTopmostState) {
            qDebug() << "需要切换置顶状态";
            toggleDisplayMode(requireTopmost);
            qDebug() << "置顶状态切换完成";
        }
    }
    catch (const std::exception& e) {
        qDebug() << "切换置顶状态时出错:" << e.what();
    }
}

void ClassScheduleApp::updateDateTime()
{
    // 这个函数现在只用于检查星期变化并更新课程表
    QDateTime now = QDateTime::currentDateTime();

    QStringList chineseWeekdays = { "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };
    int weekday = now.date().dayOfWeek() - 1;

    // 检查星期变化
    if (currentWeekday != weekday) {
        currentWeekday = weekday;
        createCourseList();
        qDebug() << "星期变化，更新课程表:" << chineseWeekdays[weekday];
    }
}

void ClassScheduleApp::updateFontSizes()
{
    // 这个函数现在只需要更新课程表的字体大小
    // 时间窗口的字体大小在 TimeWindow 类中处理
    qDebug() << "更新字体大小 - 日期:" << settings.dateFontSize
        << "时间:" << settings.timeFontSize
        << "课程:" << settings.courseFontSize;
}

void ClassScheduleApp::setTimeWindowTransparency(double transparency)
{
    if (timeWindow) {
        timeWindow->setTransparency(transparency);
    }
}