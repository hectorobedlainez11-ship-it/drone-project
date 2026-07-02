# Diagrama de conexiones fisicas - DRONE DESDE CERO v2
# Solo muestra pines realmente usados, con layout correcto

import os, svgwrite

# Intentar importar cairosvg para convertir a PNG
HAS_CAIRO = False
try:
    import cairocffi
    import cairosvg
    HAS_CAIRO = True
except (ImportError, OSError):
    HAS_CAIRO = False

W, H = 1600, 1050
OUT = os.path.join(os.path.dirname(__file__), "diagrama_circuito")
dwg = svgwrite.Drawing(OUT+".svg", size=(f"{W}px", f"{H}px"))

C_PWR = "#DC143C"
C_GND = "#333"
C_PWM = "#1E6EB8"
C_I2C = "#228B22"
C_UART = "#E67E22"
C_LED = "#8E44AD"

dwg.add(dwg.rect((0,0),(W,H),fill="#FFF"))
dwg.add(dwg.rect((10,10),(W-20,H-20),fill="none",stroke="#2C3E50",stroke_width=1.5,rx=4))
dwg.add(dwg.text("DRONE DESDE CERO v2 — DIAGRAMA DE CONEXIONES",
                 (W//2,40),text_anchor="middle",font_size=24,
                 font_weight="bold",fill="#1A1A1A",
                 font_family="Segoe UI, Arial, sans-serif"))

def txt(x,y,s,a="middle",c="#1A1A1A",sz=12,b=True):
    dwg.add(dwg.text(s,(x,y),text_anchor=a,font_size=sz,
                     font_weight="bold" if b else "normal",
                     fill=c,font_family="Segoe UI, Arial, sans-serif"))

def ruta(pts,color,w=2,dash=False):
    a={"fill":"none","stroke":color,"stroke_width":w}
    if dash: a["stroke_dasharray"]="6,3"
    dwg.add(dwg.path("M "+" L ".join(f"{px},{py}" for px,py in pts),**a))

def dot(x,y,c="#2C3E50"):
    dwg.add(dwg.circle((x,y),3,fill=c))

def tierra(x,y):
    for i,dy in enumerate([0,5,10]):
        dwg.add(dwg.line((x-8+i*4,y+dy),(x+8-i*4,y+dy),stroke=C_GND,stroke_width=2))

def resistencia(x,y):
    pts=[(x-12+(24*i)//6,y+(6 if i%2==0 else -6)) for i in range(7)]
    dwg.add(dwg.path("M "+" L ".join(f"{px},{py}" for px,py in pts),fill="none",stroke="#444",stroke_width=2))

def condensador(x,y,rotado=False):
    if rotado:
        dwg.add(dwg.line((x-6,y-10),(x-6,y+10),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x+6,y-10),(x+6,y+10),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x-6,y-6),(x+6,y-6),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x-6,y+6),(x+6,y+6),stroke="#444",stroke_width=2))
    else:
        dwg.add(dwg.line((x-10,y-6),(x+10,y-6),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x-10,y+6),(x+10,y+6),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x-6,y-6),(x-6,y+6),stroke="#444",stroke_width=2))
        dwg.add(dwg.line((x+6,y-6),(x+6,y+6),stroke="#444",stroke_width=2))

def led(x,y):
    dwg.add(dwg.polygon([(x-8,y-4),(x-8,y+4),(x+8,y)],fill="#E74C3C",stroke="#444",stroke_width=1.5))
    dwg.add(dwg.line((x+6,y-4),(x+6,y+4),stroke="#444",stroke_width=2))

def motor(x,y,lab):
    dwg.add(dwg.circle((x,y),14,fill="#F5F5F5",stroke="#444",stroke_width=2))
    txt(x,y+4,lab,"middle","#1A1A1A",10,True)

# ══════════════════════════════════════════════════════════════════
#  ARDUINO UNO - SOLO PINES USADOS
#  Tamaño: 500x500, empieza en (300, 80)
# ══════════════════════════════════════════════════════════════════
AX, AY, AW, AH = 300, 80, 500, 500
dwg.add(dwg.rect((AX,AY),(AW,AH),fill="#E3F2FD",stroke="#2C3E50",stroke_width=2,rx=6))
txt(AX+AW//2,AY+25,"ARDUINO UNO","middle","#1A1A1A",20,True)
txt(AX+AW//2,AY+45,"(Flight Controller)","middle","#555",13)

# Header izquierdo (digitales) - solo los usados, espaciados
# Posiciones Y relativas a AY
L = {
    "D0(RX)":   60,   # 140
    "D1(TX)":   100,  # 180
    "D3→ESC1":  180,  # 260
    "D5→ESC2":  240,  # 320
    "D6→ESC3":  290,  # 370
    "D9→ESC4":  360,  # 440
    "D10→LED1": 410,  # 490
    "D13→LED2": 460,  # 540
}
for pname, py in L.items():
    ly = AY+py
    dwg.add(dwg.line((AX,ly-8),(AX,ly+8),stroke="#2C3E50",stroke_width=1.5))
    dot(AX,ly)
    txt(AX-6,ly+4,pname,"end","#1A1A1A",9,False)

# Header derecho (analog+power) - solo los usados
R = {
    "A0(Bat)":   70,   # 150
    "A4(SDA)":   160,  # 240
    "A5(SCL)":   200,  # 280
    "5V(BEC)":   280,  # 360
    "GND":       350,  # 430
}
for pname, py in R.items():
    ly = AY+py
    dwg.add(dwg.line((AX+AW,ly-8),(AX+AW,ly+8),stroke="#2C3E50",stroke_width=1.5))
    dot(AX+AW,ly)
    txt(AX+AW+6,ly+4,pname,"start","#1A1A1A",9,False)

# ══════════════════════════════════════════════════════════════════
#  ESP32-CAM (izquierda)
# ══════════════════════════════════════════════════════════════════
EX, EY, EW, EH = 30, 120, 180, 130
dwg.add(dwg.rect((EX,EY),(EW,EH),fill="#FFF8E1",stroke="#2C3E50",stroke_width=2,rx=6))
txt(EX+EW//2,EY+22,"ESP32-CAM","middle","#1A1A1A",15,True)
txt(EX+EW//2,EY+42,"WiFi + OV2640","middle","#555",11)
# Pines ESP32 usados
esp_tx = (EX+EW, AY+L["D0(RX)"])   # TX → D0, mismo Y
esp_rx = (EX+EW, AY+L["D1(TX)"])   # RX ← D1
dot(*esp_tx); dot(*esp_rx)
txt(esp_tx[0]+4,esp_tx[1]+4,"TX(GPIO1)","start","#1A1A1A",9)
txt(esp_rx[0]+4,esp_rx[1]+4,"RX(GPIO3)","start","#1A1A1A",9)
# Power ESP32
txt(EX+50,EY+EH+12,"+5V","middle",C_PWR,9)
txt(EX+130,EY+EH+12,"GND","middle",C_GND,9)
tierra(EX+130,EY+EH+22)

# ══════════════════════════════════════════════════════════════════
#  MPU6500 (derecha)
# ══════════════════════════════════════════════════════════════════
MX, MY, MW, MH = 1200, 100, 200, 130
dwg.add(dwg.rect((MX,MY),(MW,MH),fill="#E8F5E9",stroke="#2C3E50",stroke_width=2,rx=6))
txt(MX+MW//2,MY+22,"MPU6500","middle","#1A1A1A",15,True)
txt(MX+MW//2,MY+42,"6-DOF IMU (0x68)","middle","#555",11)
# Pines MPU (lado izquierdo)
mpu_sda = (MX, AY+R["A4(SDA)"])
mpu_scl = (MX, AY+R["A5(SCL)"])
dot(*mpu_sda); dot(*mpu_scl)
txt(mpu_sda[0]-4,mpu_sda[1]+4,"SDA","end","#1A1A1A",9)
txt(mpu_scl[0]-4,mpu_scl[1]+4,"SCL","end","#1A1A1A",9)
# Power MPU
txt(MX+60,MY+MH+10,"VCC","middle",C_PWR,9)
txt(MX+140,MY+MH+10,"GND","middle",C_GND,9)
tierra(MX+140,MY+MH+20)

# ══════════════════════════════════════════════════════════════════
#  ESCs + MOTORES (abajo, en configuracion X)
# ══════════════════════════════════════════════════════════════════
# Mapeo correcto: D3→ESC1, D5→ESC2, D6→ESC3, D9→ESC4
# Posiciones: ESC1=FR, ESC2=RL, ESC3=FL, ESC4=RR
ESC_Y = 680
ew, eh = 200, 65
gap = 25
# Centrar los 4 ESCs
x0 = (W - 2*ew - gap)//2 - 100

esc_data = [
    ("ESC1","FR(CW)", "M1", x0+ew+gap, ESC_Y, AY+L["D3→ESC1"]),
    ("ESC2","RL(CW)", "M2", x0,        ESC_Y+eh+gap, AY+L["D5→ESC2"]),
    ("ESC3","FL(CCW)","M3", x0,        ESC_Y, AY+L["D6→ESC3"]),
    ("ESC4","RR(CCW)","M4", x0+ew+gap, ESC_Y+eh+gap, AY+L["D9→ESC4"]),
]
for name, pos, mlab, ex, ey, py_ard in esc_data:
    dwg.add(dwg.rect((ex,ey),(ew,eh),fill="#E0F7FA",stroke="#2C3E50",stroke_width=1.5,rx=4))
    txt(ex+50,ey+20,name,"middle","#1A1A1A",12,True)
    txt(ex+50,ey+43,pos,"middle","#555",10)
    motor(ex+ew-25,ey+eh//2,mlab)

    # PWM wire: desde el pin en Arduino hasta el ESC
    # Sale horizontal a izquierda, baja vertical, entra al ESC
    mid_x = AX - 40
    ruta([(AX,py_ard),(mid_x,py_ard),(mid_x,ey+eh//2),(ex,ey+eh//2)],C_PWM)
    txt(mid_x,(py_ard+ey+eh//2)//2,f"{name}","middle",C_PWM,8)

# ══════════════════════════════════════════════════════════════════
#  BATERIA + DIVISOR (debajo de los ESCs)
# ══════════════════════════════════════════════════════════════════
BY = ESC_Y + 2*eh + 2*gap + 10  # justo debajo de los ESCs
BX, BW, BH = 380, 260, 100
dwg.add(dwg.rect((BX,BY),(BW,BH),fill="#FFEBEE",stroke="#2C3E50",stroke_width=2,rx=6))
txt(BX+BW//2,BY+15,"BATERIA 3S 11.1V","middle","#1A1A1A",12,True)
txt(BX+BW//2,BY+28,"Divisor R1/R2 = 2.95:1","middle","#555",9)

# Simbolo bateria
bx, by = BX+BW//2, BY+48
dwg.add(dwg.line((bx-8,by-8),(bx-8,by+8),stroke="#444",stroke_width=3))
dwg.add(dwg.line((bx+8,by-6),(bx+8,by+6),stroke="#444",stroke_width=2))
txt(bx-12,by+4,"+","end",C_PWR,10,True)
txt(bx+12,by+4,"-","start",C_GND,10,True)

# Divisor simplificado (solo texto)
r_y = BY+70
txt(BX+70,r_y,"R1 100kΩ","middle","#1A1A1A",8,False)
txt(BX+140,r_y,"R2 33kΩ","middle","#1A1A1A",8,False)
txt(BX+BW//2,BY+88,"A0 = Vbat·R2/(R1+R2) ≈ 4.27V max","middle","#555",8,False)

# Conexiones bateria->divisor->A0
a0_pin = (BX+BW+18, BY+70)
ruta([(bx-8,by),(BX+40,by),(BX+40,r_y-4)],C_PWR)
ruta([(bx+8,by),(bx+8,BY+BH-12),(BX+BW-15,BY+BH-12)],C_GND)
ruta([(BX+52,r_y),(BX+155,r_y)],C_PWR)
ruta([(BX+155,r_y),(BX+155,BY+BH-12),(BX+BW-15,BY+BH-12)],C_GND)
tierra(BX+BW-15,BY+BH-3)
# Salida A0 a Arduino
ruta([(BX+52,r_y),(a0_pin[0],a0_pin[1])],C_PWR)
dot(*a0_pin,C_PWR)

# ══════════════════════════════════════════════════════════════════
#  LEDs (abajo, al lado de la bateria)
# ══════════════════════════════════════════════════════════════════
LX, LY, LW, LH = 200, 700, 140, 160
dwg.add(dwg.rect((LX,LY),(LW,LH),fill="#FFFDE7",stroke="#2C3E50",stroke_width=2,rx=6))
txt(LX+LW//2,LY+18,"LEDs","middle","#1A1A1A",13,True)

for i,(pin_name,offs) in enumerate([("LED1(D10)",35),("LED2(D13)",95)]):
    ly = LY + offs
    rr = LX + 40
    resistencia(rr, ly)
    txt(rr,ly+16,"220Ω","middle","#1A1A1A",8)
    ll = LX + 90
    led(ll, ly)
    txt(ll+12,ly+4,pin_name,"start","#1A1A1A",8)
    # Cable desde borde izquierdo
    ruta([(LX-5,ly),(rr-10,ly)],C_LED)
    ruta([(rr+10,ly),(ll-8,ly)],C_LED)
    # GND
    gx_ = LX+LW-20
    ruta([(ll+8,ly),(gx_,ly)],C_GND)
    if i==0:
        ruta([(gx_,ly),(gx_,ly+95-35)],C_GND)

# ══════════════════════════════════════════════════════════════════
#  CABLEADO ENTRE COMPONENTES
# ══════════════════════════════════════════════════════════════════

# 1. UART: ESP32 ↔ D0/D1 (naranja) ────────────────────────────────
# ESP32 TX → Arduino D0(RX) - misma altura (3.3V→5V, seguro)
ruta([(EX+EW,AY+L["D0(RX)"]),(AX,AY+L["D0(RX)"])],C_UART)
txt((EX+EW+AX)//2,AY+L["D0(RX)"]-6,"TX→RX","middle",C_UART,9)

# ESP32 RX ← Arduino D1(TX) - CON DIVISOR DE VOLTAJE 5V→3.3V
# Arduino TX (5V) → R1 1kΩ → MID → R2 2kΩ → GND ───→ ESP32 RX (3.3V tolerante)
uart_y = AY+L["D1(TX)"]
r1_mid = AX - 60   # x=240
r1_x2 = AX - 40    # x=260 (inicio del resistor R1)
r1_x1 = AX - 25    # x=275 (fin del resistor R1)
r2_mid = r1_mid    # x=240
# Tramo Arduino → R1
ruta([(AX,uart_y),(r1_x2,uart_y)],C_UART)
# Resistor R1
resistencia(r1_mid, uart_y)
txt(r1_mid, uart_y+14,"R1","middle","#1A1A1A",8)
# R2 (a GND) - solo bajamos una "rama" desde el mid point
div_gnd_y = uart_y + 40
ruta([(r1_mid,uart_y),(r1_mid,div_gnd_y)],C_UART)
resistencia(r1_mid, (uart_y+div_gnd_y)//2)
txt(r1_mid+10,(uart_y+div_gnd_y)//2,"R2→GND","start","#1A1A1A",7)
tierra(r1_mid, div_gnd_y)
# Tramo divisor → ESP32 RX
ruta([(r1_mid,uart_y),(EX+EW,uart_y)],C_UART)
txt((r1_mid+EX+EW)//2,uart_y+14,"5V→3.3V","middle","#E67E22",8)
# Label
txt(r1_mid-15, uart_y-8,"1kΩ","end","#1A1A1A",7)
txt(r1_mid-15, div_gnd_y-18,"2kΩ","end","#1A1A1A",7)

# 2. I2C: Arduino A4/A5 → MPU6500 (verde) ─────────────────────────
# A4(SDA) → MPU SDA
ruta([(AX+AW,AY+R["A4(SDA)"]),(AX+AW+40,AY+R["A4(SDA)"]),
      (AX+AW+40,AY+R["A4(SDA)"]),(MX,mpu_sda[1])],C_I2C)
txt(AX+AW+45,AY+R["A4(SDA)"]-6,"SDA→","start",C_I2C,9)
# A5(SCL) → MPU SCL
ruta([(AX+AW,AY+R["A5(SCL)"]),(AX+AW+40,AY+R["A5(SCL)"]),
      (AX+AW+40,AY+R["A5(SCL)"]),(MX,mpu_scl[1])],C_I2C)
txt(AX+AW+45,AY+R["A5(SCL)"]+14,"SCL→","start",C_I2C,9)

# 3. A0: Bateria → Arduino A0 (rojo) ──────────────────────────────
ard_a0_y = AY+R["A0(Bat)"]
a0_mid_x = AX - 5  # justo a la izquierda del Arduino
ruta([(a0_pin[0],a0_pin[1]),(a0_pin[0],BY+BH+5),
      (a0_mid_x,BY+BH+5),(a0_mid_x,ard_a0_y),(AX+AW,ard_a0_y)],C_PWR)
txt(a0_mid_x,(BY+BH+5+ard_a0_y)//2,"A0←Bat","middle",C_PWR,9)

# 4. D10/D13 → LEDs (purpura) ─────────────────────────────────────
# D10 → LED1
d10_y = AY+L["D10→LED1"]
lx_l1 = LX-5
ly_l1 = LY+35
# Ruta: baja desde D10, gira a izquierda, baja mas, gira a derecha hacia LED
inter_x = AX-25
ruta([(AX,d10_y),(inter_x,d10_y),(inter_x,ly_l1),(lx_l1,ly_l1)],C_LED)
txt(inter_x,(d10_y+ly_l1)//2,"D10→LED1","middle",C_LED,8)

# D13 → LED2
d13_y = AY+L["D13→LED2"]
ly_l2 = LY+95
ruta([(AX,d13_y),(inter_x,d13_y),(inter_x,ly_l2),(lx_l1,ly_l2)],C_LED)
txt(inter_x,(d13_y+ly_l2)//2,"D13→LED2","middle",C_LED,8)

# 5. 5V BEC: desde ESC → Arduino 5V (rojo punteado) ──────────────
ard_5v_y = AY+R["5V(BEC)"]
bec_esc_x = x0+ew+gap + 50
bec_esc_y = ESC_Y+eh
esc_grid_bottom = ESC_Y + 2*eh + 2*gap  # parte inferior de la parrilla ESC
ruta([(bec_esc_x,bec_esc_y),(bec_esc_x,esc_grid_bottom+5),
      (AX+AW+20,esc_grid_bottom+5),(AX+AW+20,ard_5v_y),(AX+AW,ard_5v_y)],C_PWR,dash=True)
txt(bec_esc_x,esc_grid_bottom-8,"5V BEC→","middle",C_PWR,9)

# 6. GND comun ────────────────────────────────────────────────────
ard_gnd_y = AY+R["GND"]
gnd_esc_x = x0+50
gnd_esc_y = ESC_Y+eh
ruta([(gnd_esc_x,gnd_esc_y),(gnd_esc_x,esc_grid_bottom+5),
      (AX+AW+20,esc_grid_bottom+5),(AX+AW+20,ard_gnd_y),(AX+AW,ard_gnd_y)],C_GND)
txt(gnd_esc_x,esc_grid_bottom-8,"GND","middle",C_GND,9)
tierra(gnd_esc_x,esc_grid_bottom+15)

# 7. 5V: Arduino → ESP32-CAM (rojo) ─────────────────────────────
ard_5v_y = AY+R["5V(BEC)"]
esp_vcc_x = EX+EW-30
esp_vcc_y = EY+EH+8
# Ruta: desde pin 5V del Arduino, baja y cruza por debajo, sube a ESP32
ruta([(AX+AW,ard_5v_y),(AX+AW+15,ard_5v_y),
      (AX+AW+15,EY+EH+15),(EX+EW+15,EY+EH+15),
      (EX+EW+15,EY+EH+8),(esp_vcc_x,EY+EH+8)],C_PWR)
txt(esp_vcc_x,EY+EH+20,"+5V","middle",C_PWR,9,True)

# 8. Decoupling capacitor en ESP32-CAM ──────────────────────────
cap_x, cap_y = EX+50, EY+EH+2
condensador(cap_x, cap_y)
txt(cap_x-5, cap_y+18,"C1","end","#1A1A1A",7)
txt(cap_x+5, cap_y+18,"100µF","start","#1A1A1A",7)
txt(cap_x, cap_y+28,"+ cap 0.1µF","middle","#1A1A1A",7)

# 9. GND: Arduino → ESP32-CAM y MPU6500 ─────────────────────────
ard_gnd_y2 = AY+R["GND"]
ruta([(AX+AW,ard_gnd_y2),(AX+AW+15,ard_gnd_y2),
      (AX+AW+15,EY+EH+22),(EX+EW,EY+EH+22)],C_GND)
txt(AX+AW+15,(ard_gnd_y2+EY+EH+22)//2,"GND→ESP32","middle",C_GND,8)

# ══════════════════════════════════════════════════════════════════
#  LEYENDA
# ══════════════════════════════════════════════════════════════════
lx,ly = 60, 1005
dwg.add(dwg.rect((lx,ly),(W-120,30),fill="#F9F9F9",stroke="#2C3E50",stroke_width=1.5,rx=6))
txt(lx+40,ly+20,"LEYENDA:","start","#1A1A1A",10,True)
items=[(200,C_PWR,"Alim."),(340,C_GND,"Tierra"),
       (500,C_PWM,"PWM"),(620,C_I2C,"I2C"),
       (740,C_UART,"UART"),(880,C_LED,"GPIO")]
for ix,ic,it in items:
    dwg.add(dwg.line((ix,ly+15),(ix+25,ly+15),stroke=ic,stroke_width=2.5))
    txt(ix+30,ly+19,it,"start","#1A1A1A",9)

# ══════════════════════════════════════════════════════════════════
#  GUARDAR
# ══════════════════════════════════════════════════════════════════
dwg.save()
print(f"OK: {OUT}.svg guardado")
if HAS_CAIRO:
    cairosvg.svg2png(url=OUT+".svg",write_to=OUT+".png",output_width=W,output_height=H)
    print(f"OK: {OUT}.png ({W}x{H})")
else:
    print(f"[SKIP] PNG conversion: cairo library no disponible")
    print(f"[HINT] Instalar con: pip install cairosvg y tener cairo.dll en PATH")
