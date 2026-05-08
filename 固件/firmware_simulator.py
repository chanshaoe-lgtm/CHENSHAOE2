import tkinter as tk
from tkinter import font
from PIL import Image, ImageTk
import os
import time

COLORS = {
    'bg': '#000000',
    'text': '#ffffff',
    'primary': '#0088ff',
    'success': '#00cc66',
    'accent': '#ff4444',
    'gray': '#888888',
    'dark': '#1a1a1a',
    'darker': '#0f0f0f',
    'border': '#333333',
    'btn_normal': '#252525',
    'btn_pressed': '#404040',
    'btn_hover': '#303030'
}

class FirmwareSimulator:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("ECAN赛车无线屏")
        self.root.geometry("540x420")
        self.root.configure(bg='#121212')
        self.root.resizable(False, False)
        
        self.screen_num = 1
        self.keyboard_mode = 0
        self.last_alpha_mode = 0
        self.show_password = False
        self.wifi_scroll_offset = 0
        self.drag_start_y = None
        self.drag_start_x = None
        self.drag_start_offset = 0
        self.was_dragged = False
        self.logo_img = None
        self.logo_tk = None
        self.password = ""
        self.connecting_anim_step = 0
        self.selected_wifi = 0
        self.wifis = [
            "MyHomeWiFi", "GuestWiFi", "NeighborWiFi", 
            "OfficeNetwork", "CafeWiFi", "LibraryFree",
            "SmartHome", "TechLab", "CampusNet"
        ]
        
        self.pressed_button = None
        self.button_animation_id = None
        
        self.load_logo()
        
        self.main_frame = tk.Frame(self.root, bg='#121212')
        self.main_frame.pack(pady=15)
        
        self.screen_container = tk.Frame(self.main_frame, bg='#000000', bd=3, relief='solid')
        self.screen_container.pack()
        
        self.canvas = tk.Canvas(self.screen_container, width=480, height=272, 
                                bg=COLORS['bg'], highlightthickness=0)
        self.canvas.pack()
        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)
        self.canvas.bind("<MouseWheel>", self.on_mouse_wheel)
        
        self.controls = tk.Frame(self.root, bg='#121212')
        self.controls.pack(pady=10)
        
        btn_style = {
            'font': ('Segoe UI', 10),
            'bg': '#2a2a2a',
            'fg': '#ffffff',
            'activebackground': '#3a3a3a',
            'activeforeground': '#ffffff',
            'relief': 'flat',
            'padx': 25,
            'pady': 6,
            'bd': 0
        }
        
        tk.Button(self.controls, text="<-", command=self.prev_screen, **btn_style).pack(side=tk.LEFT, padx=8)
        tk.Button(self.controls, text="->", command=self.next_screen, **btn_style).pack(side=tk.LEFT, padx=8)
        
        tk.Label(self.root, text="480 x 272  |  ECAN Racing Display", 
                bg='#121212', fg='#555555', font=('Segoe UI', 9)).pack()
        
        self.draw_current_screen()
        self.root.mainloop()
    
    def load_logo(self):
        logo_path = r"c:\Users\Administrator\Desktop\ECANNON屏幕\ECANNON项目\上位机\assets\ecannon_logo.png"
        if os.path.exists(logo_path):
            self.logo_img = Image.open(logo_path)
            self.logo_img = self.logo_img.resize((320, 60), Image.Resampling.LANCZOS)
            self.logo_tk = ImageTk.PhotoImage(self.logo_img)
    
    def text_center_x(self, x, y, text, color=COLORS['text'], size=18):
        f = font.Font(family='Segoe UI', size=size)
        self.canvas.create_text(x, y, text=text, fill=color, font=f, anchor='center')
    
    def text(self, x, y, text, color=COLORS['text'], size=14):
        f = font.Font(family='Segoe UI', size=size)
        self.canvas.create_text(x, y, text=text, fill=color, font=f, anchor='nw')
    
    def animate_button_press(self, btn_info):
        if self.button_animation_id:
            self.root.after_cancel(self.button_animation_id)
        self.pressed_button = btn_info
        self.draw_current_screen()
        self.button_animation_id = self.root.after(100, self.release_button_animation)
    
    def release_button_animation(self):
        self.pressed_button = None
        self.draw_current_screen()
    
    def on_mouse_wheel(self, event):
        if self.screen_num == 2:
            if event.delta > 0:
                if self.wifi_scroll_offset > 0:
                    self.wifi_scroll_offset -= 1
                    self.draw_current_screen()
            else:
                if self.wifi_scroll_offset < len(self.wifis) - 4:
                    self.wifi_scroll_offset += 1
                    self.draw_current_screen()
    
    def check_button_press(self, x, y, btn_type, btn_data):
        if self.screen_num == 2:
            if btn_type == 'up_arrow' and 150 <= x <= 210 and 238 <= y <= 262:
                if self.wifi_scroll_offset > 0:
                    self.animate_button_press(('up_arrow', None))
                    self.wifi_scroll_offset -= 1
                return True
            elif btn_type == 'down_arrow' and 270 <= x <= 330 and 238 <= y <= 262:
                if self.wifi_scroll_offset < len(self.wifis) - 4:
                    self.animate_button_press(('down_arrow', None))
                    self.wifi_scroll_offset += 1
                return True
        
        elif self.screen_num == 3:
            if btn_type == 'eye' and 385 <= x <= 415 and 60 <= y <= 95:
                self.animate_button_press(('eye', None))
                self.show_password = not self.show_password
                return True
            
            key_w = 36
            start_x = 40
            start_y = 95
            
            if btn_type == 'key' and start_y <= y <= start_y + 4 * 36:
                if self.keyboard_mode == 0:
                    keys = 'qwertyuiopasdfghjklzxcvbnm'
                elif self.keyboard_mode == 1:
                    keys = 'QWERTYUIOPASDFGHJKLZXCVBNM'
                elif self.keyboard_mode == 2:
                    keys = '1234567890'
                else:
                    keys = '!@#$%^&*()_+-=[]{}|;:,.<>?/\\"\'`~'
                
                if self.keyboard_mode == 2:
                    cols = 10
                    rows = 1
                else:
                    cols = 10
                    rows = 3 if self.keyboard_mode < 2 else 3
                
                for i, k in enumerate(keys):
                    if self.keyboard_mode == 2 and i >= 10:
                        break
                    if self.keyboard_mode < 2 and i >= 26:
                        break
                    if self.keyboard_mode >= 3 and i >= 30:
                        break
                    
                    row = i // cols
                    col = i % cols
                    kx = start_x + col * (key_w+4)
                    ky = start_y + row * 36
                    if kx <= x <= kx+key_w and ky <= y <= ky+30:
                        self.animate_button_press(('key', k))
                        self.password += k
                        return True
            
            if btn_type == 'shift' and 40 <= x <= 105 and 224 <= y <= 248:
                self.animate_button_press(('shift', None))
                if self.keyboard_mode == 0:
                    self.keyboard_mode = 1
                elif self.keyboard_mode == 1:
                    self.keyboard_mode = 0
                elif self.keyboard_mode == 2 or self.keyboard_mode == 3:
                    self.keyboard_mode = self.last_alpha_mode
                    if self.keyboard_mode == 0:
                        self.keyboard_mode = 1
                    else:
                        self.keyboard_mode = 0
                self.last_alpha_mode = self.keyboard_mode
                return True
            
            if btn_type == 'num' and 115 <= x <= 180 and 224 <= y <= 248:
                self.animate_button_press(('num', None))
                if self.keyboard_mode == 2:
                    self.keyboard_mode = self.last_alpha_mode
                else:
                    if self.keyboard_mode == 0 or self.keyboard_mode == 1:
                        self.last_alpha_mode = self.keyboard_mode
                    self.keyboard_mode = 2
                return True
            
            if btn_type == 'sym' and 190 <= x <= 255 and 224 <= y <= 248:
                self.animate_button_press(('sym', None))
                if self.keyboard_mode == 3:
                    self.keyboard_mode = self.last_alpha_mode
                else:
                    if self.keyboard_mode == 0 or self.keyboard_mode == 1:
                        self.last_alpha_mode = self.keyboard_mode
                    self.keyboard_mode = 3
                return True
            
            if btn_type == 'backspace' and 265 <= x <= 350 and 224 <= y <= 248:
                if len(self.password) > 0:
                    self.animate_button_press(('backspace', None))
                    self.password = self.password[:-1]
                return True
            
            if btn_type == 'enter' and 360 <= x <= 440 and 224 <= y <= 248:
                self.animate_button_press(('enter', None))
                self.screen_num = 4
                self.connecting_anim_step = 0
                return True
        
        return False
    
    def on_canvas_click(self, event):
        if self.screen_num == 2:
            x, y = event.x, event.y
            self.drag_start_x = x
            self.drag_start_y = y
            self.drag_start_offset = self.wifi_scroll_offset
            self.was_dragged = False
            
            if self.check_button_press(x, y, 'up_arrow', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'down_arrow', None):
                self.draw_current_screen()
                return
        
        elif self.screen_num == 3:
            x, y = event.x, event.y
            
            if self.check_button_press(x, y, 'eye', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'key', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'shift', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'num', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'sym', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'backspace', None):
                self.draw_current_screen()
                return
            elif self.check_button_press(x, y, 'enter', None):
                self.draw_current_screen()
                self.animate_connection()
                return
    
    def on_canvas_drag(self, event):
        if self.screen_num == 2 and self.drag_start_y is not None:
            delta_y = event.y - self.drag_start_y
            if abs(delta_y) > 5:
                self.was_dragged = True
            scroll_steps = int(delta_y / 42)
            new_offset = self.drag_start_offset - scroll_steps
            new_offset = max(0, min(new_offset, len(self.wifis) - 4))
            if new_offset != self.wifi_scroll_offset:
                self.wifi_scroll_offset = new_offset
                self.draw_current_screen()
    
    def on_canvas_release(self, event):
        if self.screen_num == 2 and not self.was_dragged:
            x, y = event.x, event.y
            if 30 <= x <= 450 and 70 <= y <= 238:
                rel_y = y - 70
                clicked_idx = rel_y // 42
                if 0 <= clicked_idx < 4:
                    actual_idx = self.wifi_scroll_offset + clicked_idx
                    if actual_idx < len(self.wifis):
                        self.selected_wifi = actual_idx
                        self.draw_current_screen()
                        self.root.after(300, self.go_to_password)
                        return
        
        self.drag_start_y = None
        self.drag_start_x = None
        self.drag_start_offset = 0
        self.was_dragged = False
    
    def go_to_password(self):
        self.screen_num = 3
        self.draw_current_screen()
    
    def animate_connection(self):
        if self.screen_num == 4 and self.connecting_anim_step < 20:
            self.connecting_anim_step += 1
            self.draw_screen4()
            self.root.after(150, self.animate_connection)
        elif self.screen_num == 4 and self.connecting_anim_step >= 20:
            self.screen_num = 1
            self.draw_current_screen()
    
    def prev_screen(self):
        if self.screen_num > 1:
            self.screen_num -= 1
            self.draw_current_screen()
    
    def next_screen(self):
        if self.screen_num < 4:
            self.screen_num += 1
            self.draw_current_screen()
    
    def draw_current_screen(self):
        if self.screen_num == 1:
            self.draw_screen1()
        elif self.screen_num == 2:
            self.draw_screen2()
        elif self.screen_num == 3:
            self.draw_screen3()
        elif self.screen_num == 4:
            self.draw_screen4()
    
    def draw_screen1(self):
        self.canvas.delete('all')
        
        if self.logo_tk:
            self.canvas.create_image(240, 45, image=self.logo_tk)
        
        self.text_center_x(240, 85, "请在PC端软件填写此IP", '#888888', 11)
        self.canvas.create_rectangle(90, 95, 390, 130, fill='#1a1a1a', outline='#333333')
        self.text_center_x(240, 112, "192.168.1.105", '#00aaff', 18)
        
        self.text_center_x(240, 150, "官网：WWW.ECANNON.CC", '#555555', 12)
        
        self.canvas.create_oval(200, 180, 280, 260, fill='#0f3d1e', outline='#00cc66', width=3)
        
        self.canvas.create_line(212, 220, 232, 240, fill='#00cc66', width=5)
        self.canvas.create_line(232, 240, 268, 204, fill='#00cc66', width=5)
    
    def draw_screen2(self):
        self.canvas.delete('all')
        
        if self.logo_tk:
            small_logo = self.logo_img.resize((180, 34), Image.Resampling.LANCZOS)
            small_logo_tk = ImageTk.PhotoImage(small_logo)
            self.canvas.create_image(240, 25, image=small_logo_tk)
            self.small_logo_ref = small_logo_tk
        
        self.text_center_x(240, 50, "WiFi List", COLORS['text'], 16)
        
        visible_wifis = self.wifis[self.wifi_scroll_offset:self.wifi_scroll_offset + 4]
        y = 70
        for i, name in enumerate(visible_wifis):
            actual_index = self.wifi_scroll_offset + i
            if actual_index == self.selected_wifi:
                self.canvas.create_rectangle(30, y, 450, y+38, fill=COLORS['primary'])
            else:
                self.canvas.create_rectangle(30, y, 450, y+38, fill=COLORS['dark'])
            self.text(85, y+11, name, '#ffffff', 14)
            for j in range(4 - (actual_index % 4)):
                self.canvas.create_rectangle(42+j*8, y+23-j*4, 48+j*8, y+23, fill='#44aa44')
            y += 42
        
        up_btn_pressed = self.pressed_button and self.pressed_button[0] == 'up_arrow'
        up_btn_bg = COLORS['btn_pressed'] if up_btn_pressed else ('#2a2a2a' if self.wifi_scroll_offset > 0 else '#1a1a1a')
        up_btn_text = '#aaaaaa' if self.wifi_scroll_offset > 0 else '#555555'
        self.canvas.create_rectangle(150, 238, 210, 262, fill=up_btn_bg, outline=COLORS['border'])
        self.text_center_x(180, 250, "▲", up_btn_text, 14)
        
        down_btn_pressed = self.pressed_button and self.pressed_button[0] == 'down_arrow'
        down_btn_bg = COLORS['btn_pressed'] if down_btn_pressed else ('#2a2a2a' if self.wifi_scroll_offset < len(self.wifis) - 4 else '#1a1a1a')
        down_btn_text = '#aaaaaa' if self.wifi_scroll_offset < len(self.wifis) - 4 else '#555555'
        self.canvas.create_rectangle(270, 238, 330, 262, fill=down_btn_bg, outline=COLORS['border'])
        self.text_center_x(300, 250, "▼", down_btn_text, 14)
    
    def draw_screen3(self):
        self.canvas.delete('all')
        
        if self.logo_tk:
            small_logo = self.logo_img.resize((180, 34), Image.Resampling.LANCZOS)
            small_logo_tk = ImageTk.PhotoImage(small_logo)
            self.canvas.create_image(240, 25, image=small_logo_tk)
            self.small_logo_ref2 = small_logo_tk
        
        self.canvas.create_rectangle(60, 50, 420, 85, fill=COLORS['dark'], outline=COLORS['border'])
        password_text = "*" * len(self.password) if not self.show_password else self.password
        self.text(75, 60, password_text, '#66ffaa', 16)
        
        eye_pressed = self.pressed_button and self.pressed_button[0] == 'eye'
        eye_bg = COLORS['btn_pressed'] if eye_pressed else '#2a2a2a'
        self.canvas.create_rectangle(385, 50, 415, 85, fill=eye_bg, outline=COLORS['border'])
        eye_text = "O" if self.show_password else "●"
        self.text_center_x(400, 67, eye_text, '#aaaaaa', 16)
        
        key_w = 36
        start_x = 40
        start_y = 95
        
        pressed_key = None
        if self.pressed_button and self.pressed_button[0] == 'key':
            pressed_key = self.pressed_button[1]
        
        if self.keyboard_mode == 0:
            keys = 'qwertyuiopasdfghjklzxcvbnm'
            for i, k in enumerate(keys):
                if i >= 26:
                    break
                row = i // 10
                col = i % 10
                x = start_x + col * (key_w+4)
                y = start_y + row * 36
                btn_bg = COLORS['btn_pressed'] if pressed_key == k else COLORS['dark']
                self.canvas.create_rectangle(x, y, x+key_w, y+30, fill=btn_bg, outline=COLORS['border'])
                self.text(x+14, y+6, k, '#cccccc', 13)
        elif self.keyboard_mode == 1:
            keys = 'QWERTYUIOPASDFGHJKLZXCVBNM'
            for i, k in enumerate(keys):
                if i >= 26:
                    break
                row = i // 10
                col = i % 10
                x = start_x + col * (key_w+4)
                y = start_y + row * 36
                btn_bg = COLORS['btn_pressed'] if pressed_key == k else COLORS['dark']
                self.canvas.create_rectangle(x, y, x+key_w, y+30, fill=btn_bg, outline=COLORS['border'])
                self.text(x+12, y+6, k, '#cccccc', 13)
        elif self.keyboard_mode == 2:
            keys = '1234567890'
            for i, k in enumerate(keys):
                row = 0
                col = i
                x = start_x + col * (key_w+4)
                y = start_y + row * 36
                btn_bg = COLORS['btn_pressed'] if pressed_key == k else COLORS['dark']
                self.canvas.create_rectangle(x, y, x+key_w, y+30, fill=btn_bg, outline=COLORS['border'])
                self.text(x+14, y+6, k, '#cccccc', 13)
        else:
            symbols = ['!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
                      '_', '+', '-', '=', '[', ']', '{', '}', '|', ';',
                      ':', ',', '.', '<', '>', '?', '/', '\\', "'"]
            for i, k in enumerate(symbols):
                if i >= 30:
                    break
                row = i // 10
                col = i % 10
                x = start_x + col * (key_w+4)
                y = start_y + row * 36
                btn_bg = COLORS['btn_pressed'] if pressed_key == k else COLORS['dark']
                self.canvas.create_rectangle(x, y, x+key_w, y+30, fill=btn_bg, outline=COLORS['border'])
                self.text(x+14, y+6, k, '#cccccc', 13)
        
        shift_pressed = self.pressed_button and self.pressed_button[0] == 'shift'
        shift_bg = COLORS['btn_pressed'] if shift_pressed else ('#3a3a3a' if self.keyboard_mode == 1 else '#252525')
        self.canvas.create_rectangle(40, 224, 105, 248, fill=shift_bg, outline=COLORS['border'])
        shift_text = "↑" if self.keyboard_mode == 1 else "↑"
        self.text_center_x(72, 236, shift_text, '#aaaaaa', 14)
        
        num_pressed = self.pressed_button and self.pressed_button[0] == 'num'
        num_bg = COLORS['btn_pressed'] if num_pressed else ('#3a3a3a' if self.keyboard_mode == 2 else '#252525')
        self.canvas.create_rectangle(115, 224, 180, 248, fill=num_bg, outline=COLORS['border'])
        num_btn_text = "ABC" if self.keyboard_mode == 2 else "123"
        self.text_center_x(147, 236, num_btn_text, '#aaaaaa', 14)
        
        sym_pressed = self.pressed_button and self.pressed_button[0] == 'sym'
        sym_bg = COLORS['btn_pressed'] if sym_pressed else ('#3a3a3a' if self.keyboard_mode == 3 else '#252525')
        self.canvas.create_rectangle(190, 224, 255, 248, fill=sym_bg, outline=COLORS['border'])
        sym_btn_text = "ABC" if self.keyboard_mode == 3 else "!@#"
        self.text_center_x(222, 236, sym_btn_text, '#aaaaaa', 14)
        
        backspace_pressed = self.pressed_button and self.pressed_button[0] == 'backspace'
        backspace_bg = COLORS['btn_pressed'] if backspace_pressed else '#2a2a2a'
        self.canvas.create_rectangle(265, 224, 350, 248, fill=backspace_bg, outline=COLORS['border'])
        self.text_center_x(307, 236, "<-", '#aaaaaa', 14)
        
        enter_pressed = self.pressed_button and self.pressed_button[0] == 'enter'
        enter_bg = '#0066cc' if enter_pressed else COLORS['primary']
        self.canvas.create_rectangle(360, 224, 440, 248, fill=enter_bg, outline=COLORS['primary'])
        self.text_center_x(400, 236, "OK", '#ffffff', 14)
    
    def draw_screen4(self):
        self.canvas.delete('all')
        
        if self.logo_tk:
            self.canvas.create_image(240, 45, image=self.logo_tk)
        
        dots = ""
        for i in range(3):
            if i < (self.connecting_anim_step % 4):
                dots += "●"
            else:
                dots += "○"
        
        self.text_center_x(240, 95, "Connecting" + dots, COLORS['text'], 20)
        
        self.text_center_x(240, 130, "Connecting to: " + self.wifis[self.selected_wifi], COLORS['gray'], 12)
        
        colors = [COLORS['primary'], COLORS['primary'], '#333333']
        for i in range(3):
            dot_color = colors[(i + self.connecting_anim_step) % 3]
            self.canvas.create_oval(215 + i*25, 160, 235 + i*25, 180, fill=dot_color)
        
        self.text_center_x(240, 210, "Please wait...", COLORS['gray'], 11)

if __name__ == "__main__":
    FirmwareSimulator()
