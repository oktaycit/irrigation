# Turkiye Pazar Projeksiyon Karsilastirmasi

Bu dokuman, Turkiye'deki tarimsal isletme yapisi ve pH/EC kontrollu dozlama
sistemlerinde piyasa kapasite tercihleri ile projenin mevcut urun
projeksiyonunu karsilastirir. Amac, hedefleri ticari olarak daha dogru bir
ilk urun noktasina cekmektir.

## 1. Pazar Varsayimlari

Calisma varsayimi olarak Turkiye'deki ana hedef kitle, 100 dekar altindaki
kucuk ve orta olcekli isletmelerdir. Ortalama isletme buyuklugunun yaklasik
60-70 dekar seviyesinde olmasi, ilk urunun asiri buyuk endustriyel kapasiteye
degil, moduler ve maliyet etkin bir mimariye odaklanmasi gerektigini gosterir.

Hedef kullanim senaryolari:

- 50-100 dekar acik alan sebze/meyve uretimi
- 10-20 dekar profesyonel sera
- Daha kucuk sera ve fide uygulamalari icin ayni kontrol kartinin dusuk
  kapasiteli varyanti

## 2. Mevcut Projeksiyon Ile Uyum

| Alan | Mevcut proje durumu | Pazar verisiyle uyum | Karar |
|------|---------------------|----------------------|-------|
| Segmentasyon | `Core` ve `Insight` ayrimi var | Uyumlu | Korunacak |
| Parsel sayisi | 8 parsel | 50-100 dekar segmenti icin yeterli | Korunacak |
| Kanal yapisi | 4 EC + 1 pH/asit cekirdegi | Turkiye fertigation recetelerine uygun | Ilk ticari standart olacak |
| Kontrol cekirdegi | pH/EC, flush, fault, scheduler | Saha guvenilirligi icin dogru | Korunacak |
| Mekanik model | Eski alternatif dozaj modeli kaldirildi | Pazar icin en uygun yol Venturi/Z-bypass | Ticari ilk SKU yalniz Venturi/Z-bypass olacak |
| Debi olcumu | Opsiyonel/gelecek faz | Hacim bazli doz ve dogrulama icin kritik | Arayuz ve kalibrasyon ilk tasarimda hazir olacak |
| Sensor paketi | Core az sensor, Insight zengin sensor | Maliyet hassasiyetiyle uyumlu | Korunacak |

## 3. Guncellenen Ticari Ilk Urun Hedefi

Ilk ticari hedef, `Core` segmenti icin 4+1 kanalli, 110 mm ana hatlarla uyumlu,
Z-bypass mimarisine dayanan bir pH/EC kontrollu fertigation kontrolcusu
olmalidir.

Tavsiye edilen hedef zarfi:

| Parametre | Guncel hedef |
|-----------|--------------|
| Hedef isletme | 50-100 dekar acik alan veya 10-20 dekar sera |
| Ana hat uyumu | 110 mm ana boru, yaklasik 40-45 m3/sa tipik debi |
| Hidrolik baglanti | Z-bypass, ana hatti tamamen cihazdan gecirmeyen mimari |
| Kanal yapisi | 4 gubre + 1 asit |
| Gubre kanali kapasitesi | Kanal basina 50-600 l/sa ayarlanabilir hedef |
| Asit kanali kapasitesi | Yaklasik 50 l/sa nominal hedef |
| Dozlama orani | 1:100 ve 1:200 kalibrasyon profilleri |
| Destek pompa | Dusuk basincli sahalar icin opsiyonel booster cikisi |
| Kontrol | EC/pH geri beslemeli PID, ileride Fuzzy-PID opsiyonu |
| Sensor yerlesimi | Karisim sonrasi flow cell, hedef gecikme 5 sn alti |

## 4. Mekanik Hedef Revizyonu

Pompa tabanli dozlama mimarisi proje hedefinden tamamen kaldirilmistir. Ticari
`Core` mekanik hedefi yalniz su mimariye dayanmalidir:

- Venturi tabanli 4+1 kanal
- Z-bypass kolektor
- her kanalda cekvalf, emme filtresi, servis vanasi ve kalibrasyon tup baglantisi
- opsiyonel 1.5 kW booster pompa kontrol cikisi
- pH/EC flow cell, statik mikser ve debi olcer baglanti noktasi
- Schedule 80 PVC veya kimyasal dayanimli muadil tesisat

## 5. Firmware Hedef Revizyonu

Firmware mimarisi temel olarak dogru yondedir. Gerekli guncelleme, hidrolik
kapasitenin yazilim veri modelinde acikca temsil edilmesidir.

Eklenmesi gereken hedef alanlar:

- kanal basina `nominal_capacity_lph`
- kanal basina minimum/maksimum guvenli duty veya acma suresi
- dozlama orani profili: `1:100`, `1:200`, ozel
- ana hat debisi icin `flow_k_factor` veya `nominal_flow_m3h`
- booster pompa kullanimi icin `booster_required` ve interlock
- sensor gecikmesi icin saha kalibrasyonlu `feedback_delay_ms`, hedef 5 sn alti

Bu alanlar ilk asamada gercek debimetre olmadan nominal kalibrasyonla
calisabilir. Debi sensoru eklendiginde ayni model fiziksel olcumle beslenir.

## 6. Hedef Guncelleme Karari

Guncellenen karar:

1. Ilk ticari urun hala `Core` olmalidir.
2. `Core` sadece yazilim olarak sade degil, mekanik olarak da Venturi/Z-bypass
   tabanli maliyet etkin bir saha urunu olmalidir.
3. Alternatif mekanik dozlama varyantlari urun hedefinden tamamen
   kaldirilmistir.
4. `Insight`, ayni kontrol cekirdegi uzerinde debi, basinç, log, recete ve
   uzaktan izleme katmani olarak gelmelidir.
5. Teknik backlog, kanal kapasitesi, dozlama orani, debi kalibrasyonu ve
   booster pompa interlock islerini icermelidir.

Bu revizyonla proje, Turkiye'deki 100 dekar alti isletme yogunluguna daha
dogrudan hitap ederken, daha buyuk isletmeler icin de moduler genisleme yolunu
acik tutar.
