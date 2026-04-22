# STM32F407VET6 Kart Şeması Özeti

Bu doküman, doğrudan [original-schematic-STM32F407VET6-STM32_F4VE_V2.0.pdf](/Users/oktaycit/Projeler/irrigation/original-schematic-STM32F407VET6-STM32_F4VE_V2.0.pdf)
üzerinden çıkarılmış özet bilgileri içerir.

Önemli not:
- Bu PDF, tam sulama kartının değil, STM32F407VET6 taban kartının şeması gibi görünüyor.
- Daha önce bu dosyada yer alan `12V giriş`, `ULN2803 vana sürücü`, `24C256 EEPROM`,
  `pH/EC analog front-end` gibi bölümler bu PDF tarafından doğrulanmıyor.
- Bu nedenle bu dosya, varsayımsal sistem şeması yerine gerçek PDF ile uyumlu bir kart özeti
  olarak yeniden düzenlendi.

---

## 1. Kapsam

Şemada doğrulanabilen ana bloklar:
- STM32F407VET6 MCU
- 5V giris ve 3.3V regülasyon
- USB slave portu
- UART ISP/programlama başlığı
- JTAG/SWD hata ayıklama başlığı
- TFT LCD başlığı
- microSD kart başlığı
- SPI flash
- NRF24L01 başlığı
- LED ve kullanıcı butonları
- RTC pil bağlantısı
- Genişletme IO başlıkları

Şemada açıkça görünmeyen veya bu PDF ile doğrulanmayan bloklar:
- 12V güç girişi
- vana sürücü katı
- sulama röleleri
- 24C256 EEPROM
- pH/EC sensör koşullandırma devresi

---

## 2. Ana Sonuçlar

| Konu | Şemadan görülen durum | Yorum |
|------|------------------------|-------|
| Ana besleme | `5V` giriş mevcut | Kart 5V tabanlı görünüyor |
| 3.3V üretimi | `AMS1117-3.3` ile yapılıyor | MCU ve 3.3V çevreleri bu hattan besleniyor |
| Native USB | `J4 - STM32-USB Slave` var | `PA11/USB_DM` ve `PA12/USB_DP` doğrudan MCU'ya gidiyor |
| UART boot/programlama | `J6 - ISP_COM` var | `PA9/TXD1`, `PA10/RXD1` hatları kullanılıyor |
| SWD/JTAG | Ayrı başlık var | Programlama ve debug için uygun |
| LCD | FSMC tabanlı TFT başlığı var | Harici ekran bağlantısı için hazırlanmış |

---

## 3. Güç Yapısı

### 3.1 Doğrulanan yapı

Şemanın sol üst bölümünde:
- `5V` giriş hattı
- `U1 = AMS1117-3.3V`
- giriş/çıkış kapasitörleri

Bu yapı, kartın doğrudan `5V` ile beslendiğini ve bundan `3.3V` üretildiğini gösteriyor.

### 3.2 Çıkarım

Bu PDF'ye göre:
- Kart için güvenli ana varsayım `5V giriş`tir.
- `12V DC jack` varsayımı bu şema ile uyuşmuyor.
- Native USB testlerinde, kartın ayrıca 5V ile beslenmesi gerekebilir.

---

## 4. USB Bağlantıları

### 4.1 Native USB

Şemada `J4` başlığı `STM32-USB Slave` olarak işaretlenmiş.

Bu blokta görülen hatlar:
- `PA11 / USB_DM`
- `PA12 / USB_DP`
- `VDD`
- `GND`

Bu, kartın STM32'nin kendi USB FS çevrebirimi ile haberleşebileceğini doğruluyor.

### 4.2 Proje açısından anlamı

Bu bilgi, firmware içinde eklediğimiz USB CDC yaklaşımını destekliyor:
- native USB için doğru pinler mevcut
- USB konfigürasyon portu teorik değil, donanımda karşılığı var

### 4.3 Dikkat

Şemada USB hattı görünse de şu konular fiziksel kart üzerinde ayrıca doğrulanmalı:
- konektör gerçekten dış dünyaya bağlı mı
- VBUS bağlantısı nasıl uygulanmış
- kart yalnız USB ile mi yoksa ayrıca 5V ile mi çalışıyor

---

## 5. Programlama ve Debug Başlıkları

### 5.1 UART ISP Başlığı

`J6 = ISP_COM`

Görünen hatlar:
- `PA10 / RXD1`
- `PA9 / TXD1`
- `5V`
- `GND`

Bu başlık UART tabanlı yükleme veya harici seri haberleşme için ayrılmış.

### 5.2 JTAG/SWD

Ayrı bir `JTAG/SWD` başlığı mevcut.

Görünen temel sinyaller:
- `PA13 / JTMS`
- `PA14 / JTCK`
- `PA15 / JTDI`
- `PB3 / JTDO`
- `NRST`
- `3V3`
- `GND`

Bu başlık debug prob ile firmware yükleme ve hata ayıklama için uygun.

---

## 6. Ekran ve Depolama Bağlantıları

### 6.1 TFT LCD

Şemada bir TFT/LCD başlığı bulunuyor.

Görülen ana sinyaller:
- FSMC veri hatları
- FSMC kontrol hatları
- dokunmatik için ek SPI benzeri sinyaller
- `LCD_BL`
- `LCD_RST`

Bu, harici paralel TFT ekran kullanımını destekliyor.

### 6.2 microSD

microSD kart başlığı mevcut.

Görülen sinyaller:
- `SDIO_D0..D3`
- `SDIO_CMD`
- `SDIO_SCK`

### 6.3 SPI Flash

Harici SPI flash bölümü mevcut.

Görülen sinyaller:
- `FLASH_CS`
- `SPI1_MOSI`
- `SPI1_MISO`
- `FLASH_CLK`

---

## 7. Yardımcı Donanımlar

Şemada ayrıca şu bölümler var:
- güç LED'i
- iki kullanıcı LED'i
- kullanıcı butonları
- NRF24L01 başlığı
- RTC yedek pili
- genişletme IO başlıkları

Bu da kartın bir çekirdek geliştirme kartı mantığında tasarlandığını düşündürüyor.

---

## 8. Proje İçin Pratik Çıkarımlar

Bu PDF'ye göre projede şu varsayımlar güvenle kullanılabilir:
- STM32F407VET6 taban kartı mevcut
- native USB FS hattı donanımda bulunuyor
- SWD ile programlama mümkün
- UART başlığı mevcut
- kart 5V tabanlı besleniyor gibi görünüyor

Bu PDF'ye bakarak şu varsayımlar yapılmamalı:
- kartta 12V giriş kesin var
- vana sürücü katı bu kart üzerinde
- EEPROM bu kart üzerinde hazır
- sensör şartlandırma devresi bu kartta bulunuyor

---

## 9. Temiz Ayrım

İleride karışıklık olmaması için şu ayrımı kullanmak daha doğru olur:

- `original-schematic-STM32F407VET6-STM32_F4VE_V2.0.pdf`
  MCU taban kartını anlatıyor.
- Sulama sistemine ait ek donanımlar
  vana sürücüleri, sensör girişleri, EEPROM ve saha bağlantıları
  ayrı bir üst seviye sistem şemasıyla dokümante edilmeli.

---

## 10. Sonraki Dokümantasyon Önerisi

Bu repo için en sağlıklı yapı şu olur:
- `03_Donanim_Semasi.md`
  mevcut STM32F407VET6 kartının doğrulanmış özeti
- yeni bir `07_Sulama_Karti_Ust_Seviye_Mimari.md`
  sulama sistemine özgü ek donanım blokları
- yeni bir `08_Baglanti_Dogrulama_Notlari.md`
  gerçek kart üzerinde test edilen pin, güç ve USB bulguları

