# 🌿 STM32F407VET6 Sulama Sistemi

pH ve EC kontrollu, 8 parsel vanali, iki farkli musteri segmentine hizmet eden dinamik ihtiyac odakli otomatik sulama kontrol sistemi.

---

## 📋 İçindekiler

- [Özellikler](#özellikler)
- [Donanım](#donanım)
- [Yazılım Mimarisi](#yazılım-mimarisi)
- [Klasör Yapısı](#klasör-yapısı)
- [Kurulum](#kurulum)
- [Kullanım](#kullanım)
- [Dokümantasyon](#dokümantasyon)
- [AI Model Stratejisi](#ai-model-stratejisi)

---

## ✨ Özellikler

### Ana Özellikler
- ✅ **3.2" TFT Dokunmatik LCD** - 320x240 QVGA, ILI9341 driver
- ✅ **pH Kontrolü** - 0-14 pH ölçüm aralığı, otomatik dozaj
- ✅ **EC Kontrolü** - 0-20 mS/cm ölçüm aralığı, otomatik dozaj
- ✅ **8 Kanal Vana Kontrolü** - Multi-zone, parsel bazlı sulama
- ✅ **Hibrit Sulama Tetikleme** - Basit modda periyot, pro modda isik/sensor odakli karar
- ✅ **3 Fazli Sulama Akisi** - Pre-flush, dosing, post-flush
- ✅ **Iki Urun Seviyesi** - Dusuk maliyetli ciftci surumu ve ileri seviye isletme surumu
- ✅ **Otomatik/Manuel Mod** - Esnek çalışma modları
- ✅ **EEPROM Veri Saklama** - Parametre ve log kayıtları
- ✅ **Kalibrasyon** - Çok noktalı pH/EC kalibrasyonu

### Güvenlik Özellikleri
- 🛡️ Sensör hata algılama
- 🛡️ Vana sıkışma koruması
- 🛡️ Ölü bant (hysteresis) kontrolü
- 🛡️ Watchdog timer
- 🛡️ Acil durum durdurma

---

## 🔌 Donanım

### Ana Bileşenler
| Bileşen | Model/Özellik |
|---------|---------------|
| MCU | STM32F407VET6 (512KB Flash, 192KB RAM) |
| Display | 3.2" TFT LCD (ILI9341, FSMC) |
| Touch | XPT2046 Dirençli Dokunmatik |
| pH Sensörü | 0-14 pH Analog |
| EC Sensörü | 0-20 mS/cm Analog |
| Vana Sürücü | ULN2803 Darlington Array |
| EEPROM | 24C256 (32 KBit) |

### Pin Assignment Özeti
| Fonksiyon | STM32 Pinleri |
|-----------|---------------|
| FSMC LCD | PD0-PD15, PE0-PE15, PD4,PD5,PD7,PD11 |
| Touch SPI | PA5,PA6,PA7,PB10,PC15 |
| pH ADC | PA0 |
| EC ADC | PA1 |
| Vanalar | PB0-PB7 |
| I2C EEPROM | PB8,PB9 |
| Debug UART | PA9,PA10 |

Detaylı pin mapping için: [Docs/02_Pin_Assignment.md](Docs/02_Pin_Assignment.md)

---

## 🏗️ Yazılım Mimarisi

```
┌─────────────────────────────────────────────────┐
│              Kullanıcı Arayüzü (GUI)            │
│         (touch_xpt2046 + gui + lcd)             │
├─────────────────────────────────────────────────┤
│           Sulama Kontrol Algoritması            │
│            (irrigation_control)                 │
├──────────────┬─────────────────┬────────────────┤
│  pH Kontrol  │   EC Kontrol    │  Vana Kontrol  │
│  (sensors)   │  (sensors)      │  (valves)      │
├──────────────┴─────────────────┴────────────────┤
│              Sensör Sürücü Katmanı              │
├──────────────┬─────────────────┬────────────────┤
│  LCD Driver  │  Touch Driver   │  EEPROM        │
│(lcd_ili9341) │(touch_xpt2046)  │  (eeprom)      │
├──────────────┴─────────────────┴────────────────┤
│              HAL / LL Driver Katmanı            │
│              (STM32CubeMX generated)            │
└─────────────────────────────────────────────────┘
```

---

## 📁 Klasör Yapısı

```
irrigation/
├── Docs/                          # Dokümantasyon dosyaları
│   ├── 01_Sistem_Gereksinimleri.md
│   ├── 02_Pin_Assignment.md
│   └── 03_Donanim_Semasi.md
├── Core/
│   ├── Inc/                       # Header dosyaları
│   │   ├── main.h
│   │   ├── system_config.h
│   │   ├── lcd_ili9341.h
│   │   ├── touch_xpt2046.h
│   │   ├── sensors.h
│   │   ├── valves.h
│   │   ├── gui.h
│   │   ├── eeprom.h
│   │   └── irrigation_control.h
│   └── Src/                       # Kaynak dosyalar (oluşturulacak)
│       ├── main.c
│       ├── lcd_ili9341.c
│       ├── touch_xpt2046.c
│       ├── sensors.c
│       ├── valves.c
│       ├── gui.c
│       ├── eeprom.c
│       └── irrigation_control.c
├── Drivers/                       # STM32 HAL/LL drivers (CubeMX)
├── Middlewares/                   # Harici kütüphaneler
└── README.md
```

---

## 🛠️ Kurulum

### 1. STM32CubeMX ile Proje Oluşturma

1. STM32CubeMX uygulamasını açın
2. STM32F407VET6 mikrodenetleyiciyi seçin
3. Aşağıdaki çevre birimlerini yapılandırın:

| Peripheral | Ayarlar |
|------------|---------|
| FSMC | 16-bit data, Bank1 NOR/SRAM |
| SPI1 | Full Duplex, 8-bit, Prescaler=64 |
| ADC1 | 2 channel scan mode (IN0, IN1) |
| I2C1 | 100/400 kHz |
| USART1 | 115200, 8N1 |
| TIM1 | PWM Generation CH1 (Backlight) |
| GPIO | PC15 as EXTI (Touch IRQ) |

4. Clock Configuration: 168 MHz (PLL kullanılarak)
5. Project Manager ile IDE seçin (Keil, IAR, STM32CubeIDE)
6. Generate Code

### 2. Oluşturulan Koda Entegrasyon

STM32CubeMX ile oluşturulan projeye `Core/Inc/` klasöründeki header dosyalarını ekleyin.

### 3. Derleme

IDE'niz ile projeyi derleyin:
```bash
# STM32CubeIDE CLI örneği
stm32cubeide -application /path/to/project -build
```

### 4. Programlama

ST-Link veya J-Link ile MCU'ya flash edin:
```bash
# OpenOCD örneği
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program firmware.elf verify reset exit"
```

---

## 📖 Kullanım

### Ana Ekran
```
┌─────────────────────────────────┐
│  pH: 6.5    EC: 1.8 mS/cm      │
│                                 │
│  Parsel 3 - Sulanıyor          │
│  ████████░░ 80%  02:30 kaldı   │
│                                 │
│  [Ana Menü] [Manuel] [Ayarlar] │
└─────────────────────────────────┘
```

### Menü Yapısı
```
ANA EKRAN
├── Ayarlar
│   ├── pH Hedef/Min/Max
│   ├── EC Hedef/Min/Max
│   └── Parsel Süreleri
├── Manuel Kontrol
│   └── Vana 1-8 Aç/Kapat
├── Kalibrasyon
│   ├── pH Kalibrasyon
│   └── EC Kalibrasyon
└── Sistem
    ├── Log Kayıtları
    └── Cihaz Bilgisi
```

---

## 📚 Dokümantasyon

| Dosya | Açıklama |
|-------|----------|
| [Docs/01_Sistem_Gereksinimleri.md](Docs/01_Sistem_Gereksinimleri.md) | Sistem gereksinimleri ve özellikler |
| [Docs/02_Pin_Assignment.md](Docs/02_Pin_Assignment.md) | STM32 pin mapping tablosu |
| [Docs/03_Donanim_Semasi.md](Docs/03_Donanim_Semasi.md) | Donanım bağlantı şemaları |
| [Docs/06_USB_Programlama_Protokolu.md](Docs/06_USB_Programlama_Protokolu.md) | USB ile program import/export komut seti |
| [Docs/07_USB_Masaustu_Arayuzu.md](Docs/07_USB_Masaustu_Arayuzu.md) | USB uzerinden program okuma/yazma masaustu araci |
| [Docs/08_Saha_Kontrol_MVP_Plani.md](Docs/08_Saha_Kontrol_MVP_Plani.md) | Parsel ve dozaj vanalari icin saha kontrol MVP kapsami ve refactor plani |
| [Docs/09_Dinamik_Ihtiyac_Modeli.md](Docs/09_Dinamik_Ihtiyac_Modeli.md) | Statik saat yerine periyot, hacim ve gunes birikimi tabanli sulama mimarisi |
| [Docs/10_Sensor_Genisleme_Plani.md](Docs/10_Sensor_Genisleme_Plani.md) | Dinamik ihtiyac modelini besleyen yeni sensorler, pin onerileri ve firmware etkileri |
| [Docs/11_Urun_Segmentasyonu.md](Docs/11_Urun_Segmentasyonu.md) | Kucuk ciftci ve kurumsal tarim segmentleri icin iki kademeli urun stratejisi |
| [Docs/12_Urun_Yol_Haritasi.md](Docs/12_Urun_Yol_Haritasi.md) | `Core` ve `Insight` segmentlerini ayirmadan ilerleyen fazli urun yol haritasi |
| [Docs/13_Teknik_Backlog.md](Docs/13_Teknik_Backlog.md) | Yol haritasini `hemen firmware`, `ek donanim ister`, `ticari fark yaratir` backlog'una ceviren teknik liste |
| [Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md](Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md) | Turkiye pazari kapasite varsayimlari ile mevcut urun hedeflerini karsilastiran hedef revizyonu |
| [Docs/16_Core_Kapanis_Checklist.md](Docs/16_Core_Kapanis_Checklist.md) | Core firmware kapanis karari, son temiz build ozeti ve kalan saha kabul maddeleri |
| [Docs/17_Insight_Raspbian_Arayuz_Plani.md](Docs/17_Insight_Raspbian_Arayuz_Plani.md) | Insight surum icin Raspbian/Raspberry Pi OS tarzi Linux edge arayuz ve gateway plani |

---

## 🤖 AI Model Stratejisi

Bu projede farklı geliştirme aşamalarında farklı AI modelleri kullanılmıştır:

| Aşama | AI Model | Kullanım |
|-------|----------|----------|
| Gereksinim Analizi | `qwen3.5-plus` | Sistem gereksinimleri dokümanı |
| Pin Assignment | `qwen3-coder-plus` | STM32 pin mapping |
| Donanım Şeması | `qwen3-coder-plus` | Bağlantı diyagramları |
| LCD Driver | `qwen3-coder-plus` | FSMC + ILI9341 kodu |
| Touch Driver | `qwen3-coder-plus` | XPT2046 SPI kodu |
| Sensör Driver | `qwen3-coder-plus` | ADC okuma, filtreleme |
| Vana Kontrol | `qwen3-coder-plus` | GPIO kontrol, state machine |
| GUI | `qwen3-coder-plus` | Widget sistemi, event handling |
| Sulama Algoritması | `qwen3-max-2026-01-23` | Kompleks kontrol mantığı |
| EEPROM | `qwen3-coder-plus` | I2C veri saklama |

### Model Seçim Kriterleri

- **Kod Üretimi**: `qwen3-coder-plus` - Gömülü sistem C kodu için optimize
- **Kompleks Algoritma**: `qwen3-max-2026-01-23` - Yüksek akıl yürütme gerektiren işler
- **Genel Açıklama**: `qwen3.5-plus` - Hızlı ve etkili dokümantasyon

---

## 🔧 Teknik Detaylar

### Performans Hedefleri
| Parametre | Hedef |
|-----------|-------|
| Touch Response | < 50 ms |
| Display Update | 30 FPS |
| pH Doğruluk | ±0.1 pH |
| EC Doğruluk | ±2% |
| Vana Response | < 100 ms |

### Güç Tüketimi
| Bileşen | Akım |
|---------|------|
| MCU + Display | ~240 mA @ 3.3V |
| Tüm Sistem | ~3.5 A @ 12V (vanalar aktif) |

---

## 📝 Versiyon Geçmişi

| Versiyon | Tarih | Açıklama |
|----------|-------|----------|
| 0.1.0 | 2024-03-15 | İlk taslak - Header dosyaları tamamlandı |

---

## 📄 Lisans

Bu proje eğitim amaçlı geliştirilmiştir.

---

## 🤝 Katkıda Bulunma

1. Projeyi fork edin
2. Feature branch oluşturun (`git checkout -b feature/YeniOzellik`)
3. Değişikliklerinizi commit edin (`git commit -m 'Yeni özellik eklendi'`)
4. Branch'i push edin (`git push origin feature/YeniOzellik`)
5. Pull Request oluşturun

---

## ⚠️ Sorumluluk Reddi

Bu proje yüksek gerilim ve su ile çalışacak bir sistemdir. Güvenlik önlemlerini alın. Profesyonel uygulama için uzman denetimi gereklidir.

---

**Son Güncelleme:** 2024-03-15
