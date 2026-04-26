# 🌿 STM32F407VET6 Sulama Sistemi - Sistem Gereksinimleri

## 1. Proje Özeti

STM32F407VET6 mikrodenetleyici ve 3.2" TFT dokunmatik LCD kullanarak pH ve EC kontrollu, 8 parsel vanali, iki musteri segmentine yonelen dinamik ihtiyac odakli otomatik sulama kontrol sistemi.

---

## 2. Donanım Gereksinimleri

### 2.0 Urun Segmentleri

Bu proje iki farkli urun seviyesiyle dusunulmelidir:

| Segment | Hedef Kullanici | Ana Hedef | Urun Yaklasimi |
|---------|------------------|-----------|----------------|
| `Core` | Kucuk ciftciler | En dusuk maliyetle guvenilir ve verimli sulama | Az sensor, sade arayuz, hizli kurulum |
| `Insight` | Kurumsal tarim isletmeleri | Operasyonel optimizasyon, daha fazla veri ve adaptif karar | Fazla sensor, parcelye ozel profil, ileri analiz |

Ortak ilke:

- Tek bir kontrol cekirdegi korunur.
- Donanim ve firmware, ozellik bayraklariyla iki seviyeye ayrilir.
- `Core` surumu, sonradan `Insight` seviyesine yukselebilecek sekilde tasarlanir.

### 2.1 Ana Kontrol Kartı
| Bileşen | Özellik |
|---------|---------|
| Mikrodenetleyici | STM32F407VET6 |
| Flash Bellek | 512 KB |
| SRAM | 192 KB |
| Clock Hızı | 168 MHz |
| ADC | 3x 12-bit ADC |
| Timer | 17 adet (2x advanced, 12x general, 3x basic) |
| İletişim | 3x SPI, 3x I2C, 6x USART |
| GPIO | 140+ pin |

### 2.2 Display Modülü
| Bileşen | Özellik |
|---------|---------|
| Ekran | 3.2" TFT LCD |
| Çözünürlük | 320x240 (QVGA) |
| Arayüz | FSMC 8080-80 Paralel |
| Driver IC | ILI9341 |
| Dokunmatik | XPT2046 Dirençli Touch |
| Renk | 16-bit RGB565 |

### 2.3 Sensörler
| Sensör | Ölçüm Aralığı | Çıkış | Bağlantı |
|--------|---------------|-------|----------|
| pH Sensörü | 0-14 pH | 0-3.3V Analog | ADC1 IN0 (PA0) |
| EC Sensörü | 0-20 mS/cm | 0-3.3V Analog | ADC1 IN1 (PA1) |
| Sıcaklık (opsiyonel) | -40~85°C | DS18B20 | 1-Wire (PB12) |

### 2.3.1 Dinamik Ihtiyac Icin Onerilen Ek Sensorler

| Sensor | Oncelik | Sinyal Tipi | Onerilen Baglanti | Amac |
|--------|---------|-------------|-------------------|------|
| Isik sensoru (`BH1750` / `VEML7700`) | Zorunluya yakin | I2C | I2C1 (`PB8`, `PB9`) | `light bucket` ve gun dogumu referansi |
| Debi sensoru | Zorunluya yakin | Pulse / dijital | `PB13` | Sure yerine litre tabanli bitis |
| Dusuk su seviyesi | Zorunlu | Dijital | `PB14` | Kuru calisma ve emniyet interlock |
| Hat basinc sensoru | Onerilen | 0-3.3V analog | `PC5` | Akis var/yok, filtre tikali, pompa davranisi |
| Drenaj sensoru | Opsiyonel | Dijital | `PB15` | Sulama geri bildirimi ve inhibit |
| Toprak nem sensoru | Opsiyonel | Dijital veya analog | `PC5` veya harici ADC | Parsel bazli inhibit veya alarm |

Notlar:

- Isik sensoru ile EEPROM ayni I2C hatti uzerinden paylasilabilir.
- Debi sensoru ilk fazda pulse sayimi ile, ileride timer capture ile iyilestirilebilir.
- Toprak nemi her parsele takilmayacaksa "ana karar verici" degil, `gate/inhibit` kaynagi olarak kullanilmalidir.

### 2.3.2 Segment Bazli Sensor Paketi

| Sensor | `Core` | `Insight` | Not |
|--------|--------|-----------|-----|
| pH | Var | Var | Ortak cekirdek ozelligi |
| EC | Var | Var | Ortak cekirdek ozelligi |
| RTC | Var | Var | Temel zaman referansi |
| Dusuk su seviyesi | Var | Var | Emniyet icin zorunlu |
| Debi | Opsiyonel / yazilimsal fallback | Var | `Insight` icin guclu tavsiye |
| Isik | Opsiyonel | Var | `light bucket` icin gerekli |
| Hat basinc | Yok veya opsiyonel | Var | Operasyonel tani icin onemli |
| Drenaj | Yok veya opsiyonel | Opsiyonel | Geri bildirim katmani |
| Toprak nem | Opsiyonel | Opsiyonel | Gate/advisory rolunde |

### 2.4 Vana Kontrol
| Özellik | Değer |
|---------|-------|
| Vana Sayısı | 8 adet |
| Vana Tipi | Solenoid Vana (12V DC veya 24V AC) |
| Sürücü | ULN2803 Darlington Array veya Röle Modülü |
| GPIO | PB0-PB7 |
| Maksimum Akım | 500mA/vana |

### 2.5 Güç Kaynağı
| Giriş | Çıkış | Bileşen |
|-------|-------|---------|
| 12V DC | 3.3V | AMS1117-3.3 (MCU için) |
| 12V DC | 5V | AMS1117-5.0 (Display/Sensörler) |
| 12V DC | 12V | Vanalar için direkt |

### 2.6 Yedekleme (Opsiyonel)
| Bileşen | Açıklama |
|---------|----------|
| RTC | STM32 Internal RTC + 32.768kHz kristal |
| Backup Battery | CR2032 (RTC backup) |
| EEPROM | 24C256 (I2C, parametre saklama) |

---

## 3. Yazılım Gereksinimleri

### 3.1 Firmware Mimarisi
```
┌─────────────────────────────────────────────────┐
│              Kullanıcı Arayüzü (GUI)            │
├─────────────────────────────────────────────────┤
│           Sulama Kontrol Algoritması            │
├──────────────┬─────────────────┬────────────────┤
│  pH Kontrol  │   EC Kontrol    │ Dinamik Tetik  │
├──────────────┴─────────────────┴────────────────┤
│              Sensör Sürücü Katmanı              │
├──────────────┬─────────────────┬────────────────┤
│  LCD Driver  │  Touch Driver   │  Vana Driver   │
├──────────────┴─────────────────┴────────────────┤
│              HAL / LL Driver Katmanı            │
└─────────────────────────────────────────────────┘
```

### 3.2 Görev Zamanlaması
| Görev | Periyot | Öncelik |
|-------|---------|---------|
| Touch Okuma | 10 ms | Yüksek |
| Display Refresh | 33 ms (30 FPS) | Orta |
| pH Okuma | 100 ms | Düşük |
| EC Okuma | 100 ms | Düşük |
| Vana Kontrol | 1000 ms | Düşük |
| Sulama Algoritması | 5000 ms | Düşük |

### 3.3 Interrupt Öncelikleri
| Interrupt | Öncelik | Açıklama |
|-----------|---------|----------|
| Touch IRQ | 0 (En yüksek) | Dokunmatik kesme |
| Timer (Systick) | 1 | Sistem zamanlaması |
| ADC Complete | 2 | Sensör okuma tamamlandı |
| USART RX | 3 | Seri iletişim |

---

## 4. Fonksiyonel Gereksinimler

### 4.1 Ana Ekran
- Gerçek zamanlı pH ve EC değer gösterimi
- Aktif vana/parsel gösterimi
- Sulama durumu (çalışıyor/bekliyor/durduruldu)
- Hata/uyarı mesajları

### 4.2 Menü Sistemi
```
ANA EKRAN
├── Ayarlar
│   ├── pH Ayarları
│   │   ├── Hedef pH Değeri
│   │   ├── Min pH Sınırı
│   │   └── Max pH Sınırı
│   ├── EC Ayarları
│   │   ├── Hedef EC Değeri
│   │   ├── Min EC Sınırı
│   │   └── Max EC Sınırı
│   ├── Sulama Süresi
│   │   ├── Parsel 1-8 Süre Ayarı
│   │   └── Bekleme Süresi
│   └── Sistem Ayarları
│       ├── Dil Seçimi
│       ├── Parlaklık
│       └── Kalibrasyon
├── Manuel Kontrol
│   └── Vanalar (1-8) Aç/Kapat
├── Geçmiş
│   ├── pH Logları
│   └── EC Logları
└── Sistem Bilgisi
    ├── Firmware Versiyon
    └── Sensör Durumu
```

### 4.3 Sulama Senaryoları

#### Senaryo 1: pH Kontrolü
```
EĞER pH < pH_min İSE:
    - Asit dozaj vanasını aç
    - pH >= pH_min olana kadar bekle
    - Asit vanasını kapat
    - Karıştırma için sirkülasyon pompasını çalıştır (30 sn)
```

#### Senaryo 2: EC Kontrolü
```
EĞER EC < EC_min İSE:
    - Gübre dozaj vanasını aç
    - EC >= EC_min olana kadar bekle
    - Gübre vanasını kapat
    - Karıştırma için sirkülasyon pompasını çalıştır (30 sn)
```

#### Senaryo 3: Parsel Sulama
```
PARSEL SIRA İLE:
    1. Parsel vanasını aç
    2. Pre-flush fazını çalıştır
    3. Dosing fazında pH/EC hedeflerini uygula
    4. Post-flush ile hattı temizle
    5. Bitiş kriteri süre veya hedef hacim ise tamamla
    6. Sonraki parsela geç
    7. Tüm parseller bitince döngüyü tamamla
```

#### Senaryo 4: Dinamik Ihtiyac Tetikleme
```
EĞER trigger_mode = PERIODIC İSE:
    - start_hhmm + anchor_offset_min referansına göre çalıştır
    - period_min kadar aralıkla tekrar et
    - max_events_per_day sınırına ulaşınca o gün tekrar tetikleme

EĞER trigger_mode = SUNRISE_PERIODIC İSE:
    - İlk fazda 06:00 + anchor_offset_min referansını kullan
    - Astronomik saat veya ışık sensörü eklendiğinde aynı alanları gerçek gün doğumuyla besle
    - period_min ve max_events_per_day sınırlarını uygula

EĞER trigger_mode = VOLUME_TARGET İSE:
    - Parsel için hedef litreye ulaşınca sulamayı bitir

EĞER trigger_mode = LIGHT_BUCKET İSE:
    - Işık şiddetini zamanla çarpıp biriktir
    - Kova eşiği dolunca 1 sulama turu başlat
```

### 4.4 Güvenlik Özellikleri
| Özellik | Açıklama |
|---------|----------|
| Kuru Çalışma Koruması | Su seviyesi sensörü ile pompa koruması |
| Aşırı Akım Koruması | Vana akım monitoringu |
| Sensör Hata Algılama | Geçersiz değer algılama |
| Watchdog Timer | Sistem donma koruması |
| Brownout Detect | Düşük gerilim koruması |
| Akis Dogrulama | Debi veya basinç sensörü ile sulama teyidi |
| Drenaj/Taşma Inhibit | Drenaj sensörü ile yeni tur baslatma engeli |

---

## 5. Kalibrasyon Gereksinimleri

### 5.1 pH Kalibrasyonu
- 2 veya 3 nokta kalibrasyon (pH 4.01, 7.00, 10.01)
- Kalibrasyon tarihi saklama
- Kalibrasyon geçerlilik uyarısı

### 5.2 EC Kalibrasyonu
- 2 nokta kalibrasyon (1413 µS/cm, 12.88 mS/cm)
- Sıcaklık kompensasyonu (25°C referans)

---

## 6. Performans Gereksinimleri

| Parametre | Hedef Değer |
|-----------|-------------|
| Touch Response Time | < 50 ms |
| Display Update Rate | 30 FPS |
| pH Okuma Doğruluğu | ±0.1 pH |
| EC Okuma Doğruluğu | ±2% |
| Vana Açma/Kapama | < 100 ms |
| Sistem Başlatma | < 3 saniye |

---

## 7. Çevresel Gereksinimler

| Parametre | Değer |
|-----------|-------|
| Çalışma Sıcaklığı | 0°C ~ 50°C |
| Saklama Sıcaklığı | -20°C ~ 70°C |
| Nem | %10 ~ %90 (yoğuşmasız) |
| IP Koruma | IP65 (kutu önerisi) |

---

## 8. Dokümantasyon Planı

| Doküman | Açıklama | Durum |
|---------|----------|-------|
| 01_Sistem_Gereksinimleri.md | Bu doküman | ✅ |
| 02_Pin_Assignment.md | STM32 pin mapping | ⏳ |
| 03_Donanim_Semasi.md | Bağlantı şeması | ⏳ |
| 04_Yazilim_Mimarisi.md | Software design | ⏳ |
| 05_Kullanim_Kilavuzu.md | User manual | ⏳ |

---

## 9. Test Planı

### 9.1 Unit Test
- [ ] ADC okuma testi
- [ ] Display init testi
- [ ] Touch kalibrasyon testi
- [ ] Vana kontrol testi

### 9.2 Entegrasyon Testi
- [ ] pH + Display gösterimi
- [ ] EC + Display gösterimi
- [ ] Touch + Menü navigasyonu
- [ ] Vana + Sulama senaryosu

### 9.3 Saha Testi
- [ ] Gerçek pH sensörü testi
- [ ] Gerçek EC sensörü testi
- [ ] Uzun süreli çalışma testi (72 saat)
- [ ] Sıcaklık testi

---

## 10. Versiyon Geçmişi

| Versiyon | Tarih | Açıklama |
|----------|-------|----------|
| 0.1 | 2024-03-15 | İlk taslak |

---

**Sonraki Adım:** Pin Assignment dokümanı oluşturulacak (02_Pin_Assignment.md)
