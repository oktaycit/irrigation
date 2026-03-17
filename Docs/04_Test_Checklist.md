# STM32 Sulama Sistemi Test Checklist

Bu dokuman, kart uzerinde yapilacak temel dogrulama ve kabul testlerini takip etmek icin hazirlanmistir.

## 1. Derleme ve Programlama Oncesi

- [ ] Proje warning'siz derleniyor
- [ ] Uretilen `.elf` dosyasi olusuyor
- [ ] Linker script dogru hedefi kullaniyor (`STM32F407VETX_FLASH.ld`)
- [ ] MCU'ya flash islemi hatasiz tamamlandi
- [ ] Kart yeniden baslatildiginda firmware aciliyor

## 2. Soguk Acilis ve Temel Baslangic

- [ ] Cihaz soguk acilista kilitlenmeden basliyor
- [ ] LCD acilisindan sonra ekran goruntu veriyor
- [ ] Splash ekranindan ana ekrana gecis oluyor
- [ ] Sistem ana ekrana dusebiliyor
- [ ] Acilista tum vanalar kapali durumda kaliyor
- [ ] EEPROM takili degilse veya okunamiyorsa sistem davranisi gozlenip not edildi

## 3. LCD ve GUI

- [ ] Ekranda pH ve EC alanlari gorunuyor
- [ ] Ana ekranda metinler okunakli
- [ ] Ekran yonu dogru
- [ ] Ekranda ciddi flicker veya bozulma yok
- [ ] Ana ekran periyodik olarak guncelleniyor
- [ ] Sistem beklemede iken durum mesaji dogru gorunuyor
- [ ] Sulama aktifken parsel bilgisi dogru gorunuyor
- [ ] Kalan sure alani mantikli degisiyor
- [ ] Ilerleme cubugu mevcut surec ile uyumlu gorunuyor

## 4. Touch ve Menu Akislari

- [ ] Touch basmalari tutarli algilaniyor
- [ ] Buton alanlari dogru yerlere tepki veriyor
- [ ] Yanlis koordinat veya kayma problemi yok
- [ ] Ana ekran -> Manuel ekran gecisi calisiyor
- [ ] Ana ekran -> Ayarlar ekran gecisi calisiyor
- [ ] Uzun kullanimda touch takili kalmiyor

## 5. Sensor Okumalari

- [ ] pH sensor verisi okunuyor
- [ ] EC sensor verisi okunuyor
- [ ] Sensor verileri ekranda anlamli aralikta gorunuyor
- [ ] Bagli olmayan sensor durumunda hata davranisi gozlendi
- [ ] Timeout durumunda sistem hata uretiyor
- [ ] Out-of-range durumda sistem hata uretiyor
- [ ] Sensor baglanti geri geldiginde toparlanma davranisi not edildi

## 6. Manuel Vana Kontrolu

- [ ] Her vana tek tek acilabiliyor
- [ ] Her vana tek tek kapatilabiliyor
- [ ] Yanlis vana tetiklenmiyor
- [ ] Bir vana kapatildiginda fiziksel cikis kapanıyor
- [ ] Manuel kullanimdan sonra sistem kararsiz duruma gecmiyor

## 7. Otomatik Sulama Akisi

- [ ] En az bir aktif parsel queue'ya alinabiliyor
- [ ] Otomatik sulama baslatilabiliyor
- [ ] Sensor kontrolu sonrasi sulama durumuna geciliyor
- [ ] Gecerli parsel vanasi aciliyor
- [ ] Parsel suresi tamamlaninca vana kapaniyor
- [ ] Queue'da siradaki parsel varsa bir sonrakine geciliyor
- [ ] Disabled parseller atlanıyor
- [ ] Ayni parsel queue'ya tekrar tekrar eklenmiyor
- [ ] Son parsel tamamlaninca sistem `IDLE` durumuna donuyor

## 8. pH / EC Dozaj ve Non-Blocking Davranis

- [ ] pH hedef disindayken uygun dozaj akisi basliyor
- [ ] EC hedef disindayken uygun dozaj akisi basliyor
- [ ] Dozaj sirasinda sistem tamamen donmuyor
- [ ] Dozaj sirasinda GUI guncellemesi devam ediyor
- [ ] Dozaj tamamlaninca vana kapaniyor
- [ ] Karistirma suresi tamamlaninca settling asamasina geciliyor
- [ ] Settling sonrasi sensorler yeniden kontrol ediliyor
- [ ] pH dusuk oldugunda baz duzeltme davranisi test edildi veya eksik olarak not edildi

## 9. Hata Yonetimi ve Emergency Stop

- [ ] Sensor hatasi durumunda sistem `ERROR` davranisina geciyor
- [ ] Error durumunda aktif dozaj vanasi kapaniyor
- [ ] Error durumunda queue temizleniyor
- [ ] Error durumunda tum vanalar guvenli sekilde kapaniyor
- [ ] Emergency stop tetiklenince tum vanalar kapaniyor
- [ ] Emergency stop sonrasi sistem beklenen state'te kaliyor
- [ ] Error/Emergency sonrasi yeniden baslatma veya tekrar calistirma davranisi dogrulandi

## 10. EEPROM ve Kalicilik

- [ ] Ilk acilista EEPROM formatlama davranisi dogru
- [ ] Parametreler EEPROM'a yazilabiliyor
- [ ] Yeniden baslatma sonrasi parametreler korunuyor
- [ ] Bozuk veri durumunda varsayilan degerlere donus davranisi test edildi
- [ ] CRC kontrolu beklendigi gibi calisiyor veya eksikligi not edildi
- [ ] Versiyon uyumsuzlugu davranisi test edildi veya eksikligi not edildi

## 11. Dayaniklilik ve Tekrar Testleri

- [ ] Ardisik 10 yeniden baslatmada sistem aciliyor
- [ ] Uzun sure acik kaldiginda sistem takilmiyor
- [ ] Ardisik manuel ac/kapat denemelerinde beklenmeyen durum olusmuyor
- [ ] Ardisik otomatik dongulerde state gecisleri bozulmuyor
- [ ] Sensor iletisimi kesilip geri geldiginde sistem toparlaniyor

## 12. Cikis Kriterleri

- [ ] Cihaz acilisinda ana ekran stabil geliyor
- [ ] Sensor degerleri ekranda anlamli sekilde gorunuyor
- [ ] Manuel modda vana ac/kapat calisiyor
- [ ] Otomatik modda en az 1 tam parsel dongusu tamamlandi
- [ ] Ayarlar yeniden baslatma sonrasi korundu
- [ ] Kritik hata durumunda tum vanalar guvenli sekilde kapandi

## 13. Test Notlari

- Test tarihi:
- Test eden:
- Kart revizyonu:
- Sensor tipi:
- Firmware build:
- Notlar:
