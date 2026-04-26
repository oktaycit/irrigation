# Baglanti, Mobil ve AI Katmani Mimarisi

Bu dokuman, cihazi bir sonraki fazda `Raspberry Pi` benzeri bir ara katman ile daha etkin yonetmek, mobil arayuz eklemek ve internet/AI katmanlarini kontrollu sekilde sisteme dahil etmek icin onerilen mimariyi tanimlar.

Temel ilke:

- `STM32` kart, gercek zamanli saha kontrolcusu olarak kalir.
- `Raspberry Pi` benzeri cihaz, baglanti ve entegrasyon katmani olarak calisir.
- mobil uygulama ve bulut servisleri, dogrudan role veya dozaj vanasina degil `gateway` uzerinden erisir.
- `AI`, otomatik karar veren kontrol cekirdegi degil; once onerici, tani ve optimizasyon katmani olarak konumlanir.

Bu ayrim sayesinde internet veya mobil uygulama olmasa bile saha kontrolu calismaya devam eder.

## 1. Neden Ayrik Katman?

Bu urunun ana riski, baglanti ozellikleri eklenirken saha emniyetinin zayiflamasidir. Bu nedenle mimari su sekilde ayrilmalidir:

- `STM32`: vana, sensor, fault ve scheduler kontrolunu yapar
- `Gateway`: veri toplar, uzaktan erisim saglar, log senkronize eder
- `Cloud`: cihaz filolari, raporlama, alarm dagitimi ve AI servisleri icin kullanilir
- `Mobile`: operator ve teknisyen arayuzu olur

Bu model, urunu "tek kutu calisan kontrolcu"dan "bagli operasyon platformu"na tasir.

## 2. Onerilen Katmanlar

### 2.1 Saha Kontrol Katmani - `STM32`

Bu katman mevcut repodaki cekirdektir.

Sorumluluklari:

- parsel ve dozaj vanalarini surmek
- pH/EC olcumlerini almak
- `pre-flush`, `dosing`, `post-flush` akislarini yurutmek
- fault ve `emergency stop` davranisini uygulamak
- temel log ve parametre kaliciligini korumak

Bu katmana eklenmesi onerilen baglanti hazirligi:

- cihaz kimligi (`device_id`)
- firmware surumu
- fault/event kayitlari icin standart olay formati
- anlik durum paketi (`telemetry snapshot`)
- komut kabul katmani: `read-only`, `set parameter`, `manual action`, `ack fault`

### 2.2 Edge Gateway Katmani - `Raspberry Pi`

Bu katman icin `Raspberry Pi 4`, `Raspberry Pi CM4` veya sahaya gore `Zero 2 W` benzeri bir kart kullanilabilir.

Ana gorevleri:

- `STM32` ile haberlesmek
- yerel API saglamak
- veriyi gecici olarak yerelde tamponlamak
- internet varsa buluta senkronize etmek
- mobil istemciye yerel ag veya internet uzerinden servis vermek
- log, alarm, tanilama ve guncelleme akisini merkezilestirmek

Bu katmanin cihaz ustunde sunacagi servisler:

- `Device Agent`: STM32 haberlesme sureci
- `Local API`: REST veya gRPC
- `Message Broker`: tercihen `MQTT`
- `Sync Worker`: bulut senkronizasyonu
- `OTA/Update Worker`: gateway yazilimi ve firmware dagitimi
- `Rule Engine`: internet olmadan calisabilecek basit kural motoru

### 2.3 Cloud Katmani

Bulut katmani ilk gunden zorunlu olmamali, ama mimari ona acik olmali.

Sorumluluklari:

- cihaz/tesis envanteri
- uzaktan izleme paneli
- alarm dagitimi (`push`, `SMS`, `e-mail`, `WhatsApp` benzeri kanallar)
- tarihsel veri depolama
- raporlama
- AI cikarim servisleri
- kullanici, rol ve yetki yonetimi

### 2.4 Mobil Arayuz Katmani

Mobil uygulama iki farkli persona icin dusunulmelidir:

- operator: "su an ne oluyor, alarm var mi, manuel mudahale gerekli mi?"
- teknisyen: "kurulum, servis, kalibrasyon, log, test ve konfig"

Ilk surum icin mobil uygulama su ekranlarla sinirli kalmalidir:

- canli durum ozeti
- aktif parsel / kalan sure
- pH / EC / alarm durumu
- manuel baslat / durdur
- fault detay ve onay
- son olaylar
- program veya profil secimi

## 3. Katmanlar Arasi Iletisim

### 3.1 `STM32 <-> Gateway`

Ilk faz icin en pratik secenek:

- `USB CDC` veya `UART`

Saha dayanikliligi daha yuksek varyant:

- `RS485` fiziksel katman
- ustte `Modbus RTU` veya sade ikili protokol

Onerilen yol:

1. mevcut `USB CDC` protokolunu servis/veri komutlari icin genislet
2. veri modelini tasarla
3. saha urunu icin ayni veri modelini `RS485` tasiyacak sekilde soyutla

Gateway tarafinda gerekli mesaj siniflari:

- `heartbeat`
- `telemetry`
- `fault_event`
- `alarm_event`
- `config_get`
- `config_set`
- `manual_command`
- `program_sync`
- `firmware_info`

### 3.2 `Gateway <-> Cloud`

Onerilen protokoller:

- surekli durum ve olay akisi icin `MQTT`
- dosya, rapor ve yonetim API'leri icin `HTTPS`

Onerilen kurallar:

- internet yoksa veri yerelde kuyruklanir
- manuel kritik komutlar bulut onayi beklemeden yerelde islenebilir
- buluttan gelen komutlar rol bazli yetki ve zaman damgasi ile dogrulanir

### 3.3 `Mobile <-> Gateway/Cloud`

Iki mod desteklenmelidir:

- `local-first`: teknisyen yerel agda gateway'e dogrudan baglanir
- `remote`: internet uzerinden bulut araciligiyla baglanir

Bu sayede kurulum ve servis icin saha internetine tam bagimli kalinmaz.

## 4. Onerilen Veri Akisi

Normal isleyis:

1. `STM32` anlik durum ve olaylari gateway'e yollar
2. `Gateway` bu veriyi normalize eder ve tamponlar
3. mobil uygulama yerel API veya bulut uzerinden durumu gorur
4. internet varsa bulut tarihsel veri ve alarm dagitimi yapar
5. `AI` katmani veri uzerinde cikarim yapar ve oneriler uretir

Kritik kontrol kurali:

- vana acma/kapama, acil stop ve fault gibi hareketler her zaman `STM32` tarafinda son karar katmanindan gecmelidir
- `AI` veya mobil arayuzun dogrudan "ham vana surme" yetkisi olmamalidir

## 5. AI Katmanini Nereye Koymali?

`AI` ilk asamada "otonom kontrol" icin degil, asagidaki isler icin kullanilmalidir:

- alarm ozetleme
- operatore sade dilde durum aciklama
- anomali tespiti
- sulama/periyot optimizasyonu icin tavsiye
- bakim tahmini
- kalibrasyon sapmasi fark etme
- tarihsel veri uzerinden "hangi parsel neden daha cok su istiyor?" analizi

Ilk guvenli AI kullanimlari:

- "Son 24 saatte 3 kez dusuk akis alarmi olustu"
- "pH duzeltmesi normalden %20 daha uzun suruyor"
- "3 nolu parselde beklenen akis ile olculen akis arasinda fark var"
- "Bu hafta filtre veya vana kontrolu gerekebilir"

Ilk fazda AI'nin yapmamasi gerekenler:

- dogrudan sulama programi yazmak
- operator onayi olmadan kritik setpoint degistirmek
- fault varken zorla sistemi yeniden baslatmak
- sensor sagligini dogrulamadan otonom dozaj karari vermek

## 6. Mobil Uygulama Kapsami

Minimum uygulanabilir mobil kapsami:

- canli dashboard
- parceller listesi ve anlik durum
- fault/alarm ekrani
- manuel kontrol
- program/profil atama
- temel ayarlar
- cihaz baglanti ve senkron durum gosterimi

Ikinci asama mobil kapsami:

- olay gecmisi
- servis modu
- kalibrasyon sihirbazi
- kullanici/rol bazli yetki
- uzaktan firmware guncelleme takibi

Teknik tercih:

- tek kod tabani icin `Flutter` mantikli bir baslangictir
- ekip web agirlikli ise `React Native` da tercih edilebilir

Bu repo baglaminda karar kriteri:

- hizli prototip ve saha testi icin `Flutter`
- mevcut web stack ile paylasim agir basiyorsa `React Native`

## 7. Guvenlik ve Emniyet Kurallari

Bu fazda en kritik konu siber guvenlikten once operasyonel emniyettir. Ikisi birlikte dusunulmelidir.

Zorunlu kurallar:

- `STM32` internetten dogrudan erisilebilir olmamali
- tum uzaktan komutlar gateway uzerinden gecmeli
- rol bazli yetkilendirme olmadan manuel role komutu acilmamali
- fault durumunda uzaktan yeniden baslatma kisitli olmali
- internet kesintisinde saha kontrolu devam etmeli
- saat senkronu ve olay zaman damgasi tutulmali
- her komut audit log'a yazilmali

## 8. Fazli Uygulama Onerisi

### Faz 4A - Yerel Gateway MVP

Hedef:

- `STM32` yanina bir `Raspberry Pi` baglayip yerel izleme ve servis katmani acmak

Teslimatlar:

- USB/UART ile cihaz baglantisi
- gateway `device agent`
- yerel REST API
- temel web veya mobil prototip
- olay ve durum loglama
- fault bildirimleri

Basari olcutu:

- internet olmadan ayni sahada telefondan cihaz durumu gorulebilmeli

### Faz 4B - Internet ve Uzak Izleme

Hedef:

- gateway uzerinden uzaktan erisim ve raporlama

Teslimatlar:

- bulut senkronizasyonu
- cihaz kaydi ve kullanici oturumu
- alarm push bildirimi
- cihaz durum gecmisi
- uzaktan profil/program guncelleme

Basari olcutu:

- birden fazla saha cihazi tek panelden izlenebilmeli

### Faz 4C - AI Destekli Operasyon

Hedef:

- cihazin davranisini veri ile aciklayan ve iyilestirme onerileri sunan katmani acmak

Teslimatlar:

- anomaly scoring
- alarm ozetleme
- bakim tahmini
- operator icin dogal dil aciklama
- tavsiye motoru

Basari olcutu:

- AI, operatorun karar kalitesini artirmali; kontrol emniyetini zayiflatmamali

## 9. Bu Repo Icin Somut Teknik Isler

Firmware tarafinda:

- cihaz durum paketi ve olay paketi veri modelini tanimla
- USB servis protokolunu telemetri/fault/log aktarimini kapsayacak sekilde genislet
- `device_id`, `firmware_version`, `hardware_revision` alanlarini netlestir
- fault/event log yapisini standartlastir
- uzaktan yazilabilir ayarlari "guvenli" ve "kritik" diye ayir

Gateway tarafinda:

- `gateway/` altinda ayri servis klasoru ac
- `Python` veya `Go` ile `device agent` baslat
- lokal `MQTT` veya hafif kuyruk yapisi kur
- REST API ve websocket olay akisi ekle
- internet kesintisi icin disk tabanli kuyruk ekle

Mobil tarafinda:

- canli durum dashboard prototipi hazirla
- manuel komut akisini rol bazli tasarla
- alarm detay ve olay gecmisi ekranlarini cikar
- yerel ag ve uzak baglanti modlarini ayir

AI tarafinda:

- ilk veri seti olarak fault, pH, EC, aktif parsel, dozaj sureleri ve akis olaylarini topla
- AI ciktilarini `advice` ve `alert insight` siniflarina ayir
- AI tavsiyelerini operator onayli akisa bagla

## 10. Net Mimari Karar

Bu urunde en saglikli sonraki adim su olmalidir:

- `STM32` = emniyetli kontrol cekirdegi
- `Raspberry Pi` = baglanti ve entegrasyon katmani
- `Mobile` = operator/servis arayuzu
- `Cloud` = filo, rapor ve uzaktan operasyon
- `AI` = tani, ozetleme ve optimizasyon

Boylece urun, sahada calisan guvenilir cekirdegini korurken bagli ve akilli bir platforma evrilebilir.
