# Urun Segmentasyonu

Bu dokuman, cihazin iki farkli musteri kitlesi icin nasil konumlanacagini tanimlar.

## 1. Hedef Segmentler

### `Core`

- Hedef kitle: kucuk ciftciler
- Ana beklenti: dusuk maliyet, kolay kurulum, stabil calisma
- Satin alma mantigi: "en az parayla en az sorun"

### `Insight`

- Hedef kitle: inovatif ve isletme mantigi ile calisan tarimsal kuruluslar
- Ana beklenti: daha fazla veri, daha iyi optimizasyon, operasyonel kontrol
- Satin alma mantigi: "yatirimin geri donusu, verimlilik, izlenebilirlik"

## 2. Ortak Cekirdek

Iki urun de ayni ana kontrol cekirdegi uzerine kurulmalidir:

- 8 parsel kontrolu
- pH ve EC dozaj mantigi
- pre-flush / dosing / post-flush akisi
- fault manager
- parcel scheduler

Yani iki farkli urun, iki farkli firmware degil; ayni platformun iki seviyesi olmalidir.

## 3. Segmentler Arasi Fark

| Alan | `Core` | `Insight` |
|------|--------|-----------|
| Donanim | Minimum gerekli sensor ve IO | Genis sensor paketi |
| Kurulum | Hizli ve sade | Daha detayli commissioning |
| Tetikleme | Periyot ve temel hacim mantigi | Isik, hacim, basinç ve gate mantigi |
| Reçete | Daha sade | Parsel bazli ve daha esnek |
| Arayuz | STM32 TFT uzerinde az parametre | Raspbian/Raspberry Pi OS tarzi Linux edge dashboard |
| Veri kullanimi | "calissin yeter" | "olc, anla, optimize et" |

## 4. `Core` Surum Onerisi

### Donanim

- RTC
- dusuk su seviyesi girişi
- 8 parsel vana cikisi
- pH ve EC cekirdegi
- opsiyonel debi olcumu
- 4 gubre + 1 asit kanalina hazir dozaj cikis mimarisi
- 110 mm ana hatlarla kullanilabilecek Z-bypass hidrolik mimariye uyum
- kanal basina 50-600 l/sa ayarlanabilir Venturi hedefi icin kalibrasyon destegi
- dusuk basincli sahalar icin opsiyonel booster pompa cikisi

### Yazilim

- Basit mod varsayilan olmali
- Saat yerine ankraj + periyot mantigi kullanilmali
- Program kurulumu en fazla birkac karar ile tamamlanmali
- Sensor eksikliginde fallback akislar bulunmali
- 1:100 ve 1:200 dozlama orani profilleri hazir gelmeli
- Debi sensoru yoksa nominal ana hat debisi ve kanal kapasitesiyle yazilimsal litre/doz tahmini yapabilmeli

### Urun vaadi

- Daha az ayar
- Daha az hata
- Dusuk maliyetle verimli sulama
- 50-100 dekar acik alan veya 10-20 dekar sera segmentine uygun maliyet etkin fertigation

## 5. `Insight` Surum Onerisi

### Donanim

- Isik sensoru
- Debi sensoru
- Dusuk su seviyesi
- Hat basinç sensoru
- opsiyonel drenaj ve toprak nem girisleri
- gercek debimetre ve K-faktoru kalibrasyonu
- kanal bazli kapasite dogrulama ve servis kalibrasyon ekipmani

### Yazilim

- Pro mod aktif olmali
- Ana operator/servis arayuzu Linux edge cihazdaki web dashboard olmali
- STM32 TFT ekrani temel durum, alarm ve gateway yokken minimal servis icin kalmali
- `light bucket` mantigi kullanilmali
- Gercek litre bazli bitis desteklenmeli
- Basinç ve akiş tutarsizliklari fault veya warning uretmeli
- Parsel bazli profiller ve receteler desteklenmeli
- Yerel log, alarm gecmisi, commissioning ve servis ekranlari gateway uzerinde tutulmali

### Urun vaadi

- Verimlilik optimizasyonu
- Operasyonel izlenebilirlik
- Daha dogru karar ve daha az saha kaybi
- Sahada internet olmasa bile yerel ekrandan veya yerel agdan zengin operasyon paneli

## 6. Gelistirme Stratejisi

En saglikli siralama:

1. Once `Core` surumu saha icin olgunlastir
2. Donanimi `Insight` sensörleri icin genislemeye acik birak
3. Sonra ayni cekirdek uzerine `Insight` ozelliklerini ac

Bunun nedeni:

- `Core` urun daha hizli sahaya iner
- ilk gelir daha erken gelir
- `Insight` icin gereken algoritmalar daha iyi saha verisiyle gelistirilir

## 7. Teknik Mimari Kurali

Kod ve donanim kararlari verilirken su kural korunmali:

- `Core` her zaman tek basina degerli olmali
- `Insight` yalnizca "fazla ozellik" degil, olculebilir ek fayda uretmeli
- Bir `Insight` ozelligi, mumkunse `Core` kartina sonradan eklenebilir olmali

## 8. Net Oneri

Bu proje icin en dogru urun stratejisi:

- Ilk ticari odak: `Core`
- Ilk mekanik/hidrolik odak: Venturi tabanli 4+1 Z-bypass dozlama unitesi
- Mimarinin yonu: Linux edge arayuzlu `Insight`

Boylece urun hem pazara hizli cikabilir, hem de ucuz cihaz olarak tikanmaz; ileride daha yuksek katma degerli bir platforma donusebilir.

Detayli Turkiye pazar/projeksiyon karsilastirmasi:

- [Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md](Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md)

## 9. Yol Haritasi Iliskisi

Bu segmentasyon kararlari, urun yol haritasinda su sekilde uygulanmalidir:

- `Core` once saha icin guvenilir ve kolay kullanilir surum olarak cikmalidir.
- `Insight` daha sonra ayni platform uzerinde Linux edge arayuz, veri, tani ve optimizasyon katmani olarak acilmalidir.
- Bir ozellik hangi fazda gelirse gelsin, hangi segmente hizmet ettigi net belirtilmelidir.

Detayli fazlama icin:

- [Docs/12_Urun_Yol_Haritasi.md](Docs/12_Urun_Yol_Haritasi.md)
- [Docs/17_Insight_Raspbian_Arayuz_Plani.md](Docs/17_Insight_Raspbian_Arayuz_Plani.md)
