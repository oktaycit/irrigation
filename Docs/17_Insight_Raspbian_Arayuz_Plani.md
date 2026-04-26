# Insight Raspbian Arayuz Plani

Bu not, `Insight` surumunun Raspbian/Raspberry Pi OS tarzi bir Linux edge arayuzu uzerinden ilerlemesi icin mimari karari ve ilk uygulama planini tanimlar.

## 1. Net Karar

`Insight` surumunde ana operator ve servis deneyimi STM32 uzerindeki 3.2" TFT'ye sigdirilmamalidir. STM32 TFT ekrani yerel acil durum, temel durum ve manuel servis icin kalabilir; zengin dashboard, commissioning, rapor, log ve recete yonetimi Raspberry Pi OS benzeri bir Linux katmaninda verilmelidir.

Bu karar ile urun katmanlari su hale gelir:

- `STM32`: emniyetli saha kontrolcusu
- `Linux Edge`: cihaz ustu arayuz, gateway, log, servis API, yerel veri tabani
- `Cloud`: filo, rapor, uzaktan erisim ve AI servisleri
- `Mobile`: ilk fazda eslikci istemci; ana mimariyi bloke etmez

## 2. Neden Bu Yon?

- `Insight` kullanicisi daha fazla veri, tani, recete ve servis akisi ister.
- 320x240 TFT zengin dashboard, grafik, log ve commissioning icin dar kalir.
- Linux edge katmani internet olmasa bile sahada yerel web arayuzu sunabilir.
- STM32 kontrol emniyeti korunur; gateway veya UI takilsa bile vana/fault kararini STM32 verir.
- Raporlama, AI, update ve mobil entegrasyon sonradan daha temiz eklenir.

## 3. Ilk Insight Mimari Paketi

### Donanim

- Raspberry Pi 4, Compute Module 4 veya endustriyel muadili
- Yerel dokunmatik ekran veya HDMI ekran opsiyonu
- STM32 ile `USB CDC` ilk prototip baglantisi
- Saha urunu icin `UART/RS485` opsiyonu acik tutulur
- Ethernet zorunluya yakin, Wi-Fi opsiyonel, 4G premium opsiyon

### Yazilim

- Raspberry Pi OS benzeri Linux imaji
- `gateway/device-agent`: STM32 protokolunu okuyan servis
- `gateway/api`: yerel REST API ve websocket olay akisi
- `gateway/store`: SQLite tabanli yerel olay, alarm ve telemetry kaydi
- `gateway/ui`: browser tabanli yerel web arayuzu
- kiosk modu: cihaz ekrani varsa Chromium tam ekran acilir

## 4. UI Kapsami

Insight v1 edge arayuzu su ekranlari icermelidir:

- canli dashboard: aktif parsel, faz, kalan sure, pH, EC, debi, basinç, alarm
- parsel kartlari: durum, son sulama, hedef hacim, recete, beklenen/olculen akis
- alarm ve olay gecmisi
- manuel servis: vana testleri, dozaj kanal testi, pompa/interlock durumu
- commissioning: sensor kalibrasyonu, K-faktor, kanal kapasitesi, dozlama orani
- recete/profil yonetimi
- cihaz sagligi: STM32 baglantisi, firmware surumu, gateway servisleri, disk/ag durumu

STM32 TFT'de kalmasi gerekenler:

- ana durum
- alarm/fault metni
- acil stop veya guvenli durdurma
- temel manuel vana testi
- gateway yokken minimal kullanim

## 5. STM32 <-> Linux Edge Protokol Ihtiyaci

Gateway'in saglikli calismasi icin firmware tarafinda su veri paketleri standartlasmalidir:

- `device_info`: cihaz kimligi, firmware surumu, donanim revizyonu
- `telemetry_snapshot`: state, faz, parsel, pH, EC, debi, basinç, kalan sure
- `fault_event`: fault kodu, severity, timestamp, latch/ack durumu
- `runtime_event`: program basladi, faz degisti, parsel bitti, dozaj basladi/bitti
- `config_get` / `config_set`: guvenli ayar okuma/yazma
- `manual_command`: izinli manuel aksiyonlar
- `program_sync`: program ve recete aktarimi

Kritik kural:

- Gateway sadece niyet ve parametre yollar; nihai emniyet kontrolu STM32 tarafinda kalir.

## 6. Ilk Uygulama Sirasi

1. Firmware'de okunabilir `status snapshot` ve `device_info` komutlarini sabitle.
2. `gateway/` klasorunde basit device-agent baslat.
3. USB CDC uzerinden STM32 status okuma ve reconnect davranisini yap.
4. SQLite olay/telemetry kaydi ekle.
5. Yerel REST API ve websocket stream ac.
6. `gateway/ui` ile canli dashboard prototipi yap.
7. Alarm/event gecmisi ve manuel servis ekranlarini ekle.
8. Recete/profil editorunu firmware veri modeli ile bagla.
9. Kiosk modu ve offline-first davranisi test et.

## 7. Basari Kriteri

Insight edge arayuz MVP'si tamamlandi sayilmasi icin:

- internet olmadan cihaz ekranindan veya ayni yerel agdan dashboard acilmali
- STM32 baglantisi kopunca UI bunu net gostermeli ve reconnect yapmali
- son olaylar gateway diskinde kalici tutulmali
- operator vana/fault gibi kritik durumlari UI'da gorebilmeli
- manuel komutlar rol/izin ve STM32 emniyet kapisindan gecmeli
- UI kapanirsa veya gateway yeniden baslarsa sulama emniyeti bozulmamali

## 8. Plan Degisikligi

Eski varsayim:

- Insight once daha fazla sensor ve mobil/bulut ozelligi ile genisler.

Yeni varsayim:

- Insight once Linux edge arayuz/gateway ile urunlesir; sensor, mobil, bulut ve AI bu katmanin uzerine eklenir.

Bu, `Insight` icin oncelik siralamasini degistirir:

1. STM32 protokol ve telemetry modeli
2. Linux gateway/device-agent
3. Yerel dashboard ve servis arayuzu
4. Debi/basinç/log/recete zenginlestirmesi
5. Cloud, mobil ve AI
