# Teknik Backlog

Bu dokuman, urun yol haritasini yazilim ve donanim islerine ceviren uygulanabilir backlog listesidir.

Amac:

- `Core` ve `Insight` segmentlerini karistirmadan teknik oncelikleri netlestirmek
- "hemen firmware", "ek donanim ister", "ticari fark yaratir" eksenlerinde karar vermeyi kolaylastirmak
- `TODO.md` icindeki gunluk isleri daha ust seviyeli bir backlog ile hizalamak

## 1. Kullanim Kurali

Bu backlog okunurken su ayrim korunmalidir:

- `TODO.md`: aktif sprint veya yakin vade uygulama listesi
- bu dokuman: orta vade teknik onceliklendirme ve paketleme listesi

## 2. Kategoriler

### A. Hemen Firmware

Ek donanim beklemeden mevcut platformda baslanabilecek isler.

### B. Ek Donanim Ister

Donanim genislemesi, saha baglantisi veya yeni sensor gerektiren isler.

### C. Ticari Fark Yaratir

Musterinin urunu neden sececegini belirginlestiren, satis diline dogrudan yansiyabilecek isler.

## 3. Hemen Firmware

Bu grup once `Core`, sonra `Insight` yolunu acan en hizli islerdir.

| ID | Is | Segment | Oncelik | Gerekce |
|----|----|---------|---------|---------|
| FW-01 | `pre-flush`, `dosing`, `post-flush` akislarini scheduler icinde netlestir | Core + Insight | P0 | Ortak kontrol mantiginin omurgasi |
| FW-02 | `fault manager`, `parcel scheduler`, `dosing controller` sorumluluklarini ayristir | Core + Insight | P0 | Kodun buyumesini guvenli hale getirir |
| FW-03 | Guvenli acilis, guvenli stop, `emergency stop` ve fault latch akislarini tekillestir | Core + Insight | P0 | Saha emniyeti icin kritik |
| FW-04 | EEPROM CRC, versiyonlama ve varsayilan deger fallback akisini tamamla | Core + Insight | P0 | Servis yukunu azaltir |
| FW-05 | TFT ana ekranini gercek durum verisi ve net alarm metinleri ile toparla | Core | P1 | Kullanim kolayligini dogrudan artirir |
| FW-06 | Ilk acilis/kurulum sihirbazi ekle | Core | P1 | Kucuk ciftci segmenti icin en yuksek UX etkisi |
| FW-07 | Basit mod veri modelini tanimla: ankraj + tekrar + gunluk limit | Core | P1 | Saat bagimli karmasayi azaltir |
| FW-08 | Hazir profil mantigi ekle: ornek urun tipleri veya sulama profilleri | Core | P1 | Recete yerine daha kolay kullanim |
| FW-09 | USB masaustu araci ile programlama akislarini sadelelestir ve servis komutlarini artir | Core + Insight | P1 | Sahada hizli devreye alma saglar |
| FW-10 | Program modelini `trigger_mode`, `pre_flush_sec`, `post_flush_sec`, `recipe_profile_id` alanlarina evrilt | Core + Insight | P1 | Gelecek fazlari bloke etmez |
| FW-13 | Kanal bazli kapasite ve dozlama orani modelini ekle: `nominal_capacity_lph`, `1:100`, `1:200`, ozel oran | Core + Insight | P1 | Turkiye pazari icin 50-600 l/sa kanal hedefini firmware'de temsil eder |
| FW-14 | Booster pompa cikisi ve interlock mantigini ekle | Core + Insight | P1 | Dusuk basincli sahalarda Venturi performansini stabil tutar |
| FW-15 | Gateway icin `device_info`, `telemetry_snapshot`, `fault_event`, `runtime_event` paketlerini standartlastir | Insight, premium Core | P0 | Linux edge arayuzun firmware ile temiz konusmasini saglar |
| FW-16 | USB CDC servis protokolunu durum, olay, config ve manuel komut siniflariyla genislet | Insight, premium Core | P0 | Raspbian tarzi gateway MVP'nin ilk baglantisi |
| FW-11 | Alarm gecmisi ve olay gunlugu ekle | Insight | P2 | Tani ve izlenebilirlik getirir |
| FW-12 | Bakim zekasi: kalibrasyon zamani, vana cevrim sayisi, servis hatirlatma | Insight | P2 | Operasyonel deger uretir |

## 4. Ek Donanim Ister

Bu grup `Insight` icin kritik fark yaratir; bazilari `Core` icin opsiyonel premium eklenti olabilir.

| ID | Is | Segment | Oncelik | Gerekce |
|----|----|---------|---------|---------|
| HW-01 | Dusuk su seviyesi girisini saha ile dogrula ve zorunlu emniyet interlock yap | Core + Insight | P0 | En temel emniyet geregi |
| HW-10 | 4+1 Venturi/Z-bypass mekanik mimarisini ticari ilk SKU olarak tasarla | Core + Insight | P0 | Turkiye'de 50-100 dekar segmenti icin en dogru maliyet/kapasite dengesi |
| HW-11 | 110 mm ana hat, statik mikser, flow cell ve servis vanalari icin saha kurulum standardi tanimla | Core + Insight | P0 | Kurulumu standartlastirir ve sensor gecikmesini azaltir |
| HW-02 | Debi sensoru entegrasyonu yap | Insight, opsiyonel Core | P1 | Hacim bazli bitis ve akis tanisi icin temel |
| HW-03 | Hat basinc sensoru girisini ekle | Insight | P1 | Pompa/filtre/hat davranisini anlamayi saglar |
| HW-04 | Isik sensoru (`BH1750` / `VEML7700`) ekle | Insight | P2 | Dinamik ihtiyac tetiklemeyi acar |
| HW-05 | Drenaj geri bildirimi veya inhibit girisi ekle | Insight | P2 | Daha guvenilir sulama dogrulamasi |
| HW-06 | Toprak nemi girisini advisory veya gate roluyle ekle | Insight | P3 | Faydali ama ilk kritik halka degil |
| HW-07 | Debi sensoru icin pulse capture / timer altyapisini guclendir | Insight | P2 | Olcum kalitesini yukselten teknik iyilestirme |
| HW-08 | `STM32` yanina `Raspberry Pi` benzeri Linux edge arayuz/gateway katmani ekle | Insight, premium Core | P0 | Insight'in ana operator ve servis deneyimini tasir |
| HW-09 | Gateway icin `Ethernet` / `Wi-Fi` / opsiyonel `4G` baglanti varyantlarini tanimla | Insight, premium Core | P1 | Saha baglantisini urunlestirir |

## 4.1 Linux Edge / Gateway Isleri

Bu grup, `Insight` surumunu Raspbian/Raspberry Pi OS tarzi arayuz uzerinden urunlestirmek icin yeni omurgadir.

| ID | Is | Segment | Oncelik | Gerekce |
|----|----|---------|---------|---------|
| GW-01 | `gateway/` klasorunde device-agent servis iskeletini baslat | Insight | P0 | STM32 ile Linux edge arasindaki canli kopru |
| GW-02 | USB CDC uzerinden reconnect destekli telemetry okuyucu ekle | Insight | P0 | Ilk saha prototipi icin en hizli baglanti |
| GW-03 | SQLite tabanli event/telemetry store ekle | Insight | P0 | Internet yokken log ve tani kaybolmaz |
| GW-04 | Yerel REST API ve websocket stream ekle | Insight | P0 | Web dashboard ve mobil istemci ayni API'yi kullanir |
| GW-05 | `gateway/ui` altinda canli dashboard prototipi ekle | Insight | P0 | Raspbian arayuz kararinin gorunen urun karsiligi |
| GW-06 | Chromium kiosk modu ve servis autostart paketini tasarla | Insight | P1 | Cihaz ustu ekran deneyimi icin gerekli |
| GW-07 | Rol/izin ve audit log altyapisini ekle | Insight | P1 | Manuel komut ve servis akislari icin emniyet |
| GW-08 | Cloud sync worker icin disk kuyrugu ve MQTT/HTTPS taslagini hazirla | Insight | P2 | Uzak izleme ve filo yonetimine gecis |
| GW-09 | `site_profile` ve `parcel_profile` veri modelini SQLite store'a ekle | Insight | P0 | Konum, urun ve parsel context'i olmadan akilli tavsiye uretilemez |
| GW-10 | Hava/tarimsal veri provider adapter iskeletini ekle | Insight | P0 | Ek sensor zorunlulugu olmadan ET0/radyasyon/yagis context'i saglar |
| GW-11 | `irrigation_advice` ve operator onayli `context_overlay` akisini tasarla | Insight | P1 | Internet verisi programi guvenli sinirlar icinde etkileyebilir |
| GW-12 | Kamera `camera_observation` pipeline taslagini hazirla | Insight | P1 | Kamera modulleri bitki/zemin gozlemini karar gerekcesine baglar |
| GW-13 | `climate-engine` servis veri modelini ekle | Insight sera | P0 | Sera iklimlendirme sulama context'iyle ayni gateway'de yonetilir |
| GW-14 | SCD41 I2C ve DS18B20/1-Wire sensor adapter'larini ekle | Insight sera | P0 | Iklim izleme icin ic sicaklik/nem/CO2 ve yardimci sicaklik verisi gerekir |
| GW-15 | PCF8574/PCF8575 tabanli pano ici role/input genisleme modelini ekle | Insight sera | P1 | Fan, sisleme, golgeleme ve isitma role cikislari icin dusuk pin maliyeti |
| GW-16 | Modbus/RS485 iklim sensor/role modul adresleme modelini tasarla | Insight sera | P1 | Uzun kablo ve dagitik sera panolari icin daha dayanikli yol |
| GW-17 | RS485/Modbus ultrasonik ruzgar hiz/yon sensoru adapter'ini ekle | Insight sera | P1 | Havalandirma, perde, sisleme ve firtina emniyeti icin dis ortam ruzgar verisi gerekir |

## 5. Ticari Fark Yaratan Isler

Bu grup, teknik olarak da degerli ama asil etkisi urunun konumlanmasinda gorulen islerdir.

| ID | Is | Segment | Oncelik | Ticari Etki |
|----|----|---------|---------|-------------|
| CV-01 | Kurulum sihirbazi | Core | P1 | "Kolay kurulum" vaadini gercek yapar |
| CV-02 | Hazir profil ve sade arayuz | Core | P1 | Teknik olmayan kullaniciya hitap eder |
| CV-03 | Learned-flow benzeri beklenen akis modeli | Insight | P1 | "Sadece ac-kapa degil, dogrulayan cihaz" algisi yaratir |
| CV-04 | Hacim bazli sulama bitisi | Insight, opsiyonel Core | P1 | Verimlilik dili icin guclu arguman |
| CV-11 | Turkiye pazari icin 50-100 dekar / 10-20 dekar sera hedef paketini netlestir | Core | P1 | Satis mesajini ve mekanik konfigurasyonu somutlastirir |
| CV-05 | Parsel bazli recete secimi | Insight | P1 | Profesyonel kullanimda dogrudan fark yaratir |
| CV-06 | Alarm gecmisi ve servis ekranlari | Insight | P2 | Teknik servis ve isletme yonetimi icin degerli |
| CV-07 | Bakim zekasi | Insight | P2 | Ariza olmadan once mudahale avantaji |
| CV-08 | Uzak izleme ve alarm bildirimi | Insight, premium Core | P2 | Ust segment satis dili icin onemli |
| CV-09 | Linux edge operator ve servis dashboardu | Insight | P0 | Insight'i ayirt eden ana arayuz deneyimi |
| CV-12 | Mobil operator ve servis uygulamasi | Insight, premium Core | P2 | Linux edge API olgunlastiktan sonra saha erisimini genisletir |
| CV-10 | AI destekli alarm ozetleme ve operasyon tavsiyeleri | Insight | P2 | "Akilli operasyon" deger onermesini guclendirir |
| CV-13 | Konum, hava, urun donemi ve kamera destekli sulama asistani | Insight | P1 | Ek sensor zorunlulugu olmadan fark yaratan akilli sulama vaadi |
| CV-14 | Sera iklim asistani ve iklim kontrol paketi | Insight sera | P1 | Sulama + fertigation + iklimlendirme tek platform degerini buyutur |

## 6. Sonraki Sprint Onerisi

Bu repo icin en mantikli kisa-orta vade siralama:

### Sprint 1

- FW-01 `pre-flush`, `dosing`, `post-flush`
- FW-02 moduller arasi sorumluluk ayrimi
- FW-03 guvenli fault ve stop akisi
- FW-04 EEPROM CRC ve fallback

Beklenen sonuc:

- Cekirdek saha davranisi guvenilir hale gelir.

### Sprint 2

- FW-05 sade TFT durum ve alarm ekranlari
- FW-06 kurulum sihirbazi
- FW-07 basit mod veri modeli
- FW-08 hazir profil mantigi
- FW-09 USB servis/programlama iyilestirmesi
- FW-13 kanal kapasitesi ve dozlama orani modelinin veri yapisini tanimla

Beklenen sonuc:

- `Core` segmenti icin ilk ticari kullanim akisi olgunlasir.

### Sprint 3

- HW-10 4+1 Venturi/Z-bypass mekanik hedefini prototiple
- HW-11 110 mm ana hat / flow cell / statik mikser kurulum standardini dokumante et
- FW-15 gateway telemetry/event paketlerini standartlastir
- FW-16 USB CDC servis protokolunu gateway icin genislet
- GW-01 device-agent iskeletini baslat
- GW-02 USB CDC telemetry okuyucu ekle
- GW-03 SQLite event/telemetry store ekle
- GW-09 site/parsel context veri modelini ekle
- GW-13 climate-engine veri modelini ekle

Beklenen sonuc:

- `Insight` Linux edge arayuzunun firmware ile konusabilecegi, konum/parsel context'i ve sera iklim veri modelini tutabilecegi ilk omurga acilir.

### Sprint 4

- GW-04 yerel REST API ve websocket stream
- GW-05 canli dashboard prototipi
- GW-06 kiosk modu ve autostart tasarimi
- HW-02 debi sensoru entegrasyonu tasarimi
- GW-10 hava/tarimsal veri provider adapter iskeleti
- GW-11 operator onayli `irrigation_advice` ve `context_overlay`
- GW-14 SCD41 ve DS18B20 sensor adapter iskeleti
- FW-10 program modelinin evrilmesi
- CV-03 learned-flow mantigi
- CV-04 hacim bazli bitis
- CV-13 konum/hava/urun donemi destekli sulama asistani
- CV-14 sera iklim asistani

Beklenen sonuc:

- `Insight` sahada Linux edge dashboard ile gorunur hale gelir; sensor zenginlestirme, internet context'i ve sera iklim izleme ayni arayuze baglanir.

### Sprint 5

- HW-09 saha baglanti varyantlari
- GW-07 rol/izin ve audit log
- GW-08 cloud sync taslagi
- CV-12 mobil uygulama MVP
- CV-10 AI advisory katmani icin veri modeli
- GW-12 kamera observation pipeline taslagi
- GW-15 PCF tabanli role/input genisleme
- GW-16 Modbus/RS485 iklim modul adresleme
- GW-17 ruzgar hiz/yon sensor adapter'i

Beklenen sonuc:

- Cihaz, internet, kamera, iklim sensorleri, iklim cikislari ve mobil ile baglanan ama saha emniyetini koruyan bir operasyon platformuna donusmeye baslar.

## 7. Teknik Bagimlilik Zinciri

Bazi backlog kalemleri birbirine baglidir:

- `pre-flush/post-flush` olmadan hacim ve recete modeli eksik kalir
- debi sensoru olmadan gercek hacim bitisi eksik kalir
- telemetry/event protokolu sabitlenmeden Linux edge arayuz saglamlasamaz
- gateway store/API olmadan mobil, bulut ve AI katmanlari temiz acilamaz
- site/parsel context olmadan hava, urun donemi ve kamera gozlemleri sulama kararina guvenli baglanamaz
- climate-engine olmadan fan/sisleme/golgeleme/isitma cikislari sulama/fertigation emniyetinden temiz ayrilamaz
- kurulum sihirbazi olmadan `Core` segmentinde kullanim kolayligi iddiasi zayif kalir

Bu nedenle teknik sira su omurga ile korunmalidir:

1. Guvenli kontrol
2. Sade kullanim
3. STM32 telemetry/event protokolu
4. Linux edge gateway ve yerel dashboard
5. Konum, hava/tarimsal veri, urun donemi ve parsel context'i
6. Sera iklim izleme ve climate-engine
7. Akis/dogrulama sensorleri, kamera gozlemleri ve iklim cikislari
8. Recete, tavsiye ve optimizasyon
9. Cloud, mobil ve AI katmani

Baglanti mimarisi icin detay:

- [Docs/14_Baglanti_Mobil_AI_Mimarisi.md](Docs/14_Baglanti_Mobil_AI_Mimarisi.md)

## 8. Repo Icin Net Oncelik

Bugun itibariyla en yuksek getirili ilk 5 is:

1. `FW-15` gateway telemetry/event paketlerini standartlastirmek
2. `FW-16` USB CDC servis protokolunu gateway icin genisletmek
3. `GW-01` device-agent iskeletini baslatmak
4. `GW-09` site/parsel context veri modelini baslatmak
5. `GW-13` climate-engine veri modelini baslatmak

Bu besli, `Core` kontrol cekirdegini korurken `Insight` surumunu Raspbian tarzi arayuz, internet/context ve sera iklim veri katmani uzerinden somut urune cevirir.

Insight edge arayuz detayi:

- [Docs/17_Insight_Raspbian_Arayuz_Plani.md](Docs/17_Insight_Raspbian_Arayuz_Plani.md)
- [Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md](Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md)
- [Docs/20_Sera_Iklimlendirme_Plani.md](Docs/20_Sera_Iklimlendirme_Plani.md)
