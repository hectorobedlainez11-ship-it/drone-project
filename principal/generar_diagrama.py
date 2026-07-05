from PIL import Image, ImageDraw, ImageFont
import os

W, H = 1800, 1100
img = Image.new("RGB", (W, H), "#ffffff")
draw = ImageDraw.Draw(img)

# Colors
C_POWER = "#e61919"
C_GND = "#000000"
C_SIGNAL = "#1a6fc4"
C_I2C = "#2e8b2e"
C_UART = "#d2691e"
C_BEC5V = "#e67300"
C_BEC12V = "#9b30ff"
C_BORDER = "#333"
C_TEXT = "#222"

try:
    font_s = ImageFont.truetype("arial.ttf", 11)
    font_m = ImageFont.truetype("arial.ttf", 13)
    font_l = ImageFont.truetype("arial.ttf", 16)
    font_b = ImageFont.truetype("arialbd.ttf", 18)
except:
    font_s = ImageFont.load_default()
    font_m = font_s
    font_l = font_s
    font_b = font_s

def bbox(draw, x, y, w, h, label_lines, fill="#f0f0f0", border=C_BORDER):
    draw.rectangle([x, y, x+w, y+h], fill=fill, outline=border, width=2)
    lines = label_lines if isinstance(label_lines, list) else [label_lines]
    total = len(lines)
    box_center_y = y + h/2
    start_y = box_center_y - (total-1)*8
    for i, line in enumerate(lines):
        draw.text(((x+w/2), start_y + i*16), line, fill=C_TEXT, font=font_m, anchor="mm")

def wire(dwg, x1, y1, x2, y2, color, label="", loff=(0,0), dash=False):
    if dash:
        dx, dy = x2-x1, y2-y1
        dist = max(abs(dx), abs(dy))
        seg = 6
        for s in range(0, dist, seg*2):
            t1 = s/dist
            t2 = min((s+seg)/dist, 1)
            sx1, sy1 = x1+dx*t1, y1+dy*t1
            sx2, sy2 = x1+dx*t2, y1+dy*t2
            dwg.line([sx1, sy1, sx2, sy2], fill=color, width=2)
    else:
        dwg.line([x1, y1, x2, y2], fill=color, width=2)
    if label:
        lx = (x1+x2)/2 + loff[0]
        ly = (y1+y2)/2 + loff[1]
        draw.text((lx, ly), label, fill=color, font=font_s, anchor="mm")

def pin(dwg, x, y, label, anchor="lm"):
    dwg.text((x, y), label, fill=C_TEXT, font=font_s, anchor=anchor)

# Title
draw.text((W/2, 20), "Diagrama de Circuito - Drone Cuadricoptero (Configuracion X)", fill=C_TEXT, font=font_b, anchor="mm")

# Legend
lx, ly = 1400, 50
draw.rectangle([lx, ly, lx+370, ly+190], fill="#fff", outline="#999", width=1)
draw.text((lx+185, ly+15), "Leyenda", fill=C_TEXT, font=font_m, anchor="mm")
legends = [
    (C_POWER, "Alimentacion (BAT)"),
    (C_GND, "GND / Tierra"),
    (C_SIGNAL, "Senial / PWM / Dato"),
    (C_I2C, "I2C (SDA/SCL)"),
    (C_UART, "UART (TX/RX)"),
    (C_BEC5V, "BEC 5V (Power HUB)"),
    (C_BEC12V, "BEC 12V (Power HUB)"),
]
for i, (c, lbl) in enumerate(legends):
    yy = ly + 40 + i*20
    draw.line([lx+15, yy, lx+45, yy], fill=c, width=3)
    draw.text((lx+55, yy), lbl, fill=C_TEXT, font=font_s, anchor="lm")

# === COMPONENTS ===

# 1. Arduino UNO
ax, ay = 80, 90
aw, ah = 280, 420
bbox(draw, ax, ay, aw, ah, ["Arduino UNO", "(Controlador de Vuelo)"])
pins_l = [
    (ay+30, "D0 (RX) <- UART ESP32"),
    (ay+52, "D1 (TX) -> UART ESP32"),
    (ay+78, "D3 -> ESC1 (PWM)"),
    (ay+100, "D5 -> ESC2 (PWM)"),
    (ay+122, "D6 -> ESC3 (PWM)"),
    (ay+144, "D9 -> ESC4 (PWM)"),
    (ay+166, "D10 -> LED rojo1"),
    (ay+188, "D13 -> LED rojo2"),
    (ay+270, "Vin/5V <- Power HUB 5V"),
    (ay+295, "GND (comun)"),
]
for yy, lbl in pins_l:
    pin(draw, ax+5, yy, lbl, "lm")
pins_r = [
    (ay+100, "A0 <- Vbat (div. tens.)"),
    (ay+130, "A4 (SDA) -> MPU6500"),
    (ay+160, "A5 (SCL) -> MPU6500"),
]
for yy, lbl in pins_r:
    pin(draw, ax+aw-5, yy, lbl, "rm")

# 2. MPU6500
mx, my = ax+aw+120, 130
mw, mh = 200, 140
bbox(draw, mx, my, mw, mh, ["MPU6500", "(IMU 6-DOF)"])
mpu_pins = [(my+25, "VCC <- 5V PH"), (my+45, "GND"), (my+68, "SCL <- A5"), (my+90, "SDA <- A4")]
for yy, lbl in mpu_pins:
    pin(draw, mx+5, yy, lbl, "lm")

# 3. ESP32-CAM
ex, ey = ax+aw+120, 330
ew, eh = 200, 200
bbox(draw, ex, ey, ew, eh, ["ESP32-CAM (AI-Thinker)"])
esp_pins = [(ey+30, "TX (GPIO1) -> Arduino RX"), (ey+55, "RX (GPIO3) <- Arduino TX"),
            (ey+85, "5V <- Power HUB 5V"), (ey+110, "GND"), (ey+145, "Camara OV2640")]
for yy, lbl in esp_pins:
    pin(draw, ex+5, yy, lbl, "lm")

# 4. Battery
bx, by = 80, 580
bw, bh = 180, 80
bbox(draw, bx, by, bw, bh, ["Bateria 3S LiPo ~11.1V"], fill="#ffe0e0")

# 5. Power HUB (NUEVO)
phx, phy = 310, 560
phw, phh = 240, 160
bbox(draw, phx, phy, phw, phh, ["Power HUB 5V/12V BEC", "(Dist. Poder + BEC)"], fill="#e6f0ff")
ph_labels = [
    (phy+25, "BAT+  Bateria"),
    (phy+50, "ESC1 ESC2 ESC3 ESC4"),
    (phy+80, "5V OUT -> Arduino, ESP32, MPU6500"),
    (phy+105, "12V OUT -> (FPV VTX futuro)"),
    (phy+130, "GND comun"),
]
for yy, lbl in ph_labels:
    pin(draw, phx+5, yy, lbl, "lm")

# 6. Voltage divider
vx, vy = 80, 700
vw, vh = 100, 60
bbox(draw, vx, vy, vw, vh, ["Divisor Tension", "A0 (~3.7V max)"], fill="#ffffd0")

# 7. ESCs + Motors
esc_data = [
    (800, 590, "ESC1 - Pin3", "Motor1 (Front-Right)"),
    (970, 590, "ESC2 - Pin5", "Motor2 (Rear-Left)"),
    (1140, 590, "ESC3 - Pin6", "Motor3 (Front-Left)"),
    (1310, 590, "ESC4 - Pin9", "Motor4 (Rear-Right)"),
]
for ecx, ecy, title, sub in esc_data:
    bbox(draw, ecx, ecy, 150, 75, [title, sub], fill="#e0ffe0")

# 8. LEDs
led1_x, led1_y = ax+aw+120, 610
led2_x, led2_y = ax+aw+120, 670
draw.rectangle([led1_x, led1_y, led1_x+100, led1_y+30], fill="#ffe0e0", outline=C_BORDER, width=2)
draw.text((led1_x+50, led1_y+10), "LED rojo1", fill=C_TEXT, font=font_s, anchor="mm")
draw.text((led1_x+50, led1_y+22), "220R", fill="#888", font=font_s, anchor="mm")

draw.rectangle([led2_x, led2_y, led2_x+100, led2_y+30], fill="#ffe0e0", outline=C_BORDER, width=2)
draw.text((led2_x+50, led2_y+10), "LED rojo2", fill=C_TEXT, font=font_s, anchor="mm")
draw.text((led2_x+50, led2_y+22), "220R", fill="#888", font=font_s, anchor="mm")

# === WIRES ===

# Battery -> Power HUB (power)
wire(draw, bx+bw, by+40, phx, phy+25, C_POWER, "BAT+", (-50, -10))
wire(draw, bx+bw, by+60, phx, phy+130, C_GND, "GND", (-50, 10))

# Power HUB -> ESCs (power distribution)
for i, (ecx, ecy, _, _) in enumerate(esc_data):
    lbl_p = "BAT+" if i == 0 else ""
    lbl_g = "GND" if i == 1 else ""
    wire(draw, phx+phw, phy+50, ecx, ecy+10, C_POWER, lbl_p, (20, -10))
    wire(draw, phx+phw, phy+50, ecx, ecy+65, C_GND, lbl_g, (20, 10))

# Power HUB 5V -> Arduino, ESP32, MPU6500
wire(draw, phx+10, phy+80, ax+5, ay+270, C_BEC5V, "5V", (0, -10))
wire(draw, mx+5, my+25, phx, phy+80, C_BEC5V, "5V", (20, -10))
wire(draw, ex+5, ey+85, phx, phy+80, C_BEC5V, "5V", (20, 10))

# Power HUB 12V -> future VTX label
draw.text((phx+phw-40, phy+105), "(futuro)", fill=C_BEC12V, font=font_s, anchor="rm")

# Battery -> Voltage divider
wire(draw, bx+bw, by+15, vx+50, vy, C_POWER, "Vbat ~11.1V", (30, -10))

# Voltage divider -> Arduino A0
wire(draw, vx+50, vy+vh, ax+aw-5, ay+100, C_SIGNAL, "", (0, 0))

# ESC -> Arduino PWM signals
pwm_info = [(ay+78, 3), (ay+100, 5), (ay+122, 6), (ay+144, 9)]
for i, (piny, pnum) in enumerate(pwm_info):
    ecx, ecy, _, _ = esc_data[i]
    wire(draw, ecx, ecy+30, ax+aw, piny, C_SIGNAL, f"Pin {pnum}", (0, -10))

# I2C
wire(draw, ax+aw-5, ay+130, mx, my+68, C_I2C, "A4 (SDA)", (30, -10))
wire(draw, ax+aw-5, ay+160, mx, my+90, C_I2C, "A5 (SCL)", (30, 10))

# UART
wire(draw, ax+aw-5, ay+30, ex, ey+30, C_UART, "Arduino RX <- ESP32 TX", (-60, -10))
wire(draw, ax+aw-5, ay+52, ex, ey+55, C_UART, "Arduino TX -> ESP32 RX", (-60, 10))

# LEDs from Arduino
wire(draw, ax+aw-5, ay+166, led1_x, led1_y+10, C_SIGNAL, "D10", (-20, -10))
wire(draw, ax+aw-5, ay+188, led2_x, led2_y+10, C_SIGNAL, "D13", (-20, 10))

# LED GND loop
wire(draw, led1_x+100, led1_y+15, led2_x+100, led2_y+15, C_GND, "", (0, 0))
wire(draw, led2_x+100, led2_y+15, 660, led2_y+15, C_GND, "", (0, 0))

# GND commun note
draw.text((400, ay+ah-20), "TODO: GND comun al Power HUB y entre todos los componentes", fill=C_GND, font=font_s, anchor="mm")

# Save PNG
out_path = r"C:\Users\obedl\AppData\Local\Temp\opencode\DRONE_DESDE_CERO-v2\diagrama_circuito.png"
img.save(out_path)
print(f"Diagrama creado: {out_path}")
