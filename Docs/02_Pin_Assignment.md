# 📌 STM32F407VET6 - Pin Assignment Dokümanı

## 1. Pin Dağılımı Özeti

| Fonksiyon | Pin Sayısı | Pinler |
|-----------|------------|--------|
| FSMC (TFT LCD) | 23 | PD0-PD15, PE0-PE2, PD4, PD5, PD11-PD13 |
| Touch SPI | 4 | PA5-PA7, PB10 |
| ADC (pH, EC) | 2 | PA0, PA1 |
| Vana Kontrol | 8 | PB0-PB7 |
| I2C (EEPROM) | 2 | PB8, PB9 |
| USART (Debug) | 2 | PA9, PA10 |
| 1-Wire (Temp) | 1 | PB12 |
| LED/Buzzer | 2 | PC13, PC14 |
| **TOPLAM** | **44** | |

---

## 2. Detaylı Pin Mapping

### 2.1 FSMC - TFT LCD Bağlantısı

#### FSMC Data Bus (16-bit)
| LCD | STM32F407VET6 | Pin No | Açıklama |
|-----|---------------|--------|----------|
| D0 | PD14 | 83 | FSMC_D0 |
| D1 | PD15 | 84 | FSMC_D1 |
| D2 | PD0 | 79 | FSMC_D2 |
| D3 | PD1 | 80 | FSMC_D3 |
| D4 | PE7 | 98 | FSMC_D4 |
| D5 | PE8 | 99 | FSMC_D5 |
| D6 | PE9 | 100 | FSMC_D6 |
| D7 | PE10 | 101 | FSMC_D7 |
| D8 | PE11 | 102 | FSMC_D8 |
| D9 | PE12 | 103 | FSMC_D9 |
| D10 | PE13 | 104 | FSMC_D10 |
| D11 | PE14 | 105 | FSMC_D11 |
| D12 | PE15 | 106 | FSMC_D12 |
| D13 | PD8 | 85 | FSMC_D13 |
| D14 | PD9 | 86 | FSMC_D14 |
| D15 | PD10 | 87 | FSMC_D15 |

#### FSMC Kontrol Sinyalleri
| LCD | STM32F407VET6 | Pin No | Açıklama |
|-----|---------------|--------|----------|
| CS/CE | PD7 | 88 | FSMC_NE1 - Chip Select |
| RS/A0 | PD11 | 81 | FSMC_A16 - Register Select |
| WR | PD5 | 82 | FSMC_NWE - Write Enable |
| RD | PD4 | 81 | FSMC_NOE - Output Enable |
| RST | PE2 | 96 | GPIO - Reset (yazılımsal) |
| BL | PA8 | 58 | TIM1_CH1 - PWM Backlight |

#### FSMC Alternate Function Ayarı
| Pin | AF | Açıklama |
|-----|----|----------|
| PD0-PD15 | AF12 | FSMC_D0-D15 |
| PE0-PE15 | AF12 | FSMC_D0-D15 |
| PD4 | AF12 | FSMC_NOE |
| PD5 | AF12 | FSMC_NWE |
| PD7 | AF12 | FSMC_NE1 |
| PD11 | AF12 | FSMC_A16 |

---

### 2.2 Dokunmatik Ekran (XPT2046) - SPI

| XPT2046 | STM32F407VET6 | Pin No | Açıklama |
|---------|---------------|--------|----------|
| VCC | 3.3V | - | Güç |
| GND | GND | - | Toprak |
| CS | PB10 | 61 | GPIO - Chip Select |
| DIN | PA7 | 57 | SPI1_MOSI |
| DOUT | PA6 | 56 | SPI1_MISO |
| CLK | PA5 | 55 | SPI1_SCK |
| IRQ | PC15 | 19 | GPIO_EXTI15 - Interrupt |

#### SPI1 Konfigürasyonu
| Parametre | Değer |
|-----------|-------|
| Mode | Full Duplex |
| Data Size | 8-bit |
| Clock Polarity | Low (CPOL=0) |
| Clock Phase | 1st Edge (CPHA=0) |
| NSS | Software |
| Baud Rate | 2.1 MHz (HCLK=168MHz, Prescaler=64) |
| Bit Order | MSB First |

---

### 2.3 Analog Sensörler (pH, EC)

#### ADC1 Konfigürasyonu
| Sensör | STM32 Pin | ADC Channel | Pin No | Açıklama |
|--------|-----------|-------------|--------|----------|
| pH | PA0 | ADC1_IN0 | 10 | Analog Input |
| EC | PA1 | ADC1_IN1 | 11 | Analog Input |

#### ADC Ayarları
| Parametre | Değer |
|-----------|-------|
| Resolution | 12-bit |
| Scan Mode | Enable (2 channel) |
| Continuous Mode | Enable |
| Sampling Time | 144 cycles (yüksek doğruluk için) |
| Clock | ADCCLK = PCLK2/4 = 21 MHz |
| EOC | End of Conversion |
| DMA | Enable (DMA2 Stream0) |

#### ADC Hesaplamalar
```
ADC Değer = (Analog Voltaj / 3.3V) × 4095
pH Değeri = ADC × (14.0 / 4095) × Kalibrasyon_Katsayısı
EC Değeri = ADC × (20.0 / 4095) × Kalibrasyon_Katsayısı
```

---

### 2.4 Vana Kontrol (8 Kanal)

| Vana No | STM32 Pin | Pin No | GPIO Mode | Açıklama |
|---------|-----------|--------|-----------|----------|
| Vana 1 | PB0 | 29 | Output Push-Pull | Parsel 1 |
| Vana 2 | PB1 | 30 | Output Push-Pull | Parsel 2 |
| Vana 3 | PB2 | 31 | Output Push-Pull | Parsel 3 |
| Vana 4 | PB3 | 32 | Output Push-Pull | Parsel 4 |
| Vana 5 | PB4 | 33 | Output Push-Pull | Parsel 5 |
| Vana 6 | PB5 | 34 | Output Push-Pull | Parsel 6 |
| Vana 7 | PB6 | 35 | Output Push-Pull | Parsel 7 |
| Vana 8 | PB7 | 36 | Output Push-Pull | Parsel 8 |

#### Vana Sürücü Devresi
```
STM32 GPIO (3.3V) → 1kΩ Direnç → ULN2803 Input
ULN2803 Output → Vana (+)
Vana (-) → 12V/24V
ULN2803 COM → 12V/24V (Flyback diyot için)
```

#### Alternatif: Röle Modülü
```
STM32 GPIO → 1kΩ → NPN Transistor Base
Transistor Collector → Röle Coil (-)
Transistor Emitter → GND
Röle Coil (+) → 12V
Röle NO → Vana AC/DC (+)
Röle COM → Güç Kaynağı
```

---

### 2.5 I2C - EEPROM (24C256)

| 24C256 | STM32F407VET6 | Pin No | Açıklama |
|--------|---------------|--------|----------|
| VCC | 3.3V | - | Güç |
| GND | GND | - | Toprak |
| SDA | PB9 | 60 | I2C1_SDA |
| SCL | PB8 | 59 | I2C1_SCL |
| WP | GND | - | Write Protect (devre dışı) |
| A0, A1, A2 | GND | - | Adres seçimi (0x50) |

#### I2C1 Konfigürasyonu
| Parametre | Değer |
|-----------|-------|
| Speed Mode | Standard (100 kHz) veya Fast (400 kHz) |
| Own Address | 7-bit |
| General Call | Disable |
| No Stretch Mode | Disable |
| Duty Cycle | 2 (Fast mode için) |

#### Pull-up Dirençleri
- SDA: 4.7kΩ → 3.3V
- SCL: 4.7kΩ → 3.3V

---

### 2.6 USART1 - Debug/PC İletişimi

| Fonksiyon | STM32F407VET6 | Pin No | Açıklama |
|-----------|---------------|--------|----------|
| TX | PA9 | 59 | USART1_TX |
| RX | PA10 | 60 | USART1_RX |

#### USART Ayarları
| Parametre | Değer |
|-----------|-------|
| Baud Rate | 115200 |
| Word Length | 8-bit |
| Stop Bits | 1 |
| Parity | None |
| Hardware Flow Control | None |
| Over Sampling | 16x |

---

### 2.7 1-Wire - DS18B20 Sıcaklık Sensörü

| DS18B20 | STM32F407VET6 | Pin No | Açıklama |
|---------|---------------|--------|----------|
| VDD | 3.3V | - | Güç |
| GND | GND | - | Toprak |
| DATA | PB12 | 63 | GPIO_Output/Open-Drain |
| Pull-up | 4.7kΩ → 3.3V | - | Data hattı |

---

### 2.8 Kullanıcı Arayüzü

#### LED Göstergeler
| LED | Renk | STM32 Pin | Pin No | Açıklama |
|-----|------|-----------|--------|----------|
| LED_PWR | Yeşil | PC13 | 18 | Güç açık |
| LED_ERR | Kırmızı | PC14 | 17 | Hata durumu |

#### Buzzer (Opsiyonel)
| Buzzer | STM32 Pin | Pin No | Açıklama |
|--------|-----------|--------|----------|
| Buzzer | PA15 | 71 | TIM2_CH1 - PWM |

---

## 3. GPIO Özet Tablosu

### Kullanılan GPIO'lar - Port A
| Pin | Fonksiyon | Mode | AF |
|-----|-----------|------|-----|
| PA0 | ADC1_IN0 (pH) | Analog | - |
| PA1 | ADC1_IN1 (EC) | Analog | - |
| PA5 | SPI1_SCK | AF_PP | AF5 |
| PA6 | SPI1_MISO | AF_PP | AF5 |
| PA7 | SPI1_MOSI | AF_PP | AF5 |
| PA8 | TIM1_CH1 (BL PWM) | AF_PP | AF1 |
| PA9 | USART1_TX | AF_PP | AF7 |
| PA10 | USART1_RX | AF_PP | AF7 |
| PA15 | TIM2_CH1 (Buzzer) | AF_PP | AF1 |

### Kullanılan GPIO'lar - Port B
| Pin | Fonksiyon | Mode | AF |
|-----|-----------|------|-----|
| PB0 | Vana1 | Output PP | - |
| PB1 | Vana2 | Output PP | - |
| PB2 | Vana3 | Output PP | - |
| PB3 | Vana4 | Output PP | - |
| PB4 | Vana5 | Output PP | - |
| PB5 | Vana6 | Output PP | - |
| PB6 | Vana7 / I2C1_SCL | Output PP / AF_OD | AF4 |
| PB7 | Vana8 / I2C1_SDA | Output PP / AF_OD | AF4 |
| PB8 | I2C1_SCL | AF_OD | AF4 |
| PB9 | I2C1_SDA | AF_OD | AF4 |
| PB10 | Touch CS | Output PP | - |
| PB12 | 1-Wire DATA | Output OD | - |

⚠️ **NOT:** PB6/PB7 hem vana hem I2C için kullanılamaz. I2C için PB8/PB9 kullanılacak, vanalar PB0-PB7 olarak kalacak.

### Kullanılan GPIO'lar - Port C
| Pin | Fonksiyon | Mode | AF |
|-----|-----------|------|-----|
| PC13 | LED_PWR | Output PP | - |
| PC14 | LED_ERR | Output PP | - |
| PC15 | Touch IRQ | Input PU | - |

### Kullanılan GPIO'lar - Port D (FSMC)
| Pin | Fonksiyon | Mode | AF |
|-----|-----------|------|-----|
| PD0-PD15 | FSMC Data/Control | AF_PP | AF12 |

### Kullanılan GPIO'lar - Port E (FSMC)
| Pin | Fonksiyon | Mode | AF |
|-----|-----------|------|-----|
| PE0-PE15 | FSMC Data | AF_PP | AF12 |
| PE2 | LCD Reset | Output PP | - |

---

## 4. Timer Kullanımı

| Timer | Kanal | Fonksiyon | Açıklama |
|-------|-------|-----------|----------|
| TIM1 | CH1 | LCD Backlight PWM | 16-bit, 42MHz |
| TIM2 | CH1 | Buzzer PWM | 32-bit, 42MHz |
| TIM3 | - | Sistem Zamanlaması | 1ms tick |
| TIM4 | - | Yedek | - |

---

## 5. DMA Kanal Ataması

| Kanal | Stream | Fonksiyon | Yön |
|-------|--------|-----------|-----|
| DMA2 Stream0 | 0 | ADC1 | Peripheral→Memory |
| DMA2 Stream3 | 3 | SPI1 TX | Memory→Peripheral |
| DMA2 Stream4 | 4 | SPI1 RX | Peripheral→Memory |

---

## 6. Interrupt Öncelikleri (NVIC)

| Interrupt | Öncelik | Grup | Açıklama |
|-----------|---------|------|----------|
| EXTI15_10_IRQn | 0 | 0 | Touch IRQ |
| ADC_IRQn | 1 | 1 | ADC Complete |
| TIM3_IRQn | 2 | 2 | Sistem Timer |
| USART1_IRQn | 3 | 3 | Debug RX |
| I2C1_EV_IRQn | 4 | 3 | EEPROM |

---

## 7. Güç Tüketimi Özeti

| Bileşen | Akım (ortalama) | Gerilim |
|---------|-----------------|---------|
| STM32F407VET6 | 80 mA @ 168MHz | 3.3V |
| TFT LCD | 100 mA | 3.3V |
| Backlight | 50 mA | 3.3V |
| pH Sensörü | 5 mA | 3.3V |
| EC Sensörü | 5 mA | 3.3V |
| Vanalar (aktif) | 400 mA × 8 | 12V |
| **TOPLAM (MCU+Display)** | **~240 mA** | **3.3V** |
| **TOPLAM (tüm sistem)** | **~3.5 A** | **12V** |

---

## 8. Pin Değişiklikleri (Revizyon Geçmişi)

| Revizyon | Tarih | Açıklama |
|----------|-------|----------|
| 0.1 | 2024-03-15 | İlk taslak |

---

**Sonraki Adım:** Donanım bağlantı şeması oluşturulacak (03_Donanim_Semasi.md)
