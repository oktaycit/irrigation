# Saha Kontrol MVP Plani

Bu dokuman, mobil arayuzden once cihazin saha tarafini guvenilir bir kontrolcu haline getirmek icin izlenecek teknik kapsami tanimlar. Odak noktasi, parsel vanalari ve dozaj vanalarinin emniyetli, test edilebilir ve genisleyebilir sekilde yonetilmesidir.

Not: Cihaz mimarisi tek zone degil, `8 parsel` uzerinden calisan `multi-zone` bir kontrolcudur. Bu nedenle orta vadede scheduler yalnizca saat tabanli degil, `parsel bazli dinamik ihtiyac profili` tasiyacak sekilde evrilmelidir.
Not 2: Bu kontrol cekirdegi, dusuk maliyetli `Core` urun ve sensorce zengin `Insight` urun icin ortak kalmalidir. Fark, scheduler iskeletinde degil; etkin ozelliklerde ve sensor kaynaklarinda olmalidir.

## 1. Hedef

Ilk urun surumunde cihaz su uc soruya sahada net cevap vermelidir:

- Hangi parsel vanasi aktif?
- Hangi dozaj vanasi ne oranda suruluyor?
- Hata oldugunda sistem kendini guvenli duruma alabiliyor mu?

Mobil uygulama, bulut ve ileri seviye recete ekranlari bu asamadan sonra gelecektir. Bu fazda ana hedef, kart ustunde calisan cekirdek kontrol yazilimini saglamlastirmaktir.

## 2. MVP Kapsami

MVP once `Core` segmentini sahaya cikaracak kadar yalın olmali, ancak `Insight` segmentine cikisi kapatmamalidir.

### Dahil

- 8 parsel vanasinin tek tek ac/kapa kontrolu
- 5 dozaj vanasinin ayri ayri kontrolu
- Dozaj vanalari icin PWM, duty ve ramp yonetimi
- Parsel bazli sure, bekleme ve sira mantigi
- Manuel baslat/durdur/duraklat/devam ettir akisi
- pH ve EC bazli temel dozaj tetikleme mantigi
- Hata algilama, alarm uretme ve guvenli kapanis
- Enerji kesintisi sonrasi guvenli yeniden baslama davranisi
- EEPROM uzerinden temel parametre kaliciligi

### Dahil Degil

- Mobil uygulama ve kablosuz haberlesme
- Gelismis recete editoru
- Coklu kullanici ve yetkilendirme
- Bulut loglama ve uzaktan izleme
- Uretim dashboardu ve gorsel raporlama

## 3. Basari Kriterleri

- Sistem acilista tum cikislari kapali ve guvenli durumda baslatir.
- Her parsel vanasi tek komutla dogru fiziksel cikisa yansir.
- Her dozaj vanasi duty/frekans parametreleri ile tutarli surulur.
- Yasakli kombinasyonlar yazilim tarafinda engellenir.
- Bir program en az 1 tam parsel dongusunu hatasiz tamamlar.
- Sensor veya vana kaynakli kritik hata durumunda tum cikislar kapanir.
- Yeniden baslatma sonrasi sistem kontrolsuz sekilde vana acmaz.

## 4. Fonksiyonel MVP Modulleri

### 4.1 Valve Layer

Bu katman donanim detayini saklar. Ust katmanlar PCF8574, GPIO veya pin bilgisi bilmez.

Temel sorumluluklar:

- `open(valve_id)`
- `close(valve_id)`
- `close_all()`
- `set_dosing_duty(valve_id, duty_percent)`
- `configure_dosing_channel(valve_id, freq, min_duty, max_duty, ramp_step)`
- `get_state(valve_id)`
- `get_fault(valve_id)`

MVP kurallari:

- Parsel vanalari ile dozaj vanalari tek ID uzayinda kalabilir.
- Dozaj tarafinda `0%` her zaman kapali kabul edilir.
- Duty degisimi ani degil, ramp ile uygulanir.
- `EmergencyStop` her katmanda `close_all()` ile sonuclanir.

### 4.2 Parcel Scheduler

Bu katman siralamayi ve zamanlamayi yonetir.

Temel sorumluluklar:

- Aktif parsel listesini olusturmak
- Mevcut parseli baslatmak
- Sure bitince kapatmak
- Bekleme suresi varsa uygulamak
- Sonraki parsele gecmek
- Program tamamlaninca sistemi `IDLE` durumuna almak

Orta vade genisleme hedefi:

- Statik saat yerine `trigger_mode` tabanli calisma
- `pre-flush`, `dosing`, `post-flush` fazlarini tek akis icinde yonetmek
- Sureye ek olarak hedef hacimle bitis kriteri desteklemek
- LDR veya astronomik saat kaynakli `light bucket` tetikleme eklemek

### 4.3 Dosing Controller

Bu katman pH ve EC icin dozaj adimlarini yonetir.

Temel sorumluluklar:

- Sensor olcumunden hedefe gore doz ihtiyacini hesaplamak
- Uygun dozaj vanasini secmek
- Dozaj suresini, duty degerini ve tepki kazancini belirlemek
- Karistirma ve sensorde etkinin gorunme gecikmesini parametrik yonetmek
- Yeniden olcup gerekiyorsa ikinci duzeltme turunu baslatmak
- Parsel gecislerinde recete degisimi yapmak

MVP kural seti:

- pH ve EC dozaji ayni anda baslamaz.
- Ayni anda birden fazla dozaj vanasi acilsa bile toplam duty limiti yazilim tarafinda sinirlanir.
- Maksimum tek doz suresi asilirsa hata uretilir.
- Sensor geri besleme gecikmesi pH ve EC icin ayri `feedback_delay_ms` parametresiyle ayarlanir.
- Agresif veya yumusak duzeltme `response_gain_percent` ile sahaya gore kalibre edilir.
- pH ve EC duty karari fuzzy rule table ile verilir; hata buyuklugu ve onceki olcume gore hata trendi birlikte kullanilir.
- EC hedefe yaklasirken daha muhafazakar duty carpani kullanilir, cunku EC overshoot ancak seyreltme veya temiz suyla geri alinabilir.
- `Settings > Params` ekranindan `FUZZY` ve eski `LINEAR` duty mantigi arasinda gecis yapilabilir.
- Bu ileri dozaj parametreleri cihazdaki `Settings > Params` ekranindan degistirilir ve EEPROM sistem kaydinda saklanir.
- Ardisik basarisiz doz denemeleri fault sayacina islenir.

### 4.4 Fault Manager

Bu katman hata kodlarini toplar, alarm metni uretir ve guvenli tepkiyi merkezilestirir.

Temel sorumluluklar:

- Sensor timeout
- Sensor out-of-range
- Vana cevap vermiyor suphe durumu
- Doz asimi
- Su seviyesi dusuk
- Asiri akim veya surucu hatasi
- EEPROM/CRC uyumsuzlugu

Cikislar:

- Makine dostu `error_code`
- Kullaniciya uygun kisa alarm metni
- `warning`, `fault`, `latch_fault` seviye ayrimi

## 5. Durum Makinesi Taslagi

Bu fazda iki seviyeli durum makinesi onerilir: ustte calisma akisi, altta dozaj alt akisi.

### 5.1 Ust Durum Makinesi

`BOOT`

- Donanim ve parametreler yuklenir
- Tum cikislar kapatilir
- Sistem `SAFE_IDLE` durumuna gecer

`SAFE_IDLE`

- Calisma yok
- Manuel komut veya program baslatma beklenir

`PRECHECK`

- Sensor gecerliligi
- Su seviyesi
- Aktif fault var mi
- Program ve parsel listesi gecerli mi

`PARCEL_START`

- Secilen parsel vanasi acilir
- `PRE_FLUSH` alt fazi baslar

`PARCEL_ACTIVE`

- `PRE_FLUSH` tamamlanir, ardindan `DOSING` fazina gecilir
- Parsel suresi veya hedef hacim takibi devam eder
- Gerekirse periyodik dozaj kontrol penceresi acilir
- Durdur veya fault gelirse cikis yapilir

`POST_FLUSH`

- Dozaj vanalari kapanir
- Hat temizligi icin yalniz su akisi devam eder
- Tamamlaninca `PARCEL_WAIT` veya `NEXT_PARCEL`

`DOSE_WINDOW`

- pH/EC hedef disinda ise dozaj alt makinesi calisir
- Hedefler uygunsa tekrar `PARCEL_ACTIVE`

`PARCEL_WAIT`

- Iki parsel arasi bekleme uygulanir

`NEXT_PARCEL`

- Siradaki parsele gecilir
- Parsel kalmadiysa `COMPLETE`

`COMPLETE`

- Tum cikislar kapanir
- Ozet istatistik guncellenir
- `SAFE_IDLE`

`FAULT`

- Hata kaydi tutulur
- Tum cikislar kapanir
- Kullanici resetine kadar beklenir veya fault tipine gore temizlenir

`EMERGENCY_STOP`

- Tum cikislar aninda kapanir
- Program iptal edilir
- Elle onay olmadan otomatik devam edilmez

### 5.2 Dozaj Alt Makinesi

`DOSE_IDLE`

- Doz ihtiyaci yok

`DOSE_PRECHECK`

- Hangi kanal aktif olacak belirlenir
- Maksimum duty ve sure limitleri kontrol edilir

`DOSE_PH`

- Asit veya gerekiyorsa baz kanali belirlenen duty ile surulur

`DOSE_EC`

- Receteye gore bir veya birden fazla gubre kanali surulur

`DOSE_MIXING`

- Dozaj kapatilir
- Karistirma icin bekleme uygulanir

`DOSE_SETTLING`

- Akis sakinlesmesi icin beklenir

`DOSE_REMEASURE`

- Sensor tekrar okunur
- Hedefe ulasildiysa `DOSE_IDLE`
- Ulasilmadiysa kontrollu ikinci tura donulur

`DOSE_FAULT`

- Maksimum deneme, sensor hatasi veya zaman asimi durumunda fault uretilir

## 6. Emniyet ve Interlock Kurallari

MVP'de yazilim tarafinda asagidaki interlock kurallari zorunlu olmalidir:

- `close_all()` her zaman tek bir merkez fonksiyon uzerinden calismali
- `Emergency stop` durumunda parsel ve dozaj cikislari ayni anda kapanmali
- Bir parsel aktifken yazilim disi bir ikinci parsel acilamaz
- pH ve EC dozaj fazlari ayni anda baslamaz
- `sensor invalid` iken yeni dozaj fazi baslatilmaz
- `fault latched` durumunda kullanici reseti olmadan yeni program baslatilmaz
- Enerji geri geldiginde sistem otomatik vana acma yapmaz

## 7. Refactor Plani

Mevcut yapida `irrigation_control.c` icinde program akisi, dozaj, runtime backup ve alarm mantigi birlikte buyuyor. Mobil arayuze gecmeden once bu ayristirma yapilmalidir.

### Faz 1: Valve Driver Stabilizasyonu

Hedef dosyalar:

- `Core/Inc/valves.h`
- `Core/Src/valves.c`

Isler:

- Parsel ve dozaj cikislarinin komut API'sini sadelestir
- Vanaya ozel fault/status alanlarini netlestir
- Dosing PWM zamanlama kodunu tek sorumlulukta tut
- `close_all`, `emergency_stop`, `set_duty` davranislarini test edilebilir hale getir

Teslim:

- Donanimdan bagimsiz davranis bekleyen net fonksiyon seti

### Faz 2: Dosing Controller Ayrisma

Yeni modul onerisi:

- `Core/Inc/dosing_controller.h`
- `Core/Src/dosing_controller.c`

Isler:

- pH ve EC doz karar mantigini `irrigation_control.c` icinden tasimak
- Mixing, settling ve re-measure akislarini bu modulde toplamak
- Tarif oranlarinin uygulanmasini tek yere cekmek

Teslim:

- `start_dose_cycle`
- `update_dose_cycle`
- `stop_dose_cycle`
- `get_dose_status`

### Faz 3: Parcel Scheduler Ayrisma

Yeni modul onerisi:

- `Core/Inc/parcel_scheduler.h`
- `Core/Src/parcel_scheduler.c`

Isler:

- Queue ve sira mantigini ayirmak
- Programdaki aktif vana sirasini ayrik bir runtime yapisinda tutmak
- Tek parsel, tum parseller ve program tabanli calismayi ayni scheduler catisinda toplamak

Teslim:

- `build_sequence`
- `start_sequence`
- `update_sequence`
- `advance_sequence`
- `cancel_sequence`

### Faz 4: Fault Manager Ayrisma

Yeni modul onerisi:

- `Core/Inc/fault_manager.h`
- `Core/Src/fault_manager.c`

Isler:

- Hata kodlarini, latched/non-latched davranisini ve alarm metinlerini merkezilestirmek
- GUI veya mobil arayuz sonradan degisse bile ayni hata modelini korumak

Teslim:

- `raise_fault`
- `clear_fault`
- `has_active_fault`
- `get_alarm_text`
- `requires_manual_ack`

### Faz 5: Orchestrator Sadelestirme

Hedef dosyalar:

- `Core/Inc/irrigation_control.h`
- `Core/Src/irrigation_control.c`

Isler:

- Bu modulun gorevini koordinasyon seviyesine indirmek
- Scheduler, dosing ve fault manager arasindaki ust akisi yonetmek
- EEPROM runtime backup ve durum gecislerini burada koordine etmek

Teslim:

- Daha kucuk, okunabilir ve test senaryosuna uygun bir ana kontrol modulu

## 8. Mevcut Dosya Yapisi Icin Onerilen Sorumluluk Dagilimi

- `valves.*`
  Fiziksel cikis surme, valve state, duty/ramp

- `sensors.*`
  Sensor okuma, filtreleme, validity

- `dosing_controller.*`
  pH/EC doz mantigi ve alt durum makinesi

- `parcel_scheduler.*`
  Parsel sirasi, sure, bekleme, tekrar

- `fault_manager.*`
  Alarm, fault, interlock, manual acknowledge

- `irrigation_control.*`
  Ust koordinasyon ve run lifecycle

- `gui.*`
  Sadece gosterim ve komut girisi

## 9. Donanim Dogrulama Sirasi

Mobil arayuze gecmeden once kart ustunde asagidaki sira ile dogrulama yapilmalidir:

1. Parsel vanalari tek tek dogru cikisa gidiyor mu
2. Dozaj vanalari tek tek dogru duty ile suruluyor mu
3. `close_all` tum kanallari aninda kapatiyor mu
4. Bir tam parsel dongusu zamanlama hatasi olmadan tamamlanabiliyor mu
5. pH ve EC doz akislari GUI'yi kilitlemeden ilerliyor mu
6. Sensor hatasinda fault durumu latched sekilde tutuluyor mu
7. Reset sonrasi cihaz guvenli `SAFE_IDLE` durumunda aciliyor mu

## 10. Kabul Testi Paketi

Bu MVP tamamlandi sayilabilmesi icin en az su testler gecmelidir:

- 8 parsel vanasi icin manuel ac/kapat testi
- 5 dozaj vanasi icin duty testi
- 1 tam program akisi: parsel, bekleme, sonraki parsel
- 1 tam faz akisi: pre-flush, dosing, post-flush
- 1 pH dusurme doz senaryosu
- 1 EC arttirma doz senaryosu
- 1 sensor timeout senaryosu
- 1 emergency stop senaryosu
- 10 yeniden baslatma sonrasi guvenli acilis testi

## 11. Sonraki Adimlar

Bu dokumana gore ilk uygulama sprint'i su sekilde baslatilabilir:

1. `valves` modulu icin net API ve interlock temizligi
2. `irrigation_control` icinden dozaj akislarini ayri modula tasima
3. Queue ve parsel runtime mantigini scheduler modulune ayirma
4. Hata/alarm modelini tek merkezde toplama
5. Kart ustu smoke test ve kabul checklist guncellemesi

Bu paket tamamlandiginda mobil uygulama artik dogrudan donanima degil, saglamlasmis bir kontrol cekirdegine baglanir.

## 12. Segment Uyum Kurali

Refactor sirasinda su kural korunmalidir:

- `Core` surum compile-time veya runtime feature flag ile calisabilmeli
- `Insight` surum ayni state machine'i daha fazla sensorle beslemeli
- `Core` icin gerekli olmayan sensor yoklugunda kontrol akisi bozulmamali
- GUI, ozellik gosterimini aktif urun seviyesine gore sadeleştirebilmeli
