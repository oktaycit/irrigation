# 🔌 STM32F407VET6 - Donanım Bağlantı Şeması

## 1. Sistem Blok Diyagramı

```
                                    ┌─────────────────────────────────────┐
                                    │         STM32F407VET6               │
                                    │           MCU                       │
                                    │                                     │
┌─────────────┐                     │  ┌─────────────────────────────┐   │
│  TFT LCD    │◄────FSMC────────────┼──┤ FSMC Controller             │   │
│  3.2"       │                     │  │ (PD0-PD15, PE0-PE15)        │   │
│  ILI9341    │                     │  └─────────────────────────────┘   │
│             │                     │                                     │
│  XPT2046    │◄────SPI1────────────┼──┤ SPI1 (PA5-PA7)              │   │
└─────────────┘                     │  └─────────────────────────────┘   │
                                    │                                     │
┌─────────────┐                     │  ┌─────────────────────────────┐   │
│ pH Sensörü  │◄────ADC1_IN0────────┼──┤ ADC1 (PA0)                  │   │
│  0-14 pH    │                     │  └─────────────────────────────┘   │
└─────────────┘                     │                                     │
                                    │  ┌─────────────────────────────┐   │
┌─────────────┐                     │  │ ADC1 (PA1)                  │   │
│ EC Sensörü  │◄────ADC1_IN1────────┼──┤                             │   │
│  0-20mS/cm  │                     │  └─────────────────────────────┘   │
└─────────────┘                     │                                     │
                                    │  ┌─────────────────────────────┐   │
┌─────────────┐                     │  │ GPIO (PB0-PB7)              │   │
│ Vana 1-8    │◄────GPIO────────────┼──┤ ULN2803 Sürücü              │   │
│ 12V/24V     │                     │  └─────────────────────────────┘   │
└─────────────┘                     │                                     │
                                    │  ┌─────────────────────────────┐   │
┌─────────────┐                     │  │ I2C1 (PB8-PB9)              │   │
│ EEPROM      │◄────I2C1────────────┼──┤ 24C256                      │   │
│ 24C256      │                     │  └─────────────────────────────┘   │
└─────────────┘                     │                                     │
                                    │  ┌─────────────────────────────┐   │
┌─────────────┐                     │  │ USART1 (PA9-PA10)           │   │
│ Debug UART  │◄────USART1──────────┼──┤ FT232/ST-Link               │   │
│             │                     │  └─────────────────────────────┘   │
└─────────────┘                     │                                     │
                                    │  ┌─────────────────────────────┐   │
┌─────────────┐                     │  │ TIM1_CH1 (PA8)              │   │
│ Backlight   │◄────PWM─────────────┼──┤ LCD Parlaklık               │   │
│             │                     │  └─────────────────────────────┘   │
└─────────────┘                     └─────────────────────────────────────┘
                                    │
                                    ▼
                              ┌─────────────┐
                              │ Güç Kaynağı │
                              │ 12V DC IN   │
                              │             │
                              │ 3.3V LDO    │
                              │ 5V LDO      │
                              │ 12V Direct  │
                              └─────────────┘
```

---

## 2. Güç Kaynağı Devresi

### 2.1 Giriş Katı
```
                    ┌─────────────────────────────────────┐
    12V DC Jack ────┤ J1                                  │
                    │                                     │
                    │  ┌───────┐     ┌───────┐           │
    GND ────────────┤──┤FUSE   ├─────┤DIODE  ├───────────│
                    │  │2A     │     │1N4007 │           │
                    │  └───────┘     └───────┘           │
                    │       │              │              │
                    │      GND           +12V_IN          │
                    └─────────────────────────────────────┘
```

### 2.2 3.3V LDO Regülatör (MCU için)
```
    +12V_IN
       │
       ├───┬──────────────────────────────────────┐
       │   │                                      │
       │  ┌┴┐ C1                                  │
       │  │ │ 10µF                                │
       │  └┬┘                                     │
       │   │                                      │
       │  ┌┴┐ C2                                  │
       │  │ │ 100nF                               │
       │  └┬┘                                     │
       │   │                                      │
       │  ┌┴┐ C3                                  │
       │  │ │ 10µF                                │
       │  └┬┘                                     │
       │   │                                      │
       │  ┌┴┐                                     │
       └──┤ │ AMS1117-3.3                         │
          │ │ VIN    VOUT─────────────────► +3.3V_MCU
          │ │ GND    ADJ                         │
          └┬┘         │                          │
           │         ┌┴┐ C4                      │
           │         │ │ 10µF                    │
           │         └┬┘                         │
           │          │                          │
          GND        GND                        GND

    +3.3V_MCU → STM32 VDD, VDDA
    GND → STM32 VSS, VSSA
```

### 2.3 5V LDO Regülatör (Display/Sensörler için)
```
    +12V_IN
       │
       │  ┌┴┐ C5
       │  │ │ 10µF
       │  └┬┘
       │   │
       │  ┌┴┐
       └──┤ │ AMS1117-5.0
          │ │ VIN    VOUT─────────────────► +5V
          │ │ GND    ADJ
          └┬┘         │
           │         ┌┴┐ C6
           │         │ │ 10µF
           │         └┬┘
           │          │
          GND        GND

    +5V → Display VCC, Sensörler, EEPROM
```

### 2.4 12V Direct (Vanalar için)
```
    +12V_IN ────────────────────────────────► +12V_VALVE
    
    +12V_VALVE → ULN2803 COM, Vana beslemesi
```

---

## 3. TFT LCD Bağlantısı (ILI9341)

### 3.1 FSMC Data Bus
```
┌──────────────────────────────────────────────────────────────────┐
│                    STM32F407VET6              ILI9341            │
│                                                                  │
│   PD14  ─────────────────────────────────────► D0 (DB0)         │
│   PD15  ─────────────────────────────────────► D1 (DB1)         │
│   PD0   ─────────────────────────────────────► D2 (DB2)         │
│   PD1   ─────────────────────────────────────► D3 (DB3)         │
│   PE7   ─────────────────────────────────────► D4 (DB4)         │
│   PE8   ─────────────────────────────────────► D5 (DB5)         │
│   PE9   ─────────────────────────────────────► D6 (DB6)         │
│   PE10  ─────────────────────────────────────► D7 (DB7)         │
│   PE11  ─────────────────────────────────────► D8 (DB8)         │
│   PE12  ─────────────────────────────────────► D9 (DB9)         │
│   PE13  ─────────────────────────────────────► D10 (DB10)       │
│   PE14  ─────────────────────────────────────► D11 (DB11)       │
│   PE15  ─────────────────────────────────────► D12 (DB12)       │
│   PD8   ─────────────────────────────────────► D13 (DB13)       │
│   PD9   ─────────────────────────────────────► D14 (DB14)       │
│   PD10  ─────────────────────────────────────► D15 (DB15)       │
│                                                                  │
│   PD7   ─────────────────────────────────────► CS/CE           │
│   PD11  ─────────────────────────────────────► RS/A0           │
│   PD5   ─────────────────────────────────────► WR/RNW          │
│   PD4   ─────────────────────────────────────► RD/OE           │
│   PE2   ─────────────────────────────────────► RST             │
│   PA8   ─────────────────────────────────────► BL (PWM)        │
│                                                                  │
│   +3.3V ─────────────────────────────────────► VCC             │
│   GND   ─────────────────────────────────────► GND             │
└──────────────────────────────────────────────────────────────────┘
```

### 3.2 Backlight PWM Devresi
```
    STM32 PA8 (TIM1_CH1)
         │
         │
        ┌┴┐
        │ │ 100Ω
        └┬┘
         │
         ├─── Base Q1 (2N3904)
         │
    +3.3V
         │
        ┌┴┐
        │ │ 10kΩ (Pull-up)
        └┬┘
         │
        GND

    +5V ────► LED Anode (LCD BL)
              LED Cathode ────► Collector Q1
              Emitter Q1 ────► GND
```

---

## 4. Dokunmatik Ekran (XPT2046) Bağlantısı

```
┌─────────────────────────────────────────────────────────────────┐
│                   STM32F407VET6              XPT2046            │
│                                                                 │
│   PA5 (SPI1_SCK)  ─────────────────────────► CLK               │
│   PA6 (SPI1_MISO) ─────────────────────────► DOUT              │
│   PA7 (SPI1_MOSI) ─────────────────────────► DIN               │
│   PB10 (GPIO)     ─────────────────────────► CS                │
│   PC15 (EXTI)     ─────────────────────────► IRQ               │
│                                                                 │
│   +3.3V           ─────────────────────────► VCC               │
│   GND             ─────────────────────────► GND               │
│                                                                 │
│   IRQ Pull-down: 10kΩ GND                                       │
│   CS Pull-up: 10kΩ +3.3V                                        │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. pH Sensörü Bağlantısı

### 5.1 Analog pH Sensör Modülü
```
┌─────────────────────────────────────────────────────────────────┐
│                   pH Sensör Modülü                              │
│                                                                 │
│   +5V ─────────────────────────────────────► VCC               │
│   GND ─────────────────────────────────────► GND               │
│   PA0 (ADC1_IN0) ◄─────────────────────────► PO (Analog Out)   │
│                                                                 │
│   pH Elektrot ──────────► BNC Connector ─────────► Modül      │
│                                                                 │
│   NOT: Analog çıkış 0-3.3V aralığında olmalı.                  │
│   5V modüller için gerilim bölücü kullanın:                    │
│   R1 = 10kΩ, R2 = 20kΩ (5V → 3.3V)                             │
│                                                                 │
│         Modül OUT                                               │
│            │                                                    │
│           ┌┴┐ R1 10kΩ                                           │
│           └┬┘                                                   │
│            ├─────────────► PA0 (ADC)                           │
│           ┌┴┐ R2 20kΩ                                           │
│           └┬┘                                                   │
│            │                                                    │
│           GND                                                   │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Op-Amp pH Devresi (Alternatif)
```
    pH Elektrot ────►┌────────────────────────────┐
                     │  MCP6002 (Op-Amp)          │
                     │                            │
    +3.3V ──┬───────►│+ IN                        │
            │        │                            │
           ┌┴┐ Rf    │                            │
           │ │ 100k  │     ┌──────────┐           │
           └┬┘       │- IN─►│          │           │
            │        │     │          │           │
           GND       │     │          │           │
                     │     │          │           │
                     │     │ OUT ─────┼─────────► PA0
                     │     │          │           │
                     └─────┴──────────┘           │
                                                  │
    MCP6002 VDD ─────────────────────────────────► +3.3V
    MCP6002 VSS ─────────────────────────────────► GND
```

---

## 6. EC Sensörü Bağlantısı

```
┌─────────────────────────────────────────────────────────────────┐
│                   EC Sensör Modülü                              │
│                                                                 │
│   +5V ─────────────────────────────────────► VCC               │
│   GND ─────────────────────────────────────► GND               │
│   PA1 (ADC1_IN1) ◄─────────────────────────► AO (Analog Out)   │
│                                                                 │
│   EC Elektrot ──────────► BNC/Connector ─────────► Modül      │
│                                                                 │
│   Sıcaklık Kompensasyonu (Opsiyonel):                          │
│   DS18B20 ───────────────────────────────► PB12 (1-Wire)       │
│                                                                 │
│   EC Hesaplama:                                                │
│   EC (mS/cm) = (ADC_değer / 4095) × 3.3 × K                    │
│   K = Kalibrasyon katsayısı (modüle göre değişir)              │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Vana Sürücü Devresi (ULN2803)

### 7.1 ULN2803 Bağlantısı
```
┌─────────────────────────────────────────────────────────────────────┐
│                        ULN2803 Darlington Array                     │
│                                                                     │
│   STM32      ULN2803           Vana                                │
│   GPIO       Input             Bağlantısı                           │
│                                                                     │
│   PB0  ────►│1B  1C│──────────► Vana1 (+) ────► +12V/24V          │
│   PB1  ────►│2B  2C│──────────► Vana2 (+) ────► +12V/24V          │
│   PB2  ────►│3B  3C│──────────► Vana3 (+) ────► +12V/24V          │
│   PB3  ────►│4B  4C│──────────► Vana4 (+) ────► +12V/24V          │
│   PB4  ────►│5B  5C│──────────► Vana5 (+) ────► +12V/24V          │
│   PB5  ────►│6B  6C│──────────► Vana6 (+) ────► +12V/24V          │
│   PB6  ────►│7B  7C│──────────► Vana7 (+) ────► +12V/24V          │
│   PB7  ────►│8B  8C│──────────► Vana8 (+) ────► +12V/24V          │
│                                                                     │
│   COM (Pin 9) ────────────────────────────────────► +12V          │
│   GND (Pin 10) ───────────────────────────────────► GND           │
│   GND (Pin 11) ───────────────────────────────────► GND           │
│                                                                     │
│   NOT: COM pini flyback diyotları için beslenir.                   │
│   Her girişe 1kΩ seri direnç önerilir.                             │
└─────────────────────────────────────────────────────────────────────┘

    GPIO ────┬─────► 1kΩ ────► ULN2803 Input
             │
            ┌┴┐
            │ │ 10kΩ (Pull-down, isteğe bağlı)
            └┬┘
             │
            GND
```

### 7.2 Alternatif: Röle Modülü
```
    STM32 GPIO ────► 1kΩ ────► NPN Transistor Base (2N2222)
                              │
    +12V ─────────────────────┤
                              │
    Röle Coil ────────────────┤ Collector
                              │
    Transistor Emitter ───────┴───► GND

    Röle Kontakları:
    COM ────► +12V/24V (Vana beslemesi)
    NO  ────► Vana (+)
    NC  ────► Boşta

    Flyback Diyot: 1N4148 röle coil'e ters paralel
```

---

## 8. EEPROM (24C256) Bağlantısı

```
┌─────────────────────────────────────────────────────────────────┐
│                   24C256 EEPROM                                 │
│                                                                 │
│   Pin 1 (A0) ────► GND                                         │
│   Pin 2 (A1) ────► GND                                         │
│   Pin 3 (A2) ────► GND                                         │
│   Pin 4 (VSS) ───► GND                                         │
│   Pin 5 (SDA) ───┼─────► PB9 (I2C1_SDA)                       │
│                  │      Pull-up: 4.7kΩ → +3.3V                 │
│   Pin 6 (SCL) ───┼─────► PB8 (I2C1_SCL)                       │
│                  │      Pull-up: 4.7kΩ → +3.3V                 │
│   Pin 7 (WP) ────► GND (Write Protect devre dışı)             │
│   Pin 8 (VCC) ───► +3.3V                                       │
│                                                                 │
│   I2C Adresi: 0x50 (A2A1A0 = 000)                              │
└─────────────────────────────────────────────────────────────────┘

    +3.3V
     │
    ┌┴┐
    │ │ 4.7kΩ (SDA Pull-up)
    └┬┘
     │
     ├─────────────► PB9 (SDA)
     │
     ├─────────────► 24C256 Pin 5 (SDA)
     │
    ┌┴┐
    │ │ 4.7kΩ (SCL Pull-up)
    └┬┘
     │
     ├─────────────► PB8 (SCL)
     │
     ├─────────────► 24C256 Pin 6 (SCL)
```

---

## 9. Debug UART Bağlantısı

```
┌─────────────────────────────────────────────────────────────────┐
│                   FT232 / ST-Link                               │
│                                                                 │
│   STM32 PA9 (TX) ─────────────────────► FT232 RX               │
│   STM32 PA10 (RX) ────────────────────► FT232 TX               │
│   GND ─────────────────────────────────► GND                   │
│   +3.3V ───────────────────────────────► VCC (Opsiyonel)       │
│                                                                 │
│   Baud Rate: 115200                                            │
│   8N1: 8 Data, No Parity, 1 Stop                               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 10. 1-Wire DS18B20 Bağlantısı

```
┌─────────────────────────────────────────────────────────────────┐
│                   DS18B20 Sıcaklık Sensörü                      │
│                                                                 │
│   Pin 1 (VDD) ─────────────────────────► +3.3V                 │
│   Pin 2 (DATA) ─┬─────► PB12 (GPIO)                            │
│                 │                                              │
│                ┌┴┐                                             │
│                │ │ 4.7kΩ (Pull-up)                             │
│                └┬┘                                             │
│                 │                                              │
│                +3.3V                                           │
│   Pin 3 (GND) ─────────────────────────► GND                   │
│                                                                 │
│   NOT: DATA hattı open-drain yapılandırılmalı.                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 11. LED ve Butonlar

### 11.1 LED Göstergeler
```
    +3.3V
     │
    ┌┴┐
    │ │ 330Ω (LED_PWR)
    └┬┘
     │
     ├─────► LED Anode (Yeşil)
     │     LED Cathode ────► PC13
     │
    ┌┴┐
    │ │ 330Ω (LED_ERR)
    └┬┘
     │
     ├─────► LED Anode (Kırmızı)
           LED Cathode ────► PC14
```

### 11.2 Kullanıcı Butonları (Opsiyonel)
```
    Buton1 ────┬─────► PC0
               │
              ┌┴┐
              │ │ 10kΩ (Pull-down)
              └┬┘
               │
              GND

    Buton2 ────┬─────► PC1
               │
              ┌┴┐
              │ │ 10kΩ
              └┬┘
               │
              GND

    NOT: GPIO Input mode, Internal Pull-up aktif.
```

---

## 12. Buzzer (Opsiyonel)

```
    STM32 PA15 (TIM2_CH1)
         │
        ┌┴┐
        │ │ 1kΩ
        └┬┘
         │
         ├─── Base Q1 (2N3904)
         │
    +5V ─┤
         │
    Buzzer (+) ─────┤
    Buzzer (-) ─────┴───► Collector Q1
                         │
    Emitter Q1 ──────────┴───► GND

    Flyback Diyot: 1N4148 buzzer'a ters paralel
```

---

## 13. PCB Layout Önerileri

### 13.1 Güç Dağılımı
- Ana güç girişini merkezi bir noktaya yerleştirin
- LDO'ları MCU'ya yakın tutun
- Her IC için VCC-GND arasına 100nF decoupling capacitor ekleyin
- Yüksek akım çeken vanalar için geniş trace kullanın

### 13.2 Analog Bölüm
- pH ve EC sensör girişlerini dijital sinyallerden uzak tutun
- Analog GND ve dijital GND'yi tek noktada birleştirin
- ADC referansını temiz tutun (LC filtre)

### 13.3 FSMC/Hızlı Sinyaller
- FSMC trace'lerini eşit uzunlukta tutun
- 90° köşelerden kaçının (45° veya curve kullanın)
- Ground plane kullanın

### 13.4 EMI/EMC
- Crystal osilatörü MCU'ya yakın yerleştirin
- Reset pinine 100nF capacitor ekleyin
- Tüm dış bağlantılara ESD koruması ekleyin

---

## 14. Malzeme Listesi (BOM)

| No | Açıklama | Miktar | Referans |
|----|----------|--------|----------|
| 1 | STM32F407VET6 LQFP144 | 1 | U1 |
| 2 | AMS1117-3.3 | 1 | U2 |
| 3 | AMS1117-5.0 | 1 | U3 |
| 4 | ULN2803 | 1 | U4 |
| 5 | 24C256 EEPROM | 1 | U5 |
| 6 | MCP6002 Op-Amp | 1 | U6 |
| 7 | 2N3904 NPN Transistor | 2 | Q1, Q2 |
| 8 | 1N4007 Diyot | 1 | D1 |
| 9 | 1N4148 Diyot | 2 | D2, D3 |
| 10 | LED Yeşil 3mm | 1 | LED1 |
| 11 | LED Kırmızı 3mm | 1 | LED2 |
| 12 | 10µF Elektrolitik Kondansatör | 6 | C1-C6 |
| 13 | 100nF Seramik Kondansatör | 10 | C7-C16 |
| 14 | 1kΩ 0805 Direnç | 10 | R1-R10 |
| 15 | 4.7kΩ 0805 Direnç | 3 | R11-R13 |
| 16 | 10kΩ 0805 Direnç | 4 | R14-R17 |
| 17 | 330Ω 0805 Direnç | 2 | R18-R19 |
| 18 | 2A Sigorta | 1 | F1 |
| 19 | 12V DC Jack | 1 | J1 |
| 20 | 8x Terminal Blok 5mm | 2 | TB1, TB2 |
| 21 | 40-pin LCD Connector | 1 | LCD_CONN |
| 22 | BNC Connector (pH) | 1 | BNC1 |
| 23 | BNC Connector (EC) | 1 | BNC2 |
| 24 | 16MHz Kristal | 1 | X1 |
| 25 | 32.768kHz Kristal (RTC) | 1 | X2 |

---

**Sonraki Adım:** STM32CubeMX konfigürasyonu oluşturulacak