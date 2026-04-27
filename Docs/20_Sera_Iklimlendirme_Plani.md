# Insight Sera Iklimlendirme Plani

Bu not, `Insight` surumunde sulama/fertigation katmanina sera iklimlendirme yetenegi eklemek icin sensor, cikis, port cogaltma ve yazilim mimarisini tanimlar.

## 1. Net Karar

`Insight` sera paketi, sulama kontrolcusunu sera iklim asistani ve iklim kontrol gateway'i haline getirmelidir.

Ana kararlar:

- STM32 emniyetli saha cikislarini ve kritik interlocklari yonetmeye devam eder.
- Linux gateway; hava, urun donemi, kamera, SCD41/DS18B20 ve internet verisini birlestiren iklim karar katmani olur.
- SCD41, sera ic ortam olcumu icin ana adaydir: CO2, nem ve sicaklik ayni I2C sensorunden gelir.
- DS18B20/1-Wire, sicaklik-only yardimci hat olarak kullanilir: su deposu, besleme hatti, dis ortam veya yedek sicaklik.
- Ruzgar hiz ve yonu icin birinci tercih RS485/Modbus ultrasonik ruzgar sensorudur.
- PCF8574/PCF8575 gibi I2C port expander'lar yakin pano icindeki dusuk hizli role/input cogaltma icin uygundur.
- Modbus/RS485, uzun kablo, farkli pano veya hazir endustriyel iklim modul baglantisi icin tercih edilmelidir.

## 2. Mevcut Donanimla Yapilabilecekler

Mevcut STM32 + gateway mimarisiyle ek iklim karti olmadan baslanabilecekler:

- internet hava/tarimsal verisinden dis iklim context'i almak
- kamera ile bitki/zemin gozlemi yapmak
- mevcut I2C hattina uygun sekilde SCD41 takarak ic sicaklik/nem/CO2 okumak
- mevcut 1-Wire hattina DS18B20 veya turevlerini baglayarak sicaklik okumak
- mevcut gateway dashboardunda iklim tavsiyesi, VPD, CO2 ve sicaklik/nem takibi gostermek
- gateway'e USB-RS485 uzerinden ultrasonik ruzgar sensoru baglayarak dis ruzgar hiz/yon verisini iklim kararlarina katmak

Mevcut donanimla sinirli yapilabilecek kontrol:

- bosta veya iklim icin ayrilmis cikis varsa basit fan/sisleme/golgeleme/isitma komutu
- operator onayli iklim tavsiyesi
- sulama programini iklim context'ine gore azalt/artir/ertele tavsiyesi

Sinir:

- Mevcut vana/dozaj cikislari ticari sera iklimlendirme icin dogrudan fan, rezistans, perde motoru veya sisleme pompasini surmemelidir.
- Guvenilir iklimlendirme icin saha cikislarinda role/SSR/kontaktor, sigorta, manuel bypass ve interlock gerekir.

## 3. Sensor Secimi

### 3.1 SCD41

Kullanim rolu:

- sera ic ortam ana iklim sensoru
- CO2, sicaklik ve bagil nem
- VPD ve havalandirma/golgeleme/sisleme kararinin ana girdisi

Baglanti:

- arayuz: I2C
- onerilen hat: mevcut `PB8/PB9` I2C1
- adres cakismasi yoksa EEPROM/PCF ile ayni bus paylasilabilir
- uzun kablo icin I2C dogrudan saha kablosu gibi kullanilmamali; sensor pano icinde kalmali veya I2C buffer/diferansiyel cozum dusunulmelidir

Not:

- Sensirion SCD41, SCD4x ailesinde CO2 sensorudur ve entegre sicaklik/nem olcumu de verir.

### 3.2 DS18B20 / 1-Wire

Kullanim rolu:

- sicaklik-only olcum
- su deposu sicakligi
- besleme/karisim hatti sicakligi
- dis ortam sicakligi
- sera icinde yedek veya bolgesel sicaklik noktasi

Baglanti:

- arayuz: 1-Wire
- mevcut onerilen hat: `PB12`
- ayni hatta birden fazla DS18B20 kullanilabilir; her sensor benzersiz ROM ID ile ayrilir

Sinir:

- DS18B20 nem olcmez.
- Tek basina VPD veya sera nem kontrolu icin yeterli degildir.

### 3.3 Kamera

Kullanim rolu:

- bitki stres belirtisi
- zemin/bitki islaklik gorunumu
- golgeleme ve havalandirma kararina destek
- iklim tavsiyesinin gerekcesini guclendirme

Kamera ilk fazda dogrudan fan/isitici/perde komutu uretmemelidir; karar motoruna guven skoru ve gozlem kaniti saglamalidir.

### 3.4 Ruzgar Hiz ve Yon Sensoru

Secilen sensor sinifi:

- RS485/Modbus ultrasonik ruzgar hiz ve yon sensoru

Tercih edilen urun:

- Renke `RK120-07` veya ayni sinifta RS485/Modbus RTU destekli ultrasonik ruzgar sensoru

Premium alternatif:

- Gill Instruments `WindSonic` RS485/Modbus veya RS422/RS485 varyanti

Kullanim rolu:

- dis ruzgar hizi ve yonu
- havalandirma penceresi/perde emniyeti
- sisleme ve fan kararini dis ruzgarla iliskilendirme
- firtina/sert ruzgar alarmi
- acik alan sulama drift veya yagmurlama riski icin tavsiye

Baglanti:

- ilk prototip: gateway uzerinde USB-RS485 adapter ile Modbus RTU
- saha urunu: gateway veya iklim I/O karti uzerinde izole RS485
- STM32 uzerinden okunacaksa `PA9/PA10 + PB11 DE` hattinin debug/pH-EC Modbus roluyle cakismasi net ayrilmalidir

Neden ultrasonik:

- hareketli parca yok
- rulman/kepce bakimi yok
- hiz ve yonu tek govdede verir
- sera disinda toz, nem ve UV kosullarinda daha az servis ister

Kurulum:

- sera disinda, catidan veya konstruksiyondan etkilenmeyecek noktada
- yatay montaj ve uretici yon referansi dogru ayarlanmis sekilde
- RS485 hattinda ekranli twisted pair, sonlandirma ve surge/TVS koruma
- veri yasi ve haberlesme timeout'u climate-engine tarafinda izlenmeli

## 4. Cikis ve Port Cogaltma Mimarisi

### 4.1 PCF8574 / PCF8575

Tercih edilecek yer:

- ayni pano icindeki role cikislari
- manuel switch/limit switch gibi dusuk hizli dijital inputlar
- fan kademesi, sisleme role komutu, golgeleme role komutu, alarm lambasi gibi on/off cikislar

Avantaj:

- I2C ile az pin kullanir
- firmware'de mevcut PCF8574 deneyimi var
- ek 8 veya 16 bit I/O kolay acilir

Sinir:

- uzun saha kablosu icin uygun degil
- hizli PWM veya hassas zamanlama icin uygun degil
- cikislar mutlaka transistor/ULN/optokuplor/role surucu uzerinden saha yukune gitmelidir

### 4.2 Modbus / RS485

Tercih edilecek yer:

- uzun kablo
- sera icinde dagitik sensor/role modulleri
- hazir endustriyel sicaklik/nem/CO2 veya role modulleri
- farkli panolara dagilan fan, perde, sisleme ve isitma kontrolu

Avantaj:

- saha kablosuna daha dayanikli
- adreslenebilir modullerle genisleme kolay
- ticari sensor ve role modulleriyle uyumlu

Sinir:

- protokol ve cihaz adres yonetimi gerekir
- gateway/STM32 tarafinda net master rol secimi yapilmalidir

## 5. Onerilen Iklim Cikis Siniflari

Ilk sera iklim cikislari:

- `climate_fan_stage_1`
- `climate_fan_stage_2`
- `climate_fogging`
- `climate_shade_open`
- `climate_shade_close`
- `climate_heater`
- `climate_roof_vent_open`
- `climate_roof_vent_close`

Emniyet kurallari:

- isitici ve sisleme ayni anda otomatik calismamali veya politika ile sinirlanmali
- perde/pencere motorlari limit switch olmadan sure tabanli calisirsa maksimum sure guard'i zorunlu olmali
- fan, sisleme ve isitma icin minimum acik/kapali sureleri uygulanmali
- manuel override ve acil stop cikislari iklim cikislarini da guvenli duruma almali

## 6. Yazilim Mimarisi

Gateway tarafinda yeni servis:

`gateway/climate-engine`

Gorevleri:

- SCD41, DS18B20, internet hava verisi ve kamera gozlemlerini normalize etmek
- VPD, dew point, CO2 durum sinifi ve isi/nem stres skorunu hesaplamak
- sera/acik alan moduna gore karar kurallarini secmek
- operatora tavsiye uretmek
- limitli otomatik iklim cikis komutlarini role/izin/audit ile yonetmek

STM32 tarafinda yeni mantiksal sinif:

`climate_actuator`

Gorevleri:

- iklim cikislarini vana/dozaj cikislarindan ayri ID uzayinda tutmak
- fan/perde/sisleme/isitma cikislarinda emniyet interlock uygulamak
- gateway komutlarini sadece izinli araliklarda kabul etmek
- fault durumunda iklim cikislarini guvenli duruma almak

## 7. Veri Modeli

Gateway store icin ilk tablolar:

- `climate_sensor`: sensor id, tip, bus, adres, konum, etkinlik
- `climate_observation`: zaman, sensor, sicaklik, nem, CO2, VPD, veri kalitesi
- `wind_observation`: zaman, sensor, ruzgar_hizi, ruzgar_yonu, gust, veri_kalitesi
- `climate_zone`: sera bolgesi, hedef sicaklik/nem/CO2 araliklari, urun donemi
- `climate_actuator`: cikis id, tip, bus, kanal, guvenli durum, minimum on/off sure
- `climate_rule`: kosul, aksiyon, oncelik, otomasyon seviyesi
- `climate_event`: karar, komut, manuel override, fault ve audit kaydi

## 8. Uygulama Fazlari

### Faz C1 - Iklim Izleme

- SCD41 I2C okuma
- DS18B20/1-Wire okuma
- gateway dashboardunda sicaklik, nem, CO2, VPD
- RS485/Modbus ruzgar hiz ve yon okuma
- internet hava verisiyle ic/dis karsilastirma
- kamera gozlemini iklim kartina ekleme

### Faz C2 - Operator Tavsiyesi

- havalandirma, sisleme, golgeleme ve isitma icin tavsiye
- ruzgar hiz/yon bilgisine gore havalandirma penceresi, perde ve sisleme tavsiyesi
- urun donemi bazli hedef araliklar
- veri yasi ve sensor sagligi gostergesi
- otomatik komut yok, sadece operator onayi

### Faz C3 - Basit Otomatik Kontrol

- PCF8574/PCF8575 uzerinden role cikislari
- fan ve sisleme icin histerezis + minimum on/off sure
- golgeleme/perde icin limitli sure tabanli kontrol
- audit log ve manuel override

### Faz C4 - Dagitik Modbus Iklim

- RS485 Modbus sensor/role modulleri
- bolge bazli iklim kontrolu
- fail-safe mod ve haberlesme kopma davranisi
- uzaktan izleme ve alarm

## 9. Teknik Tercih

Ilk prototip icin tercih:

1. SCD41, I2C, pano icinde
2. DS18B20, PB12 1-Wire, yardimci sicaklik noktasi
3. PCF8574/PCF8575, yakin pano role/input cogaltma
4. Modbus/RS485, ikinci fazda dagitik sensor ve role modulleri

Ticari saha urunu icin tercih:

- SCD41 veya endustriyel Modbus CO2/sicaklik/nem sensoru
- RS485/Modbus ultrasonik ruzgar hiz/yon sensoru
- PCF sadece pano ici I/O
- fan/perde/sisleme/isitma icin izole role/SSR/kontaktor karti
- uzun mesafe ve farkli panolar icin Modbus/RS485

## 10. Kaynak Notlari

- [Sensirion SCD41 urun sayfasi](https://sensirion.com/products/catalog/SCD41)
- [Sensirion SCD4x datasheet](https://sensirion.com/media/documents/48C4B7FB/64C134E7/Sensirion_SCD4x_Datasheet.pdf)
- [Analog Devices DS18B20-PAR urun sayfasi](https://www.analog.com/en/products/ds18b20par.html)
- [Gill WindSonic ultrasonic wind sensors](https://gillinstruments.com/compare-2-axis-anemometers/windsonic-2axis/)
- [Renke RK120-07 ultrasonic wind speed and direction sensor datasheet](https://oceancontrols.com.au/files/datasheet/rks/RK120%20Datasheet.pdf)
