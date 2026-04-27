# Internet Tarimsal Veri ve Kamera Destekli Sulama Plani

Bu not, `Insight` surumunun ek fiziksel sensor zorunlulugu olmadan bulundugu konumun atmosferik, tarimsal ve urun donemi verilerini kullanarak sulama programlarini daha akilli sekilde uygulamasi icin hedef mimariyi tanimlar.

## 1. Net Karar

`Insight` icin yeni yon:

- Gateway, konumdan gelen internet verilerini "sanal sensor" gibi toplar.
- Urun tipi, fenolojik donem, sera/acik alan bilgisi ve parsel profili yerel veri modelinde tutulur.
- Kamera modulleri bitki ve saha durumunu gozlemleyen ek kanit kaynagi olur.
- Sera modunda SCD41, DS18B20 ve iklim cikislari ayni context motoruna baglanir.
- STM32, nihai vana/fault/sulama emniyeti uygulayicisi olarak kalir.
- Gateway, once tavsiye ve program oneri katmani olarak calisir; sinirli otomatik duzeltmeler operator politikasi ile acilir.

## 2. Veri Kaynaklari

Ilk mimari tek bir saglayiciya baglanmamalidir. Gateway'de `context provider` adapter'lari kullanilmalidir.

Baslangic adaylari:

- MGM Tarimsal Hava Tahmini: Turkiye icin resmi tarimsal hava ve uyarilar.
- NASA POWER Agroclimatology: konuma dayali global gunes/radyasyon ve meteoroloji verileri.
- Open-Meteo: saatlik/tahmin verileri, radyasyon, ET0 ve toprak nemi gibi pratik alanlar.
- Copernicus CDS/ERA5-Land: daha agir ama tarihsel analiz ve model egitimi icin guclu kaynak.

Kaynak linkleri:

- [MGM Tarimsal Hava Tahmini](https://tarim.mgm.gov.tr/aciklamalar.html)
- [NASA POWER](https://power.larc.nasa.gov/)
- [NASA POWER Agroclimatology dokumantasyonu](https://power.larc.nasa.gov/docs/methodology/communities/)
- [Open-Meteo API dokumantasyonu](https://open-meteo.com/en/docs)
- [Copernicus CDS API dokumantasyonu](https://confluence.ecmwf.int/x/QhhsCg)
- [Insight Sera Iklimlendirme Plani](Docs/20_Sera_Iklimlendirme_Plani.md)

## 3. Gateway Context Engine

Gateway icinde yeni mantiksal servis:

`gateway/context-engine`

Gorevleri:

- cihaz konumunu, rakimi, sera/acik alan bilgisini ve timezone bilgisini tutmak
- hava tahmini, gerceklesen hava, yagis, sicaklik, nem, ruzgar, radyasyon ve ET0 verilerini cekmek
- internet yoksa son gecerli veriyi kullanmak ve veri yasini isaretlemek
- her parsel icin urun tipi, donem, hedef sulama stratejisi ve hassasiyet profilini tutmak
- kamera/cv gozlemlerini karar motoruna "kanit" olarak baglamak
- sulama programi icin onerilen duzeltmeleri uretmek
- operator onayi, limit ve audit log olmadan kritik setpoint yazmamak

Yerel store tablolarinin ilk taslagi:

- `site_profile`: konum, timezone, sera/acik alan, su kaynagi, ana hat bilgisi
- `parcel_profile`: parsel, urun, dikim tarihi, donem, kok bolgesi, hassasiyet
- `weather_observation`: kaynak, zaman, sicaklik, nem, yagis, ruzgar, radyasyon, ET0
- `crop_stage`: urun, donem, gun araligi, su stres katsayisi, hedef strateji
- `camera_observation`: kamera, zaman, parsel, goruntu kalitesi, bitki/zemin gozlemi
- `irrigation_advice`: onerilen aksiyon, guven skoru, gerekce, operator durumu

## 4. Karar Seviyeleri

### Seviye A - Tavsiye

Ilk guvenli faz.

- Gateway hava ve urun donemine gore "bugun sulama sayisini azalt/artir" der.
- Operator onaylamadan STM32 programi degismez.
- Dashboard, gerekceyi aciklar: sicaklik, ET0, yagis, radyasyon, urun donemi, kamera gozlemi.

### Seviye B - Limitli Otomatik Ayar

Saha verisi toplandiktan sonra acilir.

- Sadece onceden tanimli bantlar icinde sure, hedef hacim veya tekrar araligi duzeltilir.
- Ornek limit: gunluk su miktari en fazla `%20` artar veya azalir.
- Fault, sensor anomali veya eski internet verisi varsa otomasyon kilitlenir.

### Seviye C - Otonom Recete Takibi

Uzun vadeli `Insight` hedefi.

- Sistem urun donemi, hava, kamera ve tarihsel performansla parsel recetesini uygular.
- Operator politika belirler; cihaz uygulamayi takip eder ve sapmalari raporlar.
- STM32 emniyet kapisi ve manuel override her zaman korunur.

## 5. Kamera Modulu Rolu

Kamera ilk fazda kontrol emri uretmemeli, gozlem kalitesini artirmalidir.

Ilk kamera cikarimlari:

- parsel goruntu var/yok ve kamera sagligi
- bitki kaplama orani veya yesillik indeksi
- yaprak/bitki solgunluk belirtisi icin basit risk skoru
- zeminde islaklik/kuru gorunum sinifi
- damlatma hattinda belirgin su/akinti anomalisi icin uyarilar

Kamera karar motoruna su sekilde girer:

- `weather_context`: bugun beklenen su ihtiyaci
- `crop_context`: urunun donemine gore hassasiyet
- `camera_context`: sahada gozlenen stres veya islaklik belirtisi
- `device_context`: STM32 telemetry/fault/runtime durumu
- `climate_context`: ic sicaklik, nem, CO2, VPD ve iklim cikis durumu

## 6. Program Uygulama Modeli

Gateway, STM32'ye ham vana komutu yerine program veya profil onerisi yollar.

Onerilen model:

- `base_program`: operatorun sabitledigi guvenli program
- `context_overlay`: hava/urun/kamera ile hesaplanan gecici duzeltme
- `effective_program`: STM32'ye yazilacak nihai, limitlenmis program
- `explanation`: degisikligin sebebi ve veri kaynaklari

STM32 tarafinda korunacak kurallar:

- max sure / max tekrar / max gunluk hacim limitleri
- fault durumunda yeni program baslatmama
- manuel stop ve emergency stop onceligi
- pH/EC hedeflerinin guvenli aralik disina cikmamasi

## 7. Ilk Uygulama Sirasi

1. `site_profile` ve `parcel_profile` veri modelini gateway SQLite store'a ekle.
2. Konumdan hava/context cekebilen provider adapter iskeletini yaz.
3. ET0/radyasyon/yagis verisini dashboardda goster, program yazmadan once sadece logla.
4. Urun ve donem tablosunu ekle: fide/vegetatif/ciceklenme/hasat gibi sade evreler.
5. `irrigation_advice` uret: "artir", "azalt", "koru", "ertelemeyi dusun".
6. Operator onayli `context_overlay` ile program guncelleme akisini ekle.
7. Kamera modulu icin `camera_observation` pipeline'ini ekle.
8. Kamera siniflandirma sonucunu tavsiye gerekcesine kat.
9. Yeterli saha verisiyle limitli otomatik ayari feature flag arkasinda ac.

## 8. Basari Kriterleri

- Cihaz konum profili ile en az bir hava/context kaynagindan veri cekebilmeli.
- Internet kesilince son veri kullanilmali ama "stale" olarak isaretlenmeli.
- Tavsiyeler her zaman gerekce, veri yasi ve guven skoru ile gorunmeli.
- Operator onayi olmadan kritik sulama parametresi degismemeli.
- Kamera yokken sistem calismali; kamera varsa tavsiye kalitesi artmali.
- STM32 guvenlik/fault kararlari gateway kararlarindan ustte kalmali.

## 9. Urun Vaadi

Bu yon, `Insight` urununu su cumleyle konumlandirir:

"Ek sensor zorunlulugu olmadan konum, hava, urun donemi ve kamera gozlemleriyle sulamayi takip eden ve operatoru dogru karara yonlendiren akilli sulama asistani."
