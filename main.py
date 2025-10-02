# -*- coding: utf-8 -*-
import tkinter as tk
import time
from datetime import datetime
import os
import sys
import winreg
import random
import ctypes
import json
from ctypes import wintypes, Structure, POINTER
from ctypes.wintypes import DWORD, HICON, HWND, UINT
import threading

# 隐藏控制台窗口
if getattr(sys, 'frozen', False):
    ctypes.windll.user32.ShowWindow(ctypes.windll.kernel32.GetConsoleWindow(), 0)

# 设置文件路径
SETTINGS_FILE = "class_schedule_settings.json"

def load_settings():
    """加载设置"""
    default_settings = {
        "transparency": 1.0,
        "topmost_time_ranges": [
            {"start": "08:00", "end": "12:00"},
            {"start": "14:00", "end": "18:00"}
        ],
        "schedules": {
            "Monday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Tuesday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Wednesday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Thursday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Friday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Saturday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"],
            "Sunday": ["早读", "第一节", "第二节", "第三节", "第四节", "第五节", "限时一", "第六节", "第七节", "第八节", "限时二", "限时三", "第九节", "第十节", "第十一节"]
        }
    }
    
    try:
        if os.path.exists(SETTINGS_FILE):
            with open(SETTINGS_FILE, 'r', encoding='utf-8') as f:
                settings = json.load(f)
                # 确保所有星期都有课程表
                for day in ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]:
                    if day not in settings["schedules"]:
                        settings["schedules"][day] = default_settings["schedules"][day]
                # 确保有时间段设置
                if "topmost_time_ranges" not in settings:
                    settings["topmost_time_ranges"] = default_settings["topmost_time_ranges"]
                return settings
    except Exception as e:
        print(f"加载设置失败: {e}")
    
    return default_settings

def save_settings(settings):
    """保存设置"""
    try:
        with open(SETTINGS_FILE, 'w', encoding='utf-8') as f:
            json.dump(settings, f, ensure_ascii=False, indent=2)
        print("设置保存成功")
    except Exception as e:
        print(f"保存设置失败: {e}")

# 设置开机自启
def set_auto_start():
    try:
        if getattr(sys, 'frozen', False):
            app_path = sys.executable
        else:
            app_path = os.path.abspath(sys.argv[0])
        
        key = winreg.HKEY_CURRENT_USER
        subkey = r"Software\Microsoft\Windows\CurrentVersion\Run"
        
        with winreg.OpenKey(key, subkey, 0, winreg.KEY_SET_VALUE) as reg_key:
            winreg.SetValueEx(reg_key, "ClassScheduleApp", 0, winreg.REG_SZ, app_path)
            print("开机自启设置成功")
    except Exception as e:
        print(f"开机自启设置失败: {e}")

class ClassScheduleApp:
    def __init__(self, root):
        self.root = root
        self.root.title("课程表")
        
        # 加载设置
        self.settings = load_settings()
        
        # 设置纯透明背景
        self.root.attributes('-transparentcolor', 'white')
        self.root.configure(bg='white')
        
        # 应用透明度设置
        self.root.attributes('-alpha', self.settings["transparency"])
        
        # 设置窗口始终置顶（先设置，后面会取消）
        self.root.attributes('-topmost', True)
        
        # 获取屏幕尺寸
        screen_width = self.root.winfo_screenwidth()
        screen_height = self.root.winfo_screenheight()
        
        # 窗口尺寸 - 增加到400以适应更大的字体和时间显示
        window_width = 400
        window_height = screen_height
        
        # 窗口位置 - 紧贴右侧
        x = screen_width - window_width
        y = 0
        
        self.root.geometry(f"{window_width}x{window_height}+{x}+{y}")
        
        # 移除窗口边框和标题栏
        self.root.overrideredirect(True)
        
        # 将窗口置于最底层
        self.root.attributes('-topmost', False)
        
        # 防烧屏相关变量
        self.pixel_shift_count = 0
        self.max_pixel_shift = 3
        self.shift_interval = 300
        
        # 当前星期几
        self.current_weekday = None
        
        # 设置窗口相关变量
        self.settings_window = None
        self.course_frames = {}  # 存储所有选项卡的框架
        self.current_visible_tab = None  # 当前显示的选项卡
        
        # 存储用户设置的透明度
        self.user_transparency = self.settings["transparency"]
        
        # UI组件引用
        self.main_frame = None
        self.date_label = None
        self.time_label = None
        self.weekday_label = None
        
        # 当前置顶状态
        self.current_topmost_state = False
        
        # 设置窗口变量（初始化为None）
        self.transparency_var = None
        self.time_range_vars = []  # 存储时间段变量
        
        print("=== 程序启动 ===")
        print(f"当前时间: {datetime.now().strftime('%H:%M:%S')}")
        print(f"置顶时间段: {self.settings['topmost_time_ranges']}")
        
        self.setup_ui()
        self.update_datetime()
        self.start_anti_burnin()
        
        # 立即检查并应用置顶状态
        self.check_topmost_status()
        self.start_topmost_check()
        
        # 设置开机自启
        set_auto_start()
        
    def setup_ui(self):
        print("正在设置UI...")
        
        # 第一行：控制按钮
        control_frame = tk.Frame(self.root, bg='white', height=40)
        control_frame.pack(pady=(5, 0), padx=15, fill='x')
        
        # 在右侧平放三个按钮
        button_frame = tk.Frame(control_frame, bg='white')
        button_frame.pack(side='right')
        
        # 设置按钮
        settings_btn = tk.Button(button_frame, text="设置", font=("Microsoft YaHei", 10, "bold"),
                                command=self.show_settings, bg='lightblue', fg='black',
                                width=5, height=1)
        settings_btn.pack(side='left', padx=(5, 0))
        
        # 重启按钮
        restart_btn = tk.Button(button_frame, text="重启", font=("Microsoft YaHei", 10, "bold"),
                               command=self.restart_app, bg='lightgreen', fg='black',
                               width=5, height=1)
        restart_btn.pack(side='left', padx=(5, 0))
        
        # 关闭按钮
        close_btn = tk.Button(button_frame, text="关闭", font=("Microsoft YaHei", 10, "bold"),
                             command=self.quit_application, bg='lightcoral', fg='black',
                             width=5, height=1)
        close_btn.pack(side='left', padx=(5, 0))
        
        # 第二行：日期
        top_frame1 = tk.Frame(self.root, bg='white')
        top_frame1.pack(pady=(0, 2), padx=15, fill='x')
        
        self.date_label = tk.Label(top_frame1, text="", 
                                  font=("Microsoft YaHei", 24, "bold"),
                                  bg='white', fg='black', anchor='e')
        self.date_label.pack(fill='x')
        
        # 第三行：时间
        top_frame2 = tk.Frame(self.root, bg='white')
        top_frame2.pack(pady=(2, 2), padx=15, fill='x')
        
        self.time_label = tk.Label(top_frame2, text="", 
                                  font=("Microsoft YaHei", 60, "bold"),
                                  bg='white', fg='black', anchor='e')
        self.time_label.pack(fill='x')
        
        # 第四行：星期
        top_frame3 = tk.Frame(self.root, bg='white')
        top_frame3.pack(pady=(2, 5), padx=15, fill='x')
        
        self.weekday_label = tk.Label(top_frame3, text="", 
                                     font=("Microsoft YaHei", 24, "bold"),
                                     bg='white', fg='black', anchor='e')
        self.weekday_label.pack(fill='x')
        
        # 创建课程表格
        self.create_schedule_table()
        print("UI设置完成")
        
    def create_schedule_table(self):
        print("创建课程表框架...")
        self.main_frame = tk.Frame(self.root, bg='white')
        # 先创建课程表但不显示，等检查完置顶状态后再决定是否显示
        self.create_course_list()
        
    def create_course_list(self):
        """创建固定的课程列表（无滚动条）"""
        print("创建课程列表...")
        
        # 清除原有课程列表
        if self.main_frame:
            for widget in self.main_frame.winfo_children():
                widget.destroy()
        
        # 获取当前星期的课程表
        weekday_english = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
        current_weekday_index = (datetime.now().weekday() + 1) % 7
        current_weekday_english = weekday_english[current_weekday_index]
        
        class_schedule = self.settings["schedules"].get(current_weekday_english, [])
        
        print(f"当前星期: {current_weekday_english}")
        print(f"课程数量: {len(class_schedule)}")
        
        # 直接在主框架中创建课程标签
        for i, course in enumerate(class_schedule):
            course_label = tk.Label(self.main_frame, text=course,
                                   font=("Microsoft YaHei", 30, "bold"),
                                   bg='white', fg='black', height=1,
                                   anchor='e', justify='right')
            course_label.pack(fill='x', pady=0)
            print(f"创建课程标签 {i+1}: {course}")
        
        print("课程列表创建完成")
    
    def toggle_display_mode(self, is_topmost):
        """切换显示模式"""
        print(f"切换显示模式: is_topmost={is_topmost}")
        
        if is_topmost:
            # 置顶模式：只显示时间，隐藏课程表，透明度0.3
            if self.main_frame and self.main_frame.winfo_ismapped():
                print("隐藏课程表")
                self.main_frame.pack_forget()  # 隐藏课程表
            self.root.attributes('-alpha', 0.3)  # 设置透明度为0.3
            self.current_topmost_state = True
            print("切换到置顶模式：隐藏课程表，透明度0.3")
        else:
            # 正常模式：显示完整内容，包括课程表，恢复用户设置的透明度
            if self.main_frame and not self.main_frame.winfo_ismapped():
                print("显示课程表")
                self.main_frame.pack(fill='both', expand=True, padx=15, pady=(0, 10))  # 显示课程表
            self.root.attributes('-alpha', self.user_transparency)  # 恢复用户设置的透明度
            self.current_topmost_state = False
            print(f"切换到正常模式：显示课程表，透明度{self.user_transparency}")
    
    def show_settings(self):
        """显示设置窗口"""
        try:
            # 如果设置窗口已经存在，则将其置顶
            if self.settings_window and self.settings_window.winfo_exists():
                self.settings_window.lift()
                self.settings_window.focus_force()
                return
                
            self.settings_window = tk.Toplevel(self.root)
            self.settings_window.title("设置")
            self.settings_window.geometry("600x700")  # 增加高度以适应多时间段
            self.settings_window.resizable(False, False)
            self.settings_window.transient(self.root)
            self.settings_window.grab_set()
            
            # 居中显示
            self.settings_window.update_idletasks()
            x = (self.settings_window.winfo_screenwidth() - self.settings_window.winfo_width()) // 2
            y = (self.settings_window.winfo_screenheight() - self.settings_window.winfo_height()) // 2
            self.settings_window.geometry(f"+{x}+{y}")
            
            # 创建可滚动的设置框架
            settings_canvas = tk.Canvas(self.settings_window, bg='white', highlightthickness=0)
            scrollbar = tk.Scrollbar(self.settings_window, orient="vertical", command=settings_canvas.yview)
            settings_scrollable_frame = tk.Frame(settings_canvas, bg='white')
            
            settings_canvas.configure(yscrollcommand=scrollbar.set)
            
            # 绑定鼠标滚轮事件
            def on_mousewheel(event):
                settings_canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
            settings_canvas.bind_all("<MouseWheel>", on_mousewheel)
            
            # 将可滚动框架添加到画布
            settings_canvas_frame = settings_canvas.create_window((0, 0), window=settings_scrollable_frame, anchor="nw")
            
            def configure_scroll_region(event):
                settings_canvas.configure(scrollregion=settings_canvas.bbox("all"))
                settings_canvas.itemconfig(settings_canvas_frame, width=settings_canvas.winfo_width())
            
            settings_scrollable_frame.bind("<Configure>", configure_scroll_region)
            
            # 创建选项卡框架
            tab_frame = tk.Frame(settings_scrollable_frame, bg='white')
            tab_frame.pack(fill='x', padx=10, pady=5)
            
            # 星期选项卡按钮
            weekdays_chinese = ["星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"]
            weekdays_english = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
            
            self.current_tab = tk.StringVar(value=weekdays_english[0])
            
            # 创建选项卡按钮
            for i, (day_ch, day_en) in enumerate(zip(weekdays_chinese, weekdays_english)):
                btn = tk.Radiobutton(tab_frame, text=day_ch, variable=self.current_tab, value=day_en,
                                   command=lambda d=day_en: self.switch_schedule_tab(d),
                                   font=("Microsoft YaHei", 9), bg='white')
                btn.pack(side='left', padx=2)
                if i == 0:
                    btn.select()
            
            # 课程表编辑选项卡
            course_frame = tk.LabelFrame(settings_scrollable_frame, text="课程表编辑", font=("Microsoft YaHei", 10, "bold"))
            course_frame.pack(fill='x', pady=(10, 10), padx=10)
            
            # 课程表编辑容器
            self.course_edit_container = tk.Frame(course_frame, bg='white')
            self.course_edit_container.pack(fill='x', padx=10, pady=10)
            
            # 预先创建所有选项卡的内容（但不显示）
            self.create_all_tab_contents()
            
            # 显示第一个选项卡
            self.switch_schedule_tab(weekdays_english[0])
            
            # 透明度设置选项卡
            transparency_frame = tk.LabelFrame(settings_scrollable_frame, text="透明度设置", font=("Microsoft YaHei", 10, "bold"))
            transparency_frame.pack(fill='x', pady=(0, 10), padx=10)
            
            transparency_content = tk.Frame(transparency_frame)
            transparency_content.pack(fill='x', padx=10, pady=10)
            
            tk.Label(transparency_content, text="透明度:", font=("Microsoft YaHei", 9)).pack(anchor='w')
            
            # 透明度滑动条
            self.transparency_var = tk.DoubleVar(value=self.settings["transparency"])
            transparency_scale = tk.Scale(transparency_content, from_=0.1, to=1.0, resolution=0.1,
                                        orient='horizontal', variable=self.transparency_var,
                                        command=self.update_transparency, length=300)
            transparency_scale.pack(fill='x', pady=5)
            
            tk.Label(transparency_content, text="0.1: 最透明 | 1.0: 不透明", 
                    font=("Microsoft YaHei", 8), fg='gray').pack(anchor='w')
            
            # 窗口置顶时间设置选项卡
            topmost_frame = tk.LabelFrame(settings_scrollable_frame, text="窗口置顶时间段", font=("Microsoft YaHei", 10, "bold"))
            topmost_frame.pack(fill='x', pady=(0, 10), padx=10)
            
            topmost_content = tk.Frame(topmost_frame)
            topmost_content.pack(fill='x', padx=10, pady=10)
            
            tk.Label(topmost_content, text="在指定时间段内窗口将置于顶层", 
                    font=("Microsoft YaHei", 9)).pack(anchor='w', pady=(0, 10))
            
            # 时间段管理框架
            time_ranges_frame = tk.Frame(topmost_content)
            time_ranges_frame.pack(fill='x', pady=5)
            
            # 时间段列表容器
            self.time_ranges_container = tk.Frame(time_ranges_frame)
            self.time_ranges_container.pack(fill='x')
            
            # 清空时间段变量列表
            self.time_range_vars = []
            
            # 添加现有时间段
            for time_range in self.settings["topmost_time_ranges"]:
                self.add_time_range(time_range["start"], time_range["end"])
            
            # 添加时间段按钮
            add_time_range_btn = tk.Button(time_ranges_frame, text="添加时间段", 
                                         font=("Microsoft YaHei", 9),
                                         command=self.add_time_range,
                                         bg='lightblue', width=12)
            add_time_range_btn.pack(pady=5)
            
            # 按钮区域
            button_frame = tk.Frame(settings_scrollable_frame)
            button_frame.pack(fill='x', pady=10, padx=10)
            
            def save_settings():
                self.save_all_tabs_data()  # 保存所有选项卡的数据
                self.save_all_settings()
                self.settings_window.grab_release()
                self.settings_window.destroy()
                self.settings_window = None
                self.course_frames = {}  # 清空框架缓存
                self.time_range_vars = []  # 清空时间段变量
            
            def close_settings():
                self.settings_window.grab_release()
                self.settings_window.destroy()
                self.settings_window = None
                self.course_frames = {}  # 清空框架缓存
                self.time_range_vars = []  # 清空时间段变量
            
            tk.Button(button_frame, text="保存", command=save_settings, 
                     font=("Microsoft YaHei", 10), width=10, bg='lightgreen').pack(side='left', padx=(0, 10))
            tk.Button(button_frame, text="取消", command=close_settings,
                     font=("Microsoft YaHei", 10), width=10, bg='lightcoral').pack(side='left')
            
            # 打包画布和滚动条
            settings_canvas.pack(side="left", fill="both", expand=True)
            scrollbar.pack(side="right", fill="y")
            
            # 更新滚动区域
            settings_scrollable_frame.update_idletasks()
            settings_canvas.configure(scrollregion=settings_canvas.bbox("all"))
            
            # 设置窗口关闭事件
            self.settings_window.protocol("WM_DELETE_WINDOW", close_settings)
            
        except Exception as e:
            print(f"显示设置窗口时出错: {e}")
    
    def add_time_range(self, start_time="08:00", end_time="18:00"):
        """添加一个时间段输入行"""
        time_range_frame = tk.Frame(self.time_ranges_container)
        time_range_frame.pack(fill='x', pady=2)
        
        # 开始时间
        start_var = tk.StringVar(value=start_time)
        start_entry = tk.Entry(time_range_frame, textvariable=start_var, 
                              font=("Microsoft YaHei", 9), width=8)
        start_entry.pack(side='left', padx=(0, 5))
        
        tk.Label(time_range_frame, text="到", font=("Microsoft YaHei", 9)).pack(side='left', padx=5)
        
        # 结束时间
        end_var = tk.StringVar(value=end_time)
        end_entry = tk.Entry(time_range_frame, textvariable=end_var, 
                            font=("Microsoft YaHei", 9), width=8)
        end_entry.pack(side='left', padx=(0, 5))
        
        # 删除按钮
        def remove_time_range():
            time_range_frame.destroy()
            # 从变量列表中移除
            for i, (s, e, _) in enumerate(self.time_range_vars):
                if s == start_var and e == end_var:
                    self.time_range_vars.pop(i)
                    break
        
        remove_btn = tk.Button(time_range_frame, text="删除", 
                              font=("Microsoft YaHei", 8),
                              command=remove_time_range,
                              bg='lightcoral', width=6)
        remove_btn.pack(side='left')
        
        # 存储时间变量
        self.time_range_vars.append((start_var, end_var, time_range_frame))
    
    def create_all_tab_contents(self):
        """预先创建所有选项卡的内容"""
        weekdays_english = ["Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"]
        
        for day in weekdays_english:
            # 为每个选项卡创建一个框架
            tab_frame = tk.Frame(self.course_edit_container, bg='white')
            self.course_frames[day] = tab_frame
            
            schedule = self.settings["schedules"].get(day, [])
            
            # 创建15个输入框
            entries = []
            for i in range(15):
                row_frame = tk.Frame(tab_frame, bg='white')
                row_frame.pack(fill='x', pady=0)
                
                tk.Label(row_frame, text=f"第{i+1}节:", font=("Microsoft YaHei", 9), width=8, anchor='w').pack(side='left')
                entry = tk.Entry(row_frame, font=("Microsoft YaHei", 9), width=20)
                
                # 设置输入框的验证，允许所有字符包括空格
                def validate_input(char):
                    # 允许所有字符输入，包括空格
                    return True
                
                # 注册验证函数
                vcmd = (entry.register(validate_input), '%S')
                entry.configure(validate='key', validatecommand=vcmd)
                
                if i < len(schedule):
                    entry.insert(0, schedule[i])
                else:
                    entry.insert(0, "")
                entry.pack(side='left', fill='x', expand=True)
                entries.append(entry)
            
            # 存储该选项卡的输入框引用
            self.course_frames[day] = (tab_frame, entries)
    
    def switch_schedule_tab(self, day):
        """切换课程表选项卡"""
        # 隐藏当前显示的选项卡
        if self.current_visible_tab and self.current_visible_tab in self.course_frames:
            tab_frame, _ = self.course_frames[self.current_visible_tab]
            tab_frame.pack_forget()
        
        # 显示选中的选项卡
        if day in self.course_frames:
            tab_frame, _ = self.course_frames[day]
            tab_frame.pack(fill='x')
            self.current_visible_tab = day
    
    def save_all_tabs_data(self):
        """保存所有选项卡的数据"""
        for day, (tab_frame, entries) in self.course_frames.items():
            current_schedule = []
            for entry in entries:
                if entry.winfo_exists():
                    # 不再去除空格，保留原始输入内容
                    course_text = entry.get()  # 移除了.strip()
                    current_schedule.append(course_text)
            
            # 更新设置
            self.settings["schedules"][day] = current_schedule
    
    def update_transparency(self, value):
        """实时更新透明度"""
        try:
            alpha = float(value)
            # 只有在非置顶模式下才更新透明度
            if not self.current_topmost_state:
                self.root.attributes('-alpha', alpha)
                self.user_transparency = alpha  # 更新用户设置的透明度
        except:
            pass
    
    def save_all_settings(self):
        """保存所有设置"""
        # 只有在设置窗口打开时才保存设置窗口的数据
        if self.settings_window and self.settings_window.winfo_exists():
            # 保存所有选项卡的数据
            self.save_all_tabs_data()
            
            # 保存透明度
            try:
                if self.transparency_var:
                    self.settings["transparency"] = float(self.transparency_var.get())
                    self.user_transparency = self.settings["transparency"]  # 更新用户设置的透明度
            except:
                pass
            
            # 保存时间段设置
            try:
                time_ranges = []
                for start_var, end_var, frame in self.time_range_vars:
                    if frame.winfo_exists():  # 确保框架还存在
                        start_time = start_var.get()
                        end_time = end_var.get()
                        # 验证时间格式
                        datetime.strptime(start_time, "%H:%M")
                        datetime.strptime(end_time, "%H:%M")
                        time_ranges.append({"start": start_time, "end": end_time})
                
                self.settings["topmost_time_ranges"] = time_ranges
            except ValueError:
                # 时间格式错误，恢复默认值
                self.settings["topmost_time_ranges"] = [{"start": "08:00", "end": "18:00"}]
            except:
                pass
        
        # 保存到文件
        save_settings(self.settings)
        
        # 立即检查置顶状态
        self.check_topmost_status()
        
        # 刷新课程表显示
        self.create_course_list()
    
    def start_topmost_check(self):
        """开始检查窗口置顶状态"""
        # 使用after循环
        self.root.after(5000, self.start_topmost_check)  # 每5秒检查一次
        self.check_topmost_status()
    
    def check_topmost_status(self):
        """检查并更新窗口置顶状态"""
        try:
            current_time = datetime.now().time()
            should_be_topmost = False
            
            # 检查所有时间段
            for time_range in self.settings["topmost_time_ranges"]:
                start_hour, start_minute = map(int, time_range["start"].split(":"))
                end_hour, end_minute = map(int, time_range["end"].split(":"))
                
                start_time_obj = current_time.replace(hour=start_hour, minute=start_minute, second=0, microsecond=0)
                end_time_obj = current_time.replace(hour=end_hour, minute=end_minute, second=0, microsecond=0)
                
                # 检查当前时间是否在这个时间段内
                if start_time_obj <= current_time <= end_time_obj:
                    should_be_topmost = True
                    print(f"当前时间 {current_time} 在时间段 {time_range['start']}-{time_range['end']} 内")
                    break
            
            print(f"当前时间: {current_time}")
            print(f"置顶时间段: {self.settings['topmost_time_ranges']}")
            print(f"应该置顶: {should_be_topmost}")
            print(f"当前置顶状态: {self.current_topmost_state}")
            
            # 只在状态真正改变时才更新
            if should_be_topmost != self.current_topmost_state:
                if should_be_topmost:
                    self.root.attributes('-topmost', True)
                    self.toggle_display_mode(True)  # 切换到置顶模式
                    print(f"窗口置顶，透明度0.3")
                else:
                    self.root.attributes('-topmost', False)
                    self.toggle_display_mode(False)  # 切换到正常模式
                    print(f"窗口取消置顶，透明度{self.user_transparency}")
            else:
                # 确保当前显示模式正确 - 这是关键修复！
                # 如果当前状态不正确，强制切换
                if should_be_topmost and not self.current_topmost_state:
                    print("强制切换到置顶模式")
                    self.root.attributes('-topmost', True)
                    self.toggle_display_mode(True)
                elif not should_be_topmost and self.current_topmost_state:
                    print("强制切换到正常模式")
                    self.root.attributes('-topmost', False)
                    self.toggle_display_mode(False)
                # 新增：如果不在置顶状态且课程表没有显示，则显示课程表
                elif not should_be_topmost and self.main_frame and not self.main_frame.winfo_ismapped():
                    print("确保课程表显示")
                    self.main_frame.pack(fill='both', expand=True, padx=15, pady=(0, 10))
                    
        except Exception as e:
            print(f"检查置顶状态时出错: {e}")
            # 出错时默认不置顶
            if self.current_topmost_state:
                self.root.attributes('-topmost', False)
                self.toggle_display_mode(False)
    
    def restart_app(self):
        """重启应用程序"""
        print("重启应用程序...")
        # 直接重启，不保存设置窗口的数据
        self.root.after(100, self.restart_application)
    
    def restart_application(self):
        """重启应用程序"""
        python = sys.executable
        os.execl(python, python, *sys.argv)
    
    def quit_application(self):
        """退出应用程序"""
        self.root.quit()
        self.root.destroy()
        os._exit(0)
    
    def get_current_datetime(self):
        now = datetime.now()
        date_str = now.strftime("%Y年%m月%d日")
        time_str = now.strftime("%H:%M:%S")
        weekdays = ["星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"]
        weekday_num = (now.weekday() + 1) % 7
        weekday_str = weekdays[weekday_num]
        return date_str, time_str, weekday_str
    
    def update_datetime(self):
        date_str, time_str, weekday_str = self.get_current_datetime()
        self.date_label.config(text=date_str)
        self.time_label.config(text=time_str)
        self.weekday_label.config(text=weekday_str)
        
        # 检查星期是否变化，如果变化则更新课程表
        current_weekday_index = (datetime.now().weekday() + 1) % 7
        if self.current_weekday != current_weekday_index:
            self.current_weekday = current_weekday_index
            self.create_course_list()
        
        self.root.after(1000, self.update_datetime)
    
    def start_anti_burnin(self):
        self.pixel_shift()
        self.root.after(self.shift_interval * 1000, self.start_anti_burnin)
    
    def pixel_shift(self):
        screen_width = self.root.winfo_screenwidth()
        shift_x = random.randint(-self.max_pixel_shift, self.max_pixel_shift)
        shift_y = random.randint(-self.max_pixel_shift, self.max_pixel_shift)
        new_x = max(0, min(screen_width - 400, screen_width - 400 + shift_x))
        new_y = max(0, min(0 + shift_y, 10))
        self.root.geometry(f"400x{self.root.winfo_screenheight()}+{new_x}+{new_y}")
        self.pixel_shift_count += 1

def main():
    root = tk.Tk()
    app = ClassScheduleApp(root)
    
    # 直接关闭窗口退出程序
    root.protocol("WM_DELETE_WINDOW", app.quit_application)
    
    root.mainloop()

if __name__ == "__main__":
    main()