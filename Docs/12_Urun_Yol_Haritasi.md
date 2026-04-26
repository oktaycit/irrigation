# Urun Yol Haritasi

Bu dokuman, urunun iki hedef segmentini gozden kacirmadan hangi sirayla gelistirilmesi gerektigini tanimlar.

Temel kural:

- `Core` ve `Insight` iki ayri proje degil, ayni platformun iki urun seviyesidir.
- Yol haritasi once `Core` segmentini sahaya guvenle cikaracak sekilde ilerlemeli, sonra `Insight` seviyesinin ayirt edici degerini acmalidir.
- Her fazda "hangi segmente dogrudan deger uretiyoruz?" sorusu net cevaplanmalidir.

2026-04-26 karar guncellemesi:

- `Insight` surumunun ana arayuz ve operasyon katmani Raspbian/Raspberry Pi OS tarzi bir Linux edge arayuzu uzerinden ilerlemelidir.
- STM32 TFT ekrani `Core` ve temel yerel emniyet/servis akisi icin korunur; `Insight` dashboard, log, recete, commissioning ve raporlama deneyimi gateway uzerindeki web arayuzune tasinir.
- Bu nedenle `Raspberry Pi` benzeri edge katmani artik gec Faz 4 eklentisi degil, `Insight` yolunun erken omurgasidir.

## 1. Hedef Segmentler

### `Core`

- Hedef musteri: kucuk ciftciler
- Satin alma motivasyonu: dusuk maliyet, kolay kurulum, az hata
- Beklenen urun karakteri: sade, guvenilir, teknik bilgi istemeyen

### `Insight`

- Hedef musteri: inovatif ve isletme mantigi ile calisan tarimsal kuruluslar
- Satin alma motivasyonu: daha fazla kontrol, veri, optimizasyon ve izlenebilirlik
- Beklenen urun karakteri: olcen, dogrulayan, raporlayan ve operatoru yonlendiren

## 2. Yol Haritasi Kararlari

Yol haritasi su ilkelere gore ilerlemelidir:

- `Core` segmenti icin olmayan bir ozellik, ilk fazda urunu karmasiklastirmamali.
- `Insight` segmenti icin gerekli donanim genisleme noktalarini kapatmamak gerekir.
- Saha guvenilirligi, yeni ozelliklerden once gelmelidir.
- Kullanilabilirlik, yalniz arayuz sadelelestirmesi degil; kurulum ve hata yonetimi kolayligi olarak ele alinmalidir.

## 3. Fazlar

## Faz 0 - Kontrol Cekirdegini Saglamlastir

Hedef segment:

- Dogrudan `Core`
- Dolayli olarak `Insight`

Ana hedef:

- Cihazin sahada guvenilir bir sulama ve dozaj kontrolcusu gibi davranmasi

Teslim edilmesi gerekenler:

- 8 parsel vana kontrolunun tamamlanmasi
- pH ve EC dozaj akislarinin kararlı calismasi
- `pre-flush`, `dosing`, `post-flush` akisinin netlestirilmesi
- `fault manager`, `parcel scheduler`, guvenli baslatma ve guvenli durdurma
- enerji kesintisi sonrasi emniyetli davranis
- EEPROM uzerinden temel parametre kaliciligi

Bu fazin cikisi:

- "Cihaz calisiyor mu?" sorusuna guvenilir bir evet

## Faz 1 - `Core` Ticari Surumu

Hedef segment:

- `Core`

Ana hedef:

- Kurulumu ve kullanimi kolay, uygun maliyetli ilk satisa hazir urun

Oncelikli ozellikler:

- hizli kurulum sihirbazi
- sade TFT ana ekran ve net alarm dili
- basit mod: ankraj + tekrar + gunluk limit mantigi
- sure tabanli sulama ve yazilimsal litre tahmini
- dusuk su seviyesi emniyeti
- USB uzerinden kolay programlama ve servis modu
- recete yerine kullanici dostu hazir profil mantigi
- 4 gubre + 1 asit kanal yapisinin standart urun hedefi olarak sabitlenmesi
- 1:100 ve 1:200 dozlama orani profilleri
- kanal basina nominal kapasite kalibrasyonu, hedef aralik: 50-600 l/sa
- 110 mm ana hat ve Z-bypass kurulum rehberi
- opsiyonel booster pompa cikisi ve temel interlock mantigi

Urun vaadi:

- "Karmasik ayar istemeden guvenilir sulama ve temel fertigation"

Ticari not:

- Ilk satis ve sahadan geri bildirim toplama bu fazin ana amacidir.

## Faz 2 - `Insight` Altyapisini Ac

Hedef segment:

- Once `Core` platformu
- Sonraki hedef `Insight`

Ana hedef:

- Ortak platformu Linux edge arayuzu ve veri odakli kararlar icin hazirlamak

Oncelikli ozellikler:

- STM32 icin standart `device_info`, `telemetry_snapshot`, `fault_event` ve `runtime_event` paketleri
- `Raspberry Pi` / Raspberry Pi OS benzeri edge gateway MVP
- yerel REST API ve websocket olay akisi
- SQLite tabanli yerel log ve telemetry kaydi
- cihaz ustu web dashboard prototipi
- debi sensoru entegrasyonu
- debimetre K-faktoru ve ana hat debisi kalibrasyonu
- `target_volume_l` ve gercek/likide litre bazli bitis mantigi
- learned-flow benzeri beklenen akis modeli
- akis yok / dusuk akis / yuksek akis alarmlari
- hat basinc geri bildirimi icin giris katmani
- parsel bazli recete veri modeline gecis
- feature flag ile `Core` ve `Insight` ayrimi

Urun vaadi:

- "Ayni kontrol cekirdegi, Linux edge arayuzu ile olcen ve dogrulayan bir urune evriliyor"

## Faz 3 - `Insight` Fark Yaratan Surum

Hedef segment:

- `Insight`

Ana hedef:

- Cihazi sadece kontrol eden degil, karar kalitesini artiran bir urune donusturmek

Oncelikli ozellikler:

- parsel bazli recete ve profil secimi
- isik birikimi veya dinamik ihtiyac tetikleme
- sensor gate / inhibit mantigi
- basinç ve debi ile capraz dogrulama
- Linux edge arayuzunde gelismis commissioning ekrani
- alarm gecmisi ve olay gunlugu
- bakim zekasi: kalibrasyon hatirlatma, vana cevrim sayisi, servis uyarilari

Urun vaadi:

- "Olc, karar ver, dogrula, optimize et"

Ticari fark:

- Bu faz, `Insight` segmentinde fiyat ve katma deger artisini hakli cikarir.

## Faz 4 - Baglanti ve Operasyon Katmani

Hedef segment:

- Once `Insight`
- Sonra ihtiyaca gore premium `Core` varyanti

Ana hedef:

- Cihazi tekil saha kutusu olmaktan cikarip operasyon aracina cevirmek

Oncelikli ozellikler:

- uzaktan izleme
- alarm bildirimi
- log export ve raporlama
- Linux edge arayuzunun uzak erisim ve servis modlariyla genisletilmesi
- saha teknisyeni icin servis ekranlari
- yerel ag uzerinden mobil servis ve commissioning akisi
- internet senkronizasyonu ve cihaz filolari icin bulut katmani
- AI destekli alarm ozetleme, tani ve optimizasyon onerileri
- gerekirse kablosuz baglanti veya mobil eslik katmani

Urun vaadi:

- "Sahadaki cihazdan fazlasi: izlenebilir operasyon"

Bagli mimari icin detay:

- [Docs/14_Baglanti_Mobil_AI_Mimarisi.md](Docs/14_Baglanti_Mobil_AI_Mimarisi.md)

## 4. Segment Bazli Ozellik Haritasi

| Ozellik | Faz | `Core` | `Insight` |
|---------|-----|--------|-----------|
| Guvenli kontrol cekirdegi | Faz 0 | Zorunlu | Zorunlu |
| Kurulum sihirbazi | Faz 1 | Zorunlu | Var |
| Sade profil tabanli kullanim | Faz 1 | Zorunlu | Opsiyonel |
| USB servis/programlama | Faz 1 | Var | Var |
| 4+1 kanal kapasite kalibrasyonu | Faz 1 | Zorunlu | Zorunlu |
| Z-bypass / 110 mm saha kurulum hedefi | Faz 1 | Zorunlu | Var |
| Booster pompa cikisi | Faz 1 | Opsiyonel | Var |
| Linux edge gateway / Raspbian arayuz | Faz 2 | Premium opsiyon | Zorunlu |
| Debi sensoru | Faz 2 | Opsiyonel | Zorunluya yakin |
| Hacim bazli bitis | Faz 2 | Opsiyonel | Zorunluya yakin |
| Learned flow ve akis alarmi | Faz 2 | Opsiyonel | Var |
| Parsel bazli recete | Faz 3 | Yok veya sade profil | Zorunlu |
| Dinamik ihtiyac / isik tetikleme | Faz 3 | Yok | Var |
| Bakim zekasi | Faz 3 | Temel | Gelismis |
| Uzak izleme | Faz 4 | Opsiyonel premium | Var |

## 5. Simdi Yapilacaklar

Bu repo icin yakin vade uygulama sirasi su olmali:

1. `Faz 0` kalemlerini tamamla ve saha guvenilirligini kanitla
2. `Core` kullanim akisini sadelelestir: kurulum sihirbazi, profil mantigi, alarm dili
3. `Core` hidrolik hedefini 4+1 Venturi/Z-bypass ve 50-600 l/sa kanal kalibrasyonu olarak sabitle
4. STM32 icin gateway'e uygun telemetry, event ve config protokolunu sabitle
5. `gateway/` altinda Linux edge device-agent ve yerel dashboard prototipini baslat
6. Debi sensorunu ortak platform genislemesi olarak tasarla
7. Program modelini parsel bazli recete ve hacim bitisini tasiyacak sekilde evrilt
8. `Insight` ozelliklerini Linux edge arayuz uzerinden ayri deger paketi olarak ac

## 6. Net Karar

Urun yol haritasinin omurgasi su olmalidir:

- Pazara ilk cikis: `Core`
- Teknoloji yonu: Linux edge arayuzlu `Insight`
- Ortak platform: tek firmware cekirdegi, segmente gore acilan ozellik katmanlari

Boylece urun hem erken satilabilir, hem de zamanla daha yuksek katma degerli bir sisteme donusebilir.

Insight edge arayuz detayi:

- [Docs/17_Insight_Raspbian_Arayuz_Plani.md](Docs/17_Insight_Raspbian_Arayuz_Plani.md)
