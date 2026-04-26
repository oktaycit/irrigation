# Dinamik Ihtiyac Modeli

Bu dokuman, urunun sulama mantigini "saat kacta basla" anlayisindan "bitki ne zaman ihtiyac duyuyor" anlayisina tasimak icin hedef mimariyi tanimlar.

## 1. Urun Karari

Bu proje tek vana degil, zaten `8 parsel` cikisina sahip bir `multi-zone` kontrolcu olarak konumlanmis durumdadir. Bu nedenle sulama mantigi yalnizca global bir zamanlayici olarak degil, `parsel bazli recete` ve `parsel bazli tetikleme` dusuncesiyle ele alinmalidir.

Temel urun karari:

- Basit mod kullaniciyi saat ve dakika detayina bogmaz.
- Pro mod, isik ve sensor verisinden faydalanarak sulamayi erteler veya siklastirir.
- Tum modlar ayni saha kontrol cekirdegi icinde calisir.
- Sulama akisi her zaman ayni fiziksel fazlara bolunur: `pre-flush`, `dosing`, `post-flush`.
- Bu mimari iki urun seviyesine haritalanir: `Core` ve `Insight`.

## 2. Tetikleme Seviyeleri

### 2.1 Seviye 1: Periyot Tabanli Basit Mod

Kullanici dogrudan `08:00` girmek yerine bir baslangic ankrajina ve tekrar mantigina karar verir.

Onerilen kullanici modeli:

- `Gun dogumundan 2 saat sonra basla`
- `Her 2 saatte bir tekrar et`
- `Gunde en fazla 5 kez sula`

Teknik notlar:

- Mevsim gecislerinde manuel saat kaydirmayi azaltmak icin LDR veya astronomik saat destegi kullanilir.
- Astronomik saat ilk fazda yazilimsal olarak eklenebilir; ek donanim zorunlu degildir.
- Basit mod, firmware icinde yine `dinamik tetikleme` olarak tutulur; yalnizca arayuz sadeleştirilir.

Ilk firmware modeli:

| Alan | Tip | Basit moddaki anlami | Ilk varsayilan |
|------|-----|----------------------|----------------|
| `trigger_mode` | enum | `FIXED_WINDOW`, `PERIODIC`, `SUNRISE_PERIODIC` | `FIXED_WINDOW` |
| `start_hhmm` | HHMM | `PERIODIC` icin sabit saat ankraji | `0600` |
| `anchor_offset_min` | int16 | Ankrajdan once/sonra dakika kaymasi | `0` |
| `period_min` | uint16 | Iki tetikleme arasi minimum sure | `120` |
| `max_events_per_day` | uint8 | Bir gunde en fazla kac tetikleme | `1` |
| `days_mask` | bitmask | Haftanin hangi gunleri aktif | `0x7F` |
| `last_run_day` / `last_run_minute` | runtime kaydi | Ayni slotun tekrar calismasini engeller | otomatik |

Karar kurali:

1. Program pasifse veya gun maskesi uymuyorsa tetiklenmez.
2. `FIXED_WINDOW` eski davranisi korur: pencere icinde gunde bir kez calisir.
3. `PERIODIC`, `start_hhmm + anchor_offset_min` dakikasini gunluk ankraj kabul eder.
4. `SUNRISE_PERIODIC`, ilk fazda yazilimsal `06:00 + anchor_offset_min` ankrajini kullanir.
5. `period_min` doldukca yeni slot acilir; `max_events_per_day` asildiginda o gun biter.
6. Program basariyla calismaya girince `last_run_day` ve `last_run_minute` yazilir.

Basit mod UI karsiligi:

- Ankraj: `Sabit Saat` veya `Gun Dogumu`
- Kaydirma: `-120..+720 dk`
- Tekrar: `30..1440 dk`
- Gunluk limit: `1..24`

Not: `SUNRISE_PERIODIC` icin `06:00` yalnizca ilk faz placeholder'idir. Astronomik hesap veya isik sensoru eklendiginde ayni alanlar korunur; sadece ankraj kaynagi daha dogru hale gelir.

### 2.2 Seviye 2: Hacim Tabanli Sulama

Sulama sonlandirma kriteri sure degil, hedef hacim olmalidir.

Onerilen model:

- Her parsel icin `target_volume_l` tutulur.
- Debi sensoru yoksa, vana acildiginda bir kere `nominal_flow_lpm` kalibre edilir.
- Runtime sirasinda yazilimsal litre sayaci hesaplanir.
- Debimetre eklenirse ayni API fiziksel olcumle beslenir.

Bu yaklasim su faydalari saglar:

- Hat basinci degisse de sulama dozu daha tutarli kalir.
- Gubreleme recetesi zaman kaymasindan daha az etkilenir.
- Post-flush suresi ayrik tutulabildigi icin damlatici temizligi korunur.

### 2.3 Seviye 3: Evapotranspirasyon / Isik Birikimi

Gercek inovasyon katmani, anlik isik siddetini bir "enerji kovasi" icinde biriktirerek sulama karari vermektir.

Onerilen model:

- Sensor girdi kaynagi: LDR, daha iyi bir PAR/isik sensoru veya yazilimsal radyasyon tahmini
- Runtime birikim alani: `light_integral_bucket`
- Esik parametresi: `bucket_threshold`
- Kullanici parametresi: `sensitivity`

Karar mantigi:

1. Isik siddeti periyodik olculur.
2. Deger zamanla carpilip kovaya eklenir.
3. Kova esigi asarsa bir sulama turu tetiklenir.
4. Sulama yapildiginda kova tamamen veya kismen bosaltilir.

Beklenen davranis:

- Bulutlu havada sulama kendiliginden seyreklesir.
- Gunesli havada daha sik sulama olur.
- Kullanici "daha sık" yerine yalnizca hassasiyet ayariyla sistemi yonetir.

## 3. Ortak Sulama Fazlari

Hangi tetikleme modeli secilirse secilsin, fiziksel sulama akisi asagidaki fazlarla calismalidir:

### 3.1 Pre-Flush

- Bekleyen veya isinmis suyu tahliye eder.
- Tipik sure: `60-120 sn`
- Dozaj vanalari bu fazda kapali kalir.

### 3.2 Dosing

- Ana sulama fazidir.
- Sirai PWM vanalari bu fazda aktif olur.
- pH ve EC hedefleri bu fazda kontrol edilir.
- Hedef bitis kriteri sure, hacim veya dinamik ihtiyac motorundan gelebilir.

### 3.3 Post-Flush

- Sulama bitmeden `2-3 dk` once dozaj kapanir.
- Hat ve damlaticilarda gubre kalmasi engellenir.
- Tikama ve yosun riskini azaltir.

## 4. Donanim Onerileri

Minimalist urun icin zorunlu ve opsiyonel genisleme alanlari:

### Zorunlu

- `2 adet dijital input` ayrilmali
- `RTC/astronomik saat` destegi olmali
- `8 parsel` yapisi korunmali

### Onerilen

- `LDR` veya daha lineer bir isik sensoru
- `Drenaj sensori` girisi
- `Toprak nem sensori` girisi
- Ana hat debisi icin kalibrasyon parametresi

Tercih edilen sensor seti:

- Isik icin `BH1750` veya `VEML7700`
- Hacim icin pulse cikisli debi sensoru
- Emniyet icin dusuk su seviyesi switch'i
- Saha tanisi icin `0-3.3V` uyumlu basinç sensoru

Dijital input kullanim modeli:

- DI-1: drenaj geri bildirimi veya dusuk basinç / akış teyidi
- DI-2: toprak nemi veya harici kilit girişi

Bu urun icin tavsiye edilen minimum sensor paketi:

- `Isik + debi + dusuk su seviyesi`

Saha guvenilirligi odakli paket:

- `Isik + debi + dusuk su seviyesi + hat basinç`

## 5. Yazilim Veri Modeli

Mevcut veri modeli buyuk olcude statik zaman alanlarina yaslaniyor:

- `start_hhmm`
- `end_hhmm`
- `irrigation_min`
- `wait_min`

Bir sonraki iterasyonda program modeli asagidaki alanlara evrilmelidir:

- `trigger_mode`
- `start_anchor`
- `max_events_per_day`
- `target_volume_l_x10`
- `nominal_flow_lpm_x10`
- `light_bucket_threshold`
- `sensitivity_level`
- `pre_flush_sec`
- `post_flush_sec`
- `sensor_gate_mask`
- `recipe_profile_id`

Boylece ayni parsel icin farkli urun tipleri veya mevsimlere gore farkli receteler tanimlanabilir.

## 6. Multi-Zone Sonucu

Bu urun icin dogru yon `multi-zone + parsel bazli recete` modelidir.

Bunun pratik anlami:

- Her parsel icin ayri sulama tetikleme profili tutulur.
- Her parsel icin ayri pH/EC hedefi veya recete profili secilebilir.
- Ortak dozaj altyapisi korunurken karar mantigi parsel bazinda farklilasir.
- Ayni anda yalnizca tek parsel acik kalir; recete degisimi parceller arasi geciste uygulanir.

## 7. Onerilen Urun Modlari

### Basit Mod

- Ankraj: gun dogumu / gun batimi / sabit saat
- Tekrar araligi
- Gunluk maksimum tekrar
- Sulama miktari: dakika yerine litre veya "standart tur"

### Pro Mod

- Isik birikimi tabanli tetikleme
- Sensor gate kullanimi
- Parsel bazli recete
- Pre-flush ve post-flush ince ayari

## 8. Urun Segmentlerine Haritalama

### `Core` Surum

- Hedef: kucuk ciftci, az kurulum, dusuk maliyet
- Arayuz: cok daha sade
- Tetikleme: periyot, sabit ankraj, gunluk tekrar siniri
- Bitiş: sure veya yazilimsal litre tahmini
- Sensorler: en az `RTC + dusuk su seviyesi`, tercihen debi

Bu seviyede hedef "en akilli cihaz" olmak degil, "en az ayarla guvenilir verim" sunmaktir.

### `Insight` Surum

- Hedef: isletme mantigiyla calisan profesyonel tarim kuruluslari
- Arayuz: daha fazla tanilama ve recete esnekligi
- Tetikleme: isik birikimi, hacim, sensor gate
- Bitiş: gercek litre, basinç dogrulama, drenaj geri bildirimi
- Sensorler: `isik + debi + dusuk su seviyesi + hat basinç`, opsiyonel drenaj ve nem

Bu seviyede hedef, yalniz sulama kontrolu degil; operasyonel kararlari destekleyen veri katmani da sunmaktir.

## 9. Firmware Yol Haritasi

Asagidaki sirayla ilerlemek riskleri dusurur:

1. `pre-flush`, `dosing`, `post-flush` fazlarini mevcut scheduler akisina ekle
2. Program modeline `trigger_mode` ve flush alanlarini ekle
3. Sure tabanli bitis kriterine ek olarak `hedef hacim` secenegi ekle
4. LDR/astronomik saat katmanini `dynamic trigger engine` olarak ayir
5. Dijital inputlardan biriyle `sensor gate` ve inhibit mantigini ekle
6. GUI'de `Basit Mod` ve `Pro Mod` ayrimini sun

Ek segmentleme adimi:

7. `Core` ve `Insight` ozellik setlerini feature flag ile ayir
8. Program modelinde sensor bagimliligini opsiyonel hale getir

## 10. Tasarim Ilkesi

Kullanici arayuzunde detay azaltilmali, firmware tarafinda ise karar sistemi zenginlestirilmelidir. Urun degeri, kullanicinin daha fazla saat girmesinde degil, daha az parametreyle daha dogru sulama yapabilmesinde ortaya cikar.
