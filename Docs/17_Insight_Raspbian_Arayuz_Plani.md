# Insight Raspbian Arayuz Plani

Bu not, `Insight` surumunun Raspbian/Raspberry Pi OS tarzi bir Linux edge arayuzu uzerinden ilerlemesi icin mimari karari ve ilk uygulama planini tanimlar.

## 1. Net Karar

`Insight` surumunde ana operator ve servis deneyimi STM32 uzerindeki 3.2" TFT'ye sigdirilmamalidir. STM32 TFT ekrani yerel acil durum, temel durum ve manuel servis icin kalabilir; zengin dashboard, commissioning, rapor, log ve recete yonetimi Raspberry Pi OS benzeri bir Linux katmaninda verilmelidir.

Bu karar ile urun katmanlari su hale gelir:

- `STM32`: emniyetli saha kontrolcusu
- `Linux Edge`: cihaz ustu arayuz, gateway, log, servis API, yerel veri tabani, konum/hava/urun context motoru, sera iklim motoru
- `Cloud`: filo, rapor, uzaktan erisim ve AI servisleri
- `Mobile`: ilk fazda eslikci istemci; ana mimariyi bloke etmez

## 2. Neden Bu Yon?

- `Insight` kullanicisi daha fazla veri, tani, recete ve servis akisi ister.
- 320x240 TFT zengin dashboard, grafik, log ve commissioning icin dar kalir.
- Linux edge katmani internet olmasa bile sahada yerel web arayuzu sunabilir.
- Ek fiziksel sensor zorunlulugu olmadan konum, hava tahmini, tarimsal veri ve urun donemi bilgisiyle karar kalitesi artirilabilir.
- Kamera modulleri daha sonra ayni gateway uzerinden saha gozlemi ve bitki durum kaniti olarak eklenebilir.
- Sera uygulamalarinda SCD41, DS18B20/1-Wire, PCF tabanli role cikislari ve Modbus/RS485 moduller ayni gateway mimarisine iklim katmani olarak baglanabilir.
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
- `gateway/context-engine`: konum, hava/tarimsal veri, urun donemi ve kamera gozlemlerini birlestiren servis
- `gateway/climate-engine`: SCD41, DS18B20, VPD, CO2, fan/sisleme/golgeleme/isitma karar katmani
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
- hava ve tarimsal context: tahmin, ET0, radyasyon, yagis riski, veri yasi
- urun/parsel context: urun tipi, dikim tarihi, donem, hassasiyet ve aktif recete
- kamera gozlemleri: kamera sagligi, parsel goruntusu, bitki/zemin durum siniflari
- tavsiye merkezi: sulama artir/azalt/koru/ertele onerileri ve gerekceleri
- sera iklimi: ic sicaklik, nem, CO2, VPD, dew point, fan/sisleme/golgeleme/isitma durumu
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
- `climate_snapshot`: sicaklik, nem, CO2, VPD, iklim zone durumu
- `climate_command`: izinli fan, sisleme, golgeleme, isitma ve havalandirma komutlari

Kritik kural:

- Gateway sadece niyet ve parametre yollar; nihai emniyet kontrolu STM32 tarafinda kalir.
- Internet, AI veya kamera kaynakli kararlar once `advice` olarak uretilir; otomatik program degisikligi yalnizca limitli, audit log'lu ve feature flag arkasinda acilir.
- Iklim cikislari vana/dozaj cikislarindan ayri ID uzayinda tutulur; isitma, sisleme, fan ve perde komutlari ek interlocklardan gecmeden uygulanmaz.

## 6. Ilk Uygulama Sirasi

1. Firmware'de okunabilir `status snapshot` ve `device_info` komutlarini sabitle.
2. `gateway/` klasorunde basit device-agent baslat.
3. USB CDC uzerinden STM32 status okuma ve reconnect davranisini yap.
4. SQLite olay/telemetry kaydi ekle.
5. Yerel REST API ve websocket stream ac.
6. `gateway/ui` ile canli dashboard prototipi yap.
7. `site_profile` ve `parcel_profile` ile konum/urun/parsel veri modelini ekle.
8. Hava/tarimsal veri provider adapter iskeletini ekle ve ET0/radyasyon/yagis verisini logla.
9. Alarm/event gecmisi ve manuel servis ekranlarini ekle.
10. Recete/profil editorunu firmware veri modeli ile bagla.
11. `irrigation_advice` ve operator onayli context overlay akisini tasarla.
12. Kamera gozlem pipeline'ini feature flag arkasinda ekle.
13. SCD41 ve DS18B20 tabanli iklim izleme adapter'larini ekle.
14. `climate-engine` icin VPD/CO2 ve iklim tavsiye modelini ekle.
15. PCF ve Modbus tabanli iklim cikis genisleme planini prototiple.
16. Kiosk modu ve offline-first davranisi test et.

## 7. Basari Kriteri

Insight edge arayuz MVP'si tamamlandi sayilmasi icin:

- internet olmadan cihaz ekranindan veya ayni yerel agdan dashboard acilmali
- STM32 baglantisi kopunca UI bunu net gostermeli ve reconnect yapmali
- son olaylar gateway diskinde kalici tutulmali
- konum, hava/tarimsal context ve urun/parsel profili internet yokken son gecerli veriyle okunabilmeli
- operator vana/fault gibi kritik durumlari UI'da gorebilmeli
- sulama tavsiyeleri veri yasi, kaynak, guven skoru ve gerekce ile gosterilmeli
- iklim tavsiyeleri sensor sagligi, veri yasi ve interlock durumuyla birlikte gosterilmeli
- manuel komutlar rol/izin ve STM32 emniyet kapisindan gecmeli
- UI kapanirsa veya gateway yeniden baslarsa sulama emniyeti bozulmamali

## 8. Plan Degisikligi

Eski varsayim:

- Insight once daha fazla sensor ve mobil/bulut ozelligi ile genisler.

Yeni varsayim:

- Insight once Linux edge arayuz/gateway ile urunlesir; ek sensor zorunlulugu olmadan internet tarimsal verisi, urun donemi ve kamera gozlemi bu katmanin uzerine eklenir.

Bu, `Insight` icin oncelik siralamasini degistirir:

1. STM32 protokol ve telemetry modeli
2. Linux gateway/device-agent
3. Yerel dashboard ve servis arayuzu
4. Konum, hava/tarimsal veri, urun donemi ve recete context modeli
5. Kamera gozlem pipeline'i ve operator onayli tavsiye motoru
6. SCD41/DS18B20 tabanli sera iklim izleme ve `climate-engine`
7. PCF/Modbus tabanli iklim cikis kontrolu
8. Debi/basinç/log/recete zenginlestirmesi
9. Cloud, mobil ve AI

Detayli context plani:

- [Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md](Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md)
- [Docs/20_Sera_Iklimlendirme_Plani.md](Docs/20_Sera_Iklimlendirme_Plani.md)
