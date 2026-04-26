# Sensor Genisleme Plani

Bu dokuman, urunun "dinamik ihtiyac" hedefine uygun ek sensorleri; donanim, pin atamasi ve firmware etkisiyle birlikte tanimlar.

Bu plan varsayilan olarak `Insight` urun seviyesini hedefler. `Core` seviyede bu sensorlerin bir kismi opsiyonel veya yok kabul edilmelidir.

## 1. Tasarim Karari

Ilk genisleme paketinde su dort sensor en yuksek degeri uretir:

1. Isik sensoru
2. Debi sensoru
3. Dusuk su seviyesi sensoru
4. Hat basinc sensoru

Opsiyonel katman:

- Drenaj sensori
- Toprak nem sensori

## 2. Onceliklendirme

| Sensor | Oncelik | Neden |
|--------|---------|------|
| Isik sensoru | P1 | `light bucket` ve mevsimsel kayma duzeltmesi |
| Debi sensoru | P1 | Sulamayi sure yerine hacimle bitirmek |
| Dusuk su seviyesi | P1 | Kuru calisma korumasi |
| Hat basinc sensoru | P2 | Akis teyidi, tikali filtre, pompa arizasi ayirimi |
| Drenaj sensoru | P3 | Geri bildirim ve inhibit |
| Toprak nem sensoru | P3 | Yardimci karar / alarm |

## 2.1 Segment Bazli Uygulama

| Sensor | `Core` | `Insight` |
|--------|--------|-----------|
| Isik | Opsiyonel | Zorunluya yakin |
| Debi | Opsiyonel | Zorunluya yakin |
| Dusuk su seviyesi | Zorunlu | Zorunlu |
| Hat basinc | Yok veya opsiyonel | Onerilen |
| Drenaj | Yok | Opsiyonel |
| Toprak nem | Opsiyonel | Opsiyonel |

## 3. Onerilen Fiziksel Sensorler

| Islev | Onerilen tip | Arabirim |
|------|----------------|----------|
| Isik | `BH1750` veya `VEML7700` | I2C |
| Debi | Hall etkili pulse cikisli debimetre | Dijital pulse |
| Dusuk su seviyesi | Samandira switch veya kuru kontak | Dijital input |
| Hat basinc | Endustriyel `0.5-4.5V` transmitter | Analog |
| Drenaj | Kuru kontak, optik veya iletkenlik tabanli seviye | Dijital input |
| Toprak nem | Kapasitif nem sensoru | Analog veya threshold dijital |

## 4. Onerilen Pin Atamasi

Bu oneriler, mevcut repodaki pin kullanimina gore cakisma en az olacak sekilde secilmistir.

| Sensor | MCU Pin | Arabirim | Firmware rolü |
|--------|---------|----------|----------------|
| Isik | `PB8/PB9` | I2C1 | `light_bucket`, gun dogumu ankraji, sensor sagligi |
| Debi | `PB13` | EXTI / counter | `delivered_volume_l`, akiş var teyidi |
| Dusuk su seviyesi | `PB14` | Dijital input | `ERR_LOW_WATER_LEVEL`, start inhibit |
| Drenaj | `PB15` | Dijital input | post-irrigation kontrol, yeni tur inhibit |
| Hat basinc | `PC5` | ADC | basinç esigi, filtre/pompa fault ayrimi |
| Toprak nem | `PC5` veya harici ADC | ADC | parcelye ozel inhibit veya advisory |

Varsayim:

- `PB13`, `PB14`, `PB15` custom sulama kartinda saha inputlari icin ayrilabilir.
- `PC5` custom kartta analog genisleme girisi olarak ayrilabilir.
- I2C1 hatti uzerindeki toplam kapasitans saha kablolamasina gore gozden gecirilmelidir.

## 5. Sensorlere Gore Firmware Davranisi

### 5.1 Isik Sensoru

Kullanim:

- `light_bucket += lux * delta_t`
- `bucket_threshold` asildiginda sulama turu tetiklenir
- Gun dogumu referansi gerekiyorsa lux esigiyle gunduz baslangici bulunabilir

Ariza davranisi:

- I2C timeout veya sabit deger algilanirsa `warning`
- Basit mod zaman/periyot mantigiyle devam edilebilir

### 5.2 Debi Sensoru

Kullanim:

- Her pulse bir `kalibrasyon_katsayisi` ile litreye cevrilir
- `target_volume_l` dolunca parsel sulamasi biter
- Beklenen basinç varken pulse yoksa fault uretilebilir

Ariza davranisi:

- Pulse yok ama vana acik ve basinç var ise `flow anomaly`
- Debi sensori yoksa yazilimsal sayaç fallback'i kullanilabilir

### 5.3 Dusuk Su Seviyesi

Kullanim:

- Program baslatmadan once interlock
- Calisma sirasinda aktif olursa tum dozaj ve parsel cikislarini kapat

Ariza davranisi:

- `latched fault` tavsiye edilir
- Kullanici onayi olmadan otomatik restart yapilmaz

### 5.4 Hat Basinc Sensoru

Kullanim:

- Vana acikken hatta yeterli basinç geliyor mu kontrol edilir
- Debi ile birlikte kullanilirsa:
  basinç var + debi yok = tikali hat / kapali vana
  basinç yok + debi yok = pompa veya besleme problemi

Ariza davranisi:

- Esik alti uzun surerse `warning` veya `fault`

### 5.5 Drenaj Sensoru

Kullanim:

- Sulama sonrasi drenaj var/yok geri bildirimi
- Asiri drenaj durumunda bir sonraki tur ertelenebilir

### 5.6 Toprak Nem Sensoru

Kullanim:

- Ana karar motoru yerine `gate` olarak tavsiye edilir
- Ornek: toprak halen islaksa yeni turu ertele

Sinirlama:

- Tek sensorle tum parseli temsil etmek risklidir

## 6. Onerilen Kart Revizyonu

Yeni kartta su konektorler ayrilmalidir:

- `J_I2C_SENS`
- `J_FLOW_IN`
- `J_LEVEL_IN`
- `J_DRAIN_IN`
- `J_PRESS_IN`

Elektriksel gereksinimler:

- Saha dijital girislerinde opto veya endustriyel seviye uyarlama
- Ters polarite ve ESD korumasi
- Analog hatta RC filtre ve TVS
- 5V sensor kullanilacaksa ADC icin bolucu/op-amp uyarlama

## 7. Yazilim Backlog'una Etkisi

Bu sensor paketi eklendiginde asagidaki isler acilmalidir:

1. `sensors.*` icine isik, basinç ve dijital input alt yapisi eklenmesi
2. `irrigation_program_t` icine `target_volume`, `bucket_threshold`, `sensor_gate_mask` alanlarinin eklenmesi
3. `parcel_scheduler` icinde sureye ek olarak hacim bazli bitis kontrolu
4. `fault_manager` icine `low_water`, `flow_anomaly`, `low_pressure` fault kodlari
5. GUI'de `Basit Mod` ve `Pro Mod` icin yeni sensor durum alanlari

## 8. Net Oneri

`Core` icin en dengeli ilk paket:

- RTC
- dusuk su seviyesi switch'i
- opsiyonel debi veya yazilimsal debi kalibrasyonu

`Insight` icin en dengeli ilk saha paketi:

- `BH1750` veya `VEML7700`
- pulse cikisli debi sensoru
- samandira tip dusuk su seviyesi switch'i
- `0.5-4.5V` cikisli hat basinç sensoru

Boylece urun:

- saat bazli cihaz olmaktan cikar
- litre bazli sulama yapabilir
- gunesli ve bulutlu hava arasinda fark koyabilir
- saha arizalarini daha dogru ayristirabilir
