# Dinamik Ihtiyac Modeli

Bu dokuman, urunun sulama mantigini "saat kacta basla" anlayisindan "bitki ne zaman ihtiyac duyuyor" anlayisina tasimak icin hedef mimariyi tanimlar.

## 1. Urun Karari

Bu proje tek vana degil, zaten `8 parsel` cikisina sahip bir `multi-zone` kontrolcu olarak konumlanmis durumdadir. Bu nedenle sulama mantigi yalnizca global bir zamanlayici olarak degil, `parsel bazli recete` ve `parsel bazli tetikleme` dusuncesiyle ele alinmalidir.

Temel urun karari:

- Basit mod kullaniciyi saat ve dakika detayina bogmaz.
- Pro mod, isik ve sensor verisinden faydalanarak sulamayi erteler veya siklastirir.
- `Insight` modu, ek sensor zorunlulugu olmadan konum bazli hava/tarimsal internet verisini sanal sensor gibi kullanir.
- Kamera modulleri bitki ve zemin gozlemini karar gerekcesine ekler.
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

### 2.3 Seviye 3: Internet Context / ET0 / Isik Birikimi

Gercek inovasyon katmani, ek sensor zorunlulugu olmadan konum bazli hava ve tarimsal veriyi bir "sanal iklim sensori" gibi kullanarak sulama karari vermektir. Fiziksel isik sensori daha sonra ayni modele daha dogru lokal veri saglar.

Onerilen model:

- Girdi kaynagi: hava tahmini, gerceklesen hava, global radyasyon, ET0, yagis, sicaklik, nem, ruzgar
- Opsiyonel lokal sensor: LDR, PAR/isik sensoru, debi, basinç, drenaj veya toprak nemi
- Runtime birikim alani: `light_integral_bucket`
- Context alanlari: `weather_et0_x100`, `radiation_wh_m2`, `rain_probability_pct`, `context_age_min`
- Esik parametresi: `bucket_threshold`
- Kullanici parametresi: `sensitivity`

Karar mantigi:

1. Gateway konum bazli hava/tarimsal veriyi periyodik ceker.
2. Radyasyon, ET0, sicaklik, nem ve yagis riski normalize edilir.
3. Urun tipi ve donemi ile parsel hassasiyeti uygulanir.
4. Sistem `koru`, `artir`, `azalt` veya `ertele` tavsiyesi uretir.
5. Operator onayi veya guvenli feature flag olmadan kritik program degismez.
6. Sulama yapildiginda bucket veya gunluk ihtiyac bakiyesi guncellenir.

Beklenen davranis:

- Bulutlu havada sulama kendiliginden seyreklesir.
- Gunesli havada daha sik sulama olur.
- Yagis riski varsa acik alan programi ertelenebilir veya azaltma onerisi alir.
- Sera modunda yagis dogrudan azaltma sebebi olmaz; radyasyon, sicaklik ve nem daha yuksek agirlik alir.
- Kullanici "daha sık" yerine yalnizca hassasiyet ayariyla sistemi yonetir.

### 2.4 Seviye 4: Kamera Destekli Gozlem

Kamera, ilk fazda dogrudan kontrol emri uretmez. Hava ve urun donemi tavsiyesinin sahadaki goruntuyle tutarliligini artirir.

Ilk kamera siniflari:

- kamera sagligi ve goruntu var/yok
- bitki kaplama veya yesillik orani
- zemin islak/kuru gorunum sinifi
- solgunluk/stres icin basit risk skoru
- belirgin akinti veya damlatma anomalisi uyarisi

Kamera sonucunun etkisi:

- hava verisi "azalt" derken zemin cok kuru gorunuyorsa tavsiye "koru"ya cekilebilir
- hava verisi "artir" derken zemin islak gorunuyorsa operator uyarisi uretilir
- kamera kalitesi dusukse karar motoru bunu guven skoru dususu olarak isaretler

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
- `site_profile_id`
- `parcel_profile_id`
- `crop_profile_id`
- `crop_stage_id`
- `context_policy`
- `max_context_adjust_pct`
- `pre_flush_sec`
- `post_flush_sec`
- `sensor_gate_mask`
- `recipe_profile_id`

Boylece ayni parsel icin farkli urun tipleri, urun donemleri, mevsimler ve konum bazli hava kosullarina gore farkli receteler tanimlanabilir.

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
- Internet hava/tarimsal context ile ET0/radyasyon/yagis tabanli tavsiye
- Urun donemi ve parsel profiline gore program overlay'i
- Kamera gozlemi ile tavsiye guven skoru
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
- Tetikleme: internet context, urun donemi, isik birikimi, hacim, sensor gate
- Bitiş: gercek litre, basinç dogrulama, drenaj geri bildirimi
- Sensorler: ek sensor zorunlu degil; `isik + debi + dusuk su seviyesi + hat basinç` saha kalitesini artirir, kamera opsiyonel/fark yaratan paket olur

Bu seviyede hedef, yalniz sulama kontrolu degil; operasyonel kararlari destekleyen veri katmani da sunmaktir.

## 9. Firmware Yol Haritasi

Asagidaki sirayla ilerlemek riskleri dusurur:

1. `pre-flush`, `dosing`, `post-flush` fazlarini mevcut scheduler akisina ekle
2. Program modeline `trigger_mode` ve flush alanlarini ekle
3. Sure tabanli bitis kriterine ek olarak `hedef hacim` secenegi ekle
4. Gateway tarafinda internet context modelini ve provider adapter'larini ekle
5. LDR/astronomik saat katmanini `dynamic trigger engine` olarak ayir
6. Dijital inputlardan biriyle `sensor gate` ve inhibit mantigini ekle
7. GUI'de `Basit Mod`, `Pro Mod` ve `Insight Tavsiye` ayrimini sun

Ek segmentleme adimi:

8. `Core` ve `Insight` ozellik setlerini feature flag ile ayir
9. Program modelinde sensor bagimliligini opsiyonel hale getir
10. Kamera gozlemlerini tavsiye motoruna bagla

## 10. Tasarim Ilkesi

Kullanici arayuzunde detay azaltilmali, firmware tarafinda ise karar sistemi zenginlestirilmelidir. Urun degeri, kullanicinin daha fazla saat girmesinde degil, daha az parametreyle daha dogru sulama yapabilmesinde ortaya cikar.

Internet context ve kamera plani icin:

- [Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md](Docs/19_Internet_Tarimsal_Veriler_ve_Kamera_Plani.md)
