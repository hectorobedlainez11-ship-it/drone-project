import os
import sys
os.environ['PATH'] = r'C:\Program Files\MySQL\MySQL Workbench 8.0 CE' + os.pathsep + os.environ.get('PATH', '')

import svgwrite
import cairosvg

W, H = 1600, 1050
OUTPUT_SVG = os.path.join(os.path.dirname(__file__), "diagrama_circuito.svg")
OUTPUT_PNG = os.path.join(os.path.dirname(__file__), "diagrama_circuito.png")

# Color palette
POWER = '#DC143C'
GND = '#333333'
PWM = '#1E6EB8'
I2C = '#228B22'
UART = '#E67E22'
LED_SIG = '#8E44AD'
TEXT = '#1A1A1A'
BOX_BORDER = '#2C3E50'

FILL_ESP = '#FFF8E1'
FILL_MPU = '#E8F5E9'
FILL_ARDUINO = '#E3F2FD'
FILL_BAT = '#FFEBEE'
FILL_LED = '#FFFDE7'
FILL_ESC = '#E0F7FA'
FILL_CLUSTER = '#F0F4F8'

dwg = svgwrite.Drawing(OUTPUT_SVG, size=(f'{W}px', f'{H}px'))

# ── Helpers ──────────────────────────────────────────────────────

def box(x, y, w, h, label, fill='#FFFFFF', sub=None, big=False):
    dwg.add(dwg.rect(insert=(x, y), size=(w, h),
                     fill=fill, stroke=BOX_BORDER, stroke_width=2, rx=6, ry=6))
    fs = 20 if big else 16
    dwg.add(dwg.text(label, insert=(x + w//2, y + 28),
                     text_anchor='middle', font_size=fs, font_weight='bold', fill=TEXT,
                     font_family='Segoe UI, Arial, sans-serif'))
    if sub:
        dwg.add(dwg.text(sub, insert=(x + w//2, y + 48),
                         text_anchor='middle', font_size=13, fill='#555',
                         font_family='Segoe UI, Arial, sans-serif'))

def pin_label(x, y, text, anchor='start', color=TEXT):
    dx = -4 if anchor == 'end' else 4
    dwg.add(dwg.text(text, insert=(x + dx, y + 4),
                     text_anchor=anchor, font_size=10, fill=color, font_weight='bold',
                     font_family='Segoe UI, Arial, sans-serif'))

def route(points, color, width=2, dash=False):
    attrs = {'fill': 'none', 'stroke': color, 'stroke_width': width}
    if dash:
        attrs['stroke_dasharray'] = '6,3'
    d = 'M ' + ' L '.join(f'{px},{py}' for px, py in points)
    dwg.add(dwg.path(d=d, **attrs))

def dot(x, y, color=BOX_BORDER):
    dwg.add(dwg.circle(center=(x, y), r=3, fill=color))

def wire_label(x, y, text, color=TEXT, anchor='start'):
    dwg.add(dwg.text(text, insert=(x, y), text_anchor=anchor,
                     font_size=10, font_weight='bold', fill=color,
                     font_family='Segoe UI, Arial, sans-serif'))

def gnd_symbol(x, y):
    dwg.add(dwg.line(start=(x-10, y), end=(x+10, y), stroke=GND, stroke_width=2))
    dwg.add(dwg.line(start=(x-6, y+5), end=(x+6, y+5), stroke=GND, stroke_width=2))
    dwg.add(dwg.line(start=(x-2, y+10), end=(x+2, y+10), stroke=GND, stroke_width=2))

def resistor(x, y, w=24, h=10):
    pts = []
    segs = 6
    for i in range(segs + 1):
        px = x - w//2 + (w * i) // segs
        py = y + (h if i % 2 == 0 else -h)
        pts.append((px, py))
    d = 'M ' + ' L '.join(f'{px},{py}' for px, py in pts)
    dwg.add(dwg.path(d=d, fill='none', stroke='#444', stroke_width=2))

def led_symbol(x, y, sz=10, color='#E74C3C'):
    dwg.add(dwg.polygon([(x-sz, y-sz//2), (x-sz, y+sz//2), (x+sz, y)],
                        fill=color, stroke='#444', stroke_width=1.5))
    dwg.add(dwg.line(start=(x+sz-2, y-sz//2), end=(x+sz-2, y+sz//2),
                     stroke='#444', stroke_width=2.5))
    dwg.add(dwg.line(start=(x+sz+4, y-sz), end=(x+sz+10, y-sz-4),
                     stroke='#444', stroke_width=1.5))
    dwg.add(dwg.line(start=(x+sz+5, y-4), end=(x+sz+12, y-6),
                     stroke='#444', stroke_width=1.5))
    dwg.add(dwg.polygon([(x+sz+10, y-sz-4), (x+sz+7, y-sz-7), (x+sz+13, y-sz-6)],
                        fill='#444'))
    dwg.add(dwg.polygon([(x+sz+12, y-6), (x+sz+9, y-9), (x+sz+15, y-8)],
                        fill='#444'))

def battery_symbol(x, y):
    dwg.add(dwg.line(start=(x-12, y-14), end=(x-12, y+14), stroke='#444', stroke_width=5))
    dwg.add(dwg.line(start=(x+12, y-10), end=(x+12, y+10), stroke='#444', stroke_width=2.5))
    dwg.add(dwg.text('+', insert=(x-20, y+5), font_size=12, fill=POWER,
                     font_family='Segoe UI, Arial, sans-serif'))
    dwg.add(dwg.text('−', insert=(x+16, y+5), font_size=14, fill=GND,
                     font_family='Segoe UI, Arial, sans-serif'))

def motor_symbol(x, y, label):
    dwg.add(dwg.circle(center=(x, y), r=16, fill='#F5F5F5', stroke='#444', stroke_width=2))
    dwg.add(dwg.text(label, insert=(x, y+4), text_anchor='middle',
                     font_size=10, font_weight='bold', fill=TEXT,
                     font_family='Segoe UI, Arial, sans-serif'))
    for angle in [0, 120, 240]:
        import math
        rad = math.radians(angle)
        dx = 10 * math.cos(rad)
        dy = 10 * math.sin(rad)
        dwg.add(dwg.line(start=(x, y), end=(x+dx, y+dy), stroke='#444', stroke_width=1.5))


# ══════════════════════════════════════════════════════════════════
#  1. BACKGROUND + BORDER
# ══════════════════════════════════════════════════════════════════
dwg.add(dwg.rect(insert=(0, 0), size=(W, H), fill='#FFFFFF'))
dwg.add(dwg.rect(insert=(10, 10), size=(W-20, H-20),
                 fill='none', stroke=BOX_BORDER, stroke_width=1.5, rx=4, ry=4))

# ══════════════════════════════════════════════════════════════════
#  2. TITLE
# ══════════════════════════════════════════════════════════════════
dwg.add(dwg.text('DRONE DESDE CERO v2 — DIAGRAMA DE CIRCUITO',
                 insert=(W//2, 45), text_anchor='middle',
                 font_size=26, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))

# ══════════════════════════════════════════════════════════════════
#  3. COMPONENT BOXES
# ══════════════════════════════════════════════════════════════════

# -- ESP32-CAM (left, top) --
x_esp, y_esp, w_esp, h_esp = 70, 80, 220, 150
box(x_esp, y_esp, w_esp, h_esp, 'ESP32-CAM (AI-Thinker)', FILL_ESP, 'WiFi + Camera')

# -- MPU6500 (right, top) --
x_mpu, y_mpu, w_mpu, h_mpu = 1290, 80, 230, 150
box(x_mpu, y_mpu, w_mpu, h_mpu, 'MPU6500', FILL_MPU, '6-DOF IMU (I2C)')

# -- Arduino UNO (center) --
x_ard, y_ard, w_ard, h_ard = 370, 340, 760, 240
box(x_ard, y_ard, w_ard, h_ard, 'ARDUINO UNO', FILL_ARDUINO, 'Flight Controller', big=True)

# -- Battery + Divider (bottom-left) --
x_bat, y_bat, w_bat, h_bat = 70, 680, 250, 220
box(x_bat, y_bat, w_bat, h_bat, 'BATERÍA 3S LiPo', FILL_BAT, '11.1V + Divisor de Voltaje')

# -- LEDs (bottom-center) --
x_led, y_led, w_led, h_led = 500, 710, 240, 190
box(x_led, y_led, w_led, h_led, 'LEDs Indicadores', FILL_LED, 'Estado (armado/error)')

# -- ESC/Motor cluster (bottom-right) --
x_esc_cl, y_esc_cl, w_esc_cl, h_esc_cl = 900, 690, 640, 240
dwg.add(dwg.rect(insert=(x_esc_cl, y_esc_cl), size=(w_esc_cl, h_esc_cl),
                 fill=FILL_CLUSTER, stroke='#5D6D7E', stroke_width=1.5, rx=8, ry=8, stroke_dasharray='6,3'))
dwg.add(dwg.text('ESCs + MOTORES (Configuración X)',
                 insert=(x_esc_cl + w_esc_cl//2, y_esc_cl + 22),
                 text_anchor='middle', font_size=14, font_weight='bold', fill='#5D6D7E',
                 font_family='Segoe UI, Arial, sans-serif'))

# Individual ESC boxes in 2x2 grid
esc_w, esc_h = 240, 75
gap_x, gap_y = 30, 20
x_esc1 = x_esc_cl + 340
x_esc3 = x_esc_cl + 50
y_esc1 = y_esc_cl + 35
y_esc2 = y_esc_cl + 35 + esc_h + gap_y

# ESC1 (FR, CW)
x1, y1 = x_esc1, y_esc1
dwg.add(dwg.rect(insert=(x1, y1), size=(esc_w, esc_h),
                 fill=FILL_ESC, stroke=BOX_BORDER, stroke_width=1.5, rx=4, ry=4))
dwg.add(dwg.text('ESC1', insert=(x1+50, y1+28), text_anchor='middle', font_size=13, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('FR • CW', insert=(x1+50, y1+50), text_anchor='middle', font_size=11, fill='#555',
                 font_family='Segoe UI, Arial, sans-serif'))
motor_symbol(x1+190, y1+esc_h//2, 'M1')

# ESC3 (FL, CCW)
x3, y3 = x_esc3, y_esc1
dwg.add(dwg.rect(insert=(x3, y3), size=(esc_w, esc_h),
                 fill=FILL_ESC, stroke=BOX_BORDER, stroke_width=1.5, rx=4, ry=4))
dwg.add(dwg.text('ESC3', insert=(x3+50, y3+28), text_anchor='middle', font_size=13, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('FL • CCW', insert=(x3+50, y3+50), text_anchor='middle', font_size=11, fill='#555',
                 font_family='Segoe UI, Arial, sans-serif'))
motor_symbol(x3+190, y3+esc_h//2, 'M3')

# ESC2 (RL, CW)
x2, y2 = x_esc3, y_esc2
dwg.add(dwg.rect(insert=(x2, y2), size=(esc_w, esc_h),
                 fill=FILL_ESC, stroke=BOX_BORDER, stroke_width=1.5, rx=4, ry=4))
dwg.add(dwg.text('ESC2', insert=(x2+50, y2+28), text_anchor='middle', font_size=13, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('RL • CW', insert=(x2+50, y2+50), text_anchor='middle', font_size=11, fill='#555',
                 font_family='Segoe UI, Arial, sans-serif'))
motor_symbol(x2+190, y2+esc_h//2, 'M2')

# ESC4 (RR, CCW)
x4, y4 = x_esc1, y_esc2
dwg.add(dwg.rect(insert=(x4, y4), size=(esc_w, esc_h),
                 fill=FILL_ESC, stroke=BOX_BORDER, stroke_width=1.5, rx=4, ry=4))
dwg.add(dwg.text('ESC4', insert=(x4+50, y4+28), text_anchor='middle', font_size=13, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('RR • CCW', insert=(x4+50, y4+50), text_anchor='middle', font_size=11, fill='#555',
                 font_family='Segoe UI, Arial, sans-serif'))
motor_symbol(x4+190, y4+esc_h//2, 'M4')


# ══════════════════════════════════════════════════════════════════
#  4. INTERNAL COMPONENT DETAILS
# ══════════════════════════════════════════════════════════════════

# -- Battery internal --
bat_cx, bat_cy = x_bat + w_bat//2, y_bat + 55
battery_symbol(bat_cx, bat_cy)

dwg.add(dwg.text('+11.1V', insert=(x_bat + 15, y_bat + 90), font_size=11, fill=POWER,
                 font_family='Segoe UI, Arial, sans-serif'))

# Voltage divider
r1_x, r1_y = x_bat + 80, y_bat + 130
r2_x, r2_y = x_bat + 150, y_bat + 185

resistor(r1_x, r1_y, 30, 12)
dwg.add(dwg.text('R1', insert=(r1_x-10, r1_y+20), font_size=10, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))

resistor(r2_x, r2_y, 30, 12)
dwg.add(dwg.text('R2', insert=(r2_x-10, r2_y+20), font_size=10, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))

dwg.add(dwg.text('Ratio ~2.95:1', insert=(x_bat + w_bat//2, y_bat + 205),
                 text_anchor='middle', font_size=10, fill='#888',
                 font_family='Segoe UI, Arial, sans-serif'))

# Battery internal connections
route([(bat_cx-12, y_bat+50), (bat_cx-12, r1_y-12), (r1_x-15, r1_y-12), (r1_x-15, r1_y-12)], POWER)
route([(bat_cx+12, y_bat+50), (bat_cx+12, y_bat+170), (x_bat+220, y_bat+170)], GND)
route([(r1_x+15, r1_y), (x_bat+220, r1_y)], POWER)
route([(r2_x-15, r2_y), (r1_x+15, r1_y)], POWER)
route([(r2_x+15, r2_y), (x_bat+220, r2_y)], GND)

# A0 output from divider
a0_out_x = x_bat + w_bat
a0_out_y = y_bat + 130
dot(a0_out_x, a0_out_y, POWER)

# -- LEDs internal --
# LED1
led1_in_x = x_led - 5
led1_in_y = y_led + 55
led1_r_x = x_led + 40
led1_r_y = y_led + 55
led1_sym_x = x_led + 90
led1_sym_y = y_led + 55

resistor(led1_r_x, led1_r_y, 24, 10)
dwg.add(dwg.text('220Ω', insert=(led1_r_x, led1_r_y+20), text_anchor='middle', font_size=9, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
led_symbol(led1_sym_x, led1_sym_y)
dwg.add(dwg.text('Rojo 1', insert=(led1_sym_x+5, led1_sym_y+25), font_size=10, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('Pin 10', insert=(led1_in_x-30, led1_in_y+4), font_size=9, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
route([(led1_in_x, led1_in_y), (led1_r_x-12, led1_r_y)], LED_SIG)
route([(led1_r_x+12, led1_r_y), (led1_sym_x-10, led1_sym_y)], LED_SIG)
# LED1 GND
gnd_led1_x = x_led + 180
gnd_led1_y = y_led + 55
route([(led1_sym_x+8, led1_sym_y), (gnd_led1_x, gnd_led1_y)], GND)
gnd_symbol(gnd_led1_x, gnd_led1_y)

# LED2
led2_in_x = x_led - 5
led2_in_y = y_led + 120
led2_r_x = x_led + 40
led2_r_y = y_led + 120
led2_sym_x = x_led + 90
led2_sym_y = y_led + 120

resistor(led2_r_x, led2_r_y, 24, 10)
dwg.add(dwg.text('220Ω', insert=(led2_r_x, led2_r_y+20), text_anchor='middle', font_size=9, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
led_symbol(led2_sym_x, led2_sym_y)
dwg.add(dwg.text('Rojo 2', insert=(led2_sym_x+5, led2_sym_y+25), font_size=10, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
dwg.add(dwg.text('Pin 13', insert=(led2_in_x-30, led2_in_y+4), font_size=9, fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))
route([(led2_in_x, led2_in_y), (led2_r_x-12, led2_r_y)], LED_SIG)
route([(led2_r_x+12, led2_r_y), (led2_sym_x-10, led2_sym_y)], LED_SIG)
gnd_led2_x = x_led + 180
gnd_led2_y = y_led + 120
route([(led2_sym_x+8, led2_sym_y), (gnd_led2_x, gnd_led2_y)], GND)
gnd_symbol(gnd_led2_x, gnd_led2_y)

# ══════════════════════════════════════════════════════════════════
#  5. PIN LABELS ON COMPONENTS
# ══════════════════════════════════════════════════════════════════

# ESP32-CAM pins (right edge)
esp_rx_x = x_esp + w_esp
esp_tx_y = y_esp + 45
esp_rx_y = y_esp + 85
pin_label(esp_rx_x, esp_tx_y, 'TX (GPIO1)', 'end')
pin_label(esp_rx_x, esp_rx_y, 'RX (GPIO3)', 'end')
pin_label(x_esp + 60, y_esp + h_esp, '5V', 'middle')
pin_label(x_esp + 150, y_esp + h_esp, 'GND', 'middle')

# MPU6500 pins (left edge)
mpu_scl_y = y_mpu + 45
mpu_sda_y = y_mpu + 85
pin_label(x_mpu, mpu_scl_y, 'SCL', 'start')
pin_label(x_mpu, mpu_sda_y, 'SDA', 'start')
pin_label(x_mpu + 70, y_mpu + h_mpu, 'VCC', 'middle')
pin_label(x_mpu + 150, y_mpu + h_mpu, 'GND', 'middle')

# Arduino left pins
ard_rx_y = y_ard + 50
ard_tx_y = y_ard + 85
ard_a0_y = y_ard + 130
ard_5v_y = y_ard + 175
ard_gnd_y = y_ard + 215
pin_label(x_ard, ard_rx_y, 'RX (D0)', 'start')
pin_label(x_ard, ard_tx_y, 'TX (D1)', 'start')
pin_label(x_ard, ard_a0_y, 'A0', 'start')
pin_label(x_ard, ard_5v_y, '5V', 'start')
pin_label(x_ard, ard_gnd_y, 'GND', 'start')

# Arduino bottom pins
pins_bottom = [
    (x_ard + 55, 'Pin 3'),
    (x_ard + 125, 'Pin 5'),
    (x_ard + 195, 'Pin 6'),
    (x_ard + 265, 'Pin 9'),
    (x_ard + 350, 'Pin 10'),
    (x_ard + 430, 'Pin 13'),
]
for px, plabel in pins_bottom:
    dot(px, y_ard + h_ard)
    pin_label(px, y_ard + h_ard + 5, plabel, 'middle')

# Arduino right pins
ard_sda_y = y_ard + 65
ard_scl_y = y_ard + 120
pin_label(x_ard + w_ard, ard_sda_y, 'A4 (SDA)', 'end')
pin_label(x_ard + w_ard, ard_scl_y, 'A5 (SCL)', 'end')


# ══════════════════════════════════════════════════════════════════
#  6. WIRES (routes)
# ══════════════════════════════════════════════════════════════════

# -- UART: ESP32 ↔ Arduino (Orange) --
# ESP32 TX → Arduino RX
tx_route = [
    (x_esp + w_esp, esp_tx_y),   # ESP32 TX
    (x_esp + w_esp + 30, esp_tx_y),
    (x_esp + w_esp + 30, ard_rx_y),
    (x_ard, ard_rx_y)            # Arduino RX
]
route(tx_route, UART)
wire_label(x_esp + w_esp + 5, esp_tx_y - 8, 'TX →', UART)

# ESP32 RX → Arduino TX
rx_route = [
    (x_esp + w_esp, esp_rx_y),   # ESP32 RX
    (x_esp + w_esp + 30, esp_rx_y),
    (x_esp + w_esp + 30, ard_tx_y),
    (x_ard, ard_tx_y)            # Arduino TX
]
route(rx_route, UART)
wire_label(x_esp + w_esp + 5, esp_rx_y - 8, 'RX →', UART)

# -- I2C: MPU6500 ↔ Arduino (Green) --
# MPU6500 SCL → Arduino A5
scl_route = [
    (x_mpu, mpu_scl_y),
    (x_mpu - 80, mpu_scl_y),
    (x_mpu - 80, ard_scl_y),
    (x_ard + w_ard, ard_scl_y)
]
route(scl_route, I2C)
wire_label(x_mpu - 5, mpu_scl_y - 8, 'SCL', I2C, 'end')

# MPU6500 SDA → Arduino A4
sda_route = [
    (x_mpu, mpu_sda_y),
    (x_mpu - 80, mpu_sda_y),
    (x_mpu - 80, ard_sda_y),
    (x_ard + w_ard, ard_sda_y)
]
route(sda_route, I2C)
wire_label(x_mpu - 5, mpu_sda_y - 8, 'SDA', I2C, 'end')

# -- Battery A0 → Arduino A0 (Red) --
bat_a0_route = [
    (a0_out_x, a0_out_y),
    (a0_out_x + 30, a0_out_y),
    (a0_out_x + 30, ard_a0_y),
    (x_ard, ard_a0_y)
]
route(bat_a0_route, POWER)
wire_label(a0_out_x + 5, a0_out_y - 8, 'A0 →', POWER)

# -- 5V from ESC BEC → Arduino 5V (Red) --
# Pick up from ESC3 area
bec_route = [
    (x_esc3 + esc_w//2, y_esc1 + esc_h),
    (x_esc3 + esc_w//2, y_esc1 + esc_h + 30),
    (x_ard + 60, y_esc1 + esc_h + 30),
    (x_ard + 60, ard_5v_y),
    (x_ard, ard_5v_y)
]
route(bec_route, POWER)
wire_label(x_ard + 5, ard_5v_y - 8, '5V BEC', POWER)

# -- GND from ESC → Arduino GND (Black) --
gnd_route = [
    (x_esc3 + esc_w//2 - 40, y_esc1 + esc_h),
    (x_esc3 + esc_w//2 - 40, y_esc1 + esc_h + 40),
    (x_ard + 140, y_esc1 + esc_h + 40),
    (x_ard + 140, ard_gnd_y),
    (x_ard, ard_gnd_y)
]
route(gnd_route, GND)
wire_label(x_ard + 5, ard_gnd_y - 8, 'GND', GND)

# -- PWM: Arduino → ESCs (Blue) --
# Pin 3 → ESC1 (FR, CW) — right column, top row
pwm1 = [
    (x_ard + 55, y_ard + h_ard),
    (x_ard + 55, y_ard + h_ard + 35),
    (x_esc1 + esc_w//2, y_ard + h_ard + 35),
    (x_esc1 + esc_w//2, y_esc1)
]
route(pwm1, PWM)
wire_label(x_ard + 55, y_ard + h_ard + 12, 'Pin 3 → ESC1', PWM, 'middle')

# Pin 5 → ESC2 (RL, CW) — left column, bottom row
pwm2 = [
    (x_ard + 125, y_ard + h_ard),
    (x_ard + 125, y_ard + h_ard + 50),
    (x_esc3 + esc_w//2, y_ard + h_ard + 50),
    (x_esc3 + esc_w//2, y_esc2)
]
route(pwm2, PWM)
wire_label(x_ard + 125, y_ard + h_ard + 12, 'Pin 5 → ESC2', PWM, 'middle')

# Pin 6 → ESC3 (FL, CCW) — left column, top row
pwm3 = [
    (x_ard + 195, y_ard + h_ard),
    (x_ard + 195, y_ard + h_ard + 35),
    (x_esc3 + esc_w//2, y_ard + h_ard + 35),
    (x_esc3 + esc_w//2, y_esc1)
]
route(pwm3, PWM)
wire_label(x_ard + 195, y_ard + h_ard + 12, 'Pin 6 → ESC3', PWM, 'middle')

# Pin 9 → ESC4 (RR, CCW) — right column, bottom row
pwm4 = [
    (x_ard + 265, y_ard + h_ard),
    (x_ard + 265, y_ard + h_ard + 50),
    (x_esc1 + esc_w//2, y_ard + h_ard + 50),
    (x_esc1 + esc_w//2, y_esc2)
]
route(pwm4, PWM)
wire_label(x_ard + 265, y_ard + h_ard + 12, 'Pin 9 → ESC4', PWM, 'middle')

# -- LED signals: Arduino → LEDs (Purple) --
# Pin 10 → LED1
led1_route = [
    (x_ard + 350, y_ard + h_ard),
    (x_ard + 350, y_ard + h_ard + 30),
    (x_led - 5, y_ard + h_ard + 30),
    (x_led - 5, led1_in_y)
]
route(led1_route, LED_SIG)
wire_label(x_ard + 350, y_ard + h_ard + 12, 'Pin 10 → LED1', LED_SIG, 'middle')

# Pin 13 → LED2
led2_route = [
    (x_ard + 430, y_ard + h_ard),
    (x_ard + 430, y_ard + h_ard + 30),
    (x_led - 5, y_ard + h_ard + 30),
    (x_led - 5, led2_in_y)
]
route(led2_route, LED_SIG)
wire_label(x_ard + 430, y_ard + h_ard + 12, 'Pin 13 → LED2', LED_SIG, 'middle')

# -- GND for battery --
bat_gnd_y = y_bat + 195
route([(x_bat + w_bat - 30, bat_gnd_y), (x_bat + w_bat - 30, y_bat + h_bat + 20)], GND)
gnd_symbol(x_bat + w_bat - 30, y_bat + h_bat + 20)

# -- Common GND symbol for LEDs box --
route([(x_led + w_led - 20, y_led + h_led), (x_led + w_led - 20, y_led + h_led + 15)], GND)
gnd_symbol(x_led + w_led - 20, y_led + h_led + 15)

# -- MPU6500 VCC to 5V --
vcc_route = [
    (x_mpu + 70, y_mpu + h_mpu),
    (x_mpu + 70, y_mpu + h_mpu + 20),
    (x_mpu + 70, 300),
]
route(vcc_route, POWER, dash=True)
wire_label(x_mpu + 70, y_mpu + h_mpu + 25, 'a +5V', POWER, 'middle')

# -- ESP32 5V --
route([(x_esp + 60, y_esp + h_esp), (x_esp + 60, y_esp + h_esp + 15)], POWER)
wire_label(x_esp + 60, y_esp + h_esp + 18, '+5V', POWER, 'middle')

# -- ESP32 GND --
route([(x_esp + 150, y_esp + h_esp), (x_esp + 150, y_esp + h_esp + 15)], GND)
gnd_symbol(x_esp + 150, y_esp + h_esp + 15)

# -- MPU6500 GND --
route([(x_mpu + 150, y_mpu + h_mpu), (x_mpu + 150, y_mpu + h_mpu + 20)], GND)
gnd_symbol(x_mpu + 150, y_mpu + h_mpu + 20)

# -- Battery + to R1 --
route([(bat_cx - 12, y_bat + 50), (r1_x - 15, r1_y - 12), (r1_x - 15, r1_y)], POWER)

# -- Battery - to GND --
route([(bat_cx + 12, y_bat + 50), (bat_cx + 12, y_bat + 170), (x_bat + w_bat - 30, y_bat + 170)], GND)

# -- R1 to A0 out --
route([(r1_x + 15, r1_y), (a0_out_x, a0_out_y)], POWER)
dot(a0_out_x - 3, a0_out_y, POWER)

# -- R1 to R2 junction --
route([(r1_x + 15, r1_y), (r2_x - 15, r2_y)], POWER)

# -- R2 to GND --
route([(r2_x + 15, r2_y), (x_bat + w_bat - 30, r2_y), (x_bat + w_bat - 30, y_bat + 170)], GND)


# ══════════════════════════════════════════════════════════════════
#  7. LEGEND
# ══════════════════════════════════════════════════════════════════
legend_x, legend_y = 60, 960
legend_w, legend_h = 1480, 70

dwg.add(dwg.rect(insert=(legend_x, legend_y), size=(legend_w, legend_h),
                 fill='#F9F9F9', stroke=BOX_BORDER, stroke_width=1.5, rx=6, ry=6))
dwg.add(dwg.text('L E Y E N D A',
                 insert=(legend_x + 120, legend_y + 28),
                 text_anchor='middle', font_size=14, font_weight='bold', fill=TEXT,
                 font_family='Segoe UI, Arial, sans-serif'))

legend_items = [
    (200, POWER, 'Alimentación (VCC, 5V, Batería +)'),
    (440, GND, 'Tierra (GND)'),
    (640, PWM, 'PWM (ESCs)'),
    (840, I2C, 'I2C (SDA, SCL)'),
    (1040, UART, 'UART (TX, RX)'),
    (1240, LED_SIG, 'GPIO (LEDs)'),
]

for lx, lcolor, ltext in legend_items:
    dwg.add(dwg.line(start=(lx, legend_y + 35), end=(lx + 40, legend_y + 35),
                     stroke=lcolor, stroke_width=3))
    dwg.add(dwg.text(ltext, insert=(lx + 48, legend_y + 39),
                     font_size=12, fill=TEXT,
                     font_family='Segoe UI, Arial, sans-serif'))

# ══════════════════════════════════════════════════════════════════
#  8. SAVE & EXPORT
# ══════════════════════════════════════════════════════════════════
dwg.save()
print(f'SVG saved: {OUTPUT_SVG}')

cairosvg.svg2png(url=OUTPUT_SVG, write_to=OUTPUT_PNG,
                 output_width=W, output_height=H)
print(f'PNG saved: {OUTPUT_PNG}')
print(f'Dimensions: {W}x{H} pixels')
