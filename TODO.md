# Irrigation Project TODO

Durum ozeti: Proje derlenebilir firmware prototipi asamasinda. Hedef, 2026-03-23 sonuna kadar temel islevleri kart uzerinde dogrulanmis bir release candidate cikarmak.

## Bu Hafta

### 2026-03-17 - Stabilizasyon
- [ ] Derleme warning'lerini gozden gecir ve kritik olanlari kapat
- [ ] Mevcut placeholder alanlari backlog olarak netlestir
- [ ] Ana modullerin acilis sirasini ve hata akislarini kontrol et

### 2026-03-18 - GUI Temel Akislari
- [ ] Ana ekrani gercek durum verileri ile netlestir
- [ ] Manuel vana kontrol ekranini tamamla
- [ ] Ayarlar ekranini temel parametreler icin calisir hale getir
- [ ] Kalibrasyon giris ekranlarini ekle
- [ ] Placeholder ilerleme cubugunu gercek hesap ile degistir

### 2026-03-19 - Sulama Algoritmasi
- [ ] `HAL_Delay()` kullanan dozaj akislarini non-blocking state machine yapisina tasi
- [ ] pH baz duzeltme akisini ekle
- [ ] Karistirma ve bekleme sureclerini netlestir
- [ ] Hata durumlarinda guvenli kapanis davranisini tamamla

### 2026-03-20 - EEPROM ve Kalicilik
- [ ] Sistem parametreleri icin CRC kontrolunu netlestir
- [ ] Versiyon uyumlulugu ve varsayilan deger yukleme akisini tamamla
- [ ] GUI uzerinden degisen ayarlari EEPROM'a kaydet
- [ ] Yeniden baslatma sonrasi ayar koruma testini yap

### 2026-03-21 - Donanim Entegrasyon Testi
- [ ] LCD baslatma ve ekran cizimlerini kart uzerinde dogrula
- [ ] Touch koordinatlari ve tus alanlarini test et
- [ ] EEPROM okuma/yazma testini yap
- [ ] Vana cikislarini tek tek dogrula
- [ ] pH ve EC sensor okumalarini gercek donanimda test et

### 2026-03-22 - Guvenlik ve Hata Yonetimi
- [ ] Sensor timeout ve out-of-range durumlarini test et
- [ ] Emergency stop akisini dogrula
- [ ] Watchdog stratejisini netlestir ve entegrasyonunu kontrol et
- [ ] Global hata kodlarini ve durum gostergelerini duzenle

### 2026-03-23 - Kabul Testi ve RC
- [ ] Soguk acilis testini yap
- [ ] Manuel kontrol senaryolarini test et
- [ ] Otomatik modda en az 1 tam parsel dongusunu tamamla
- [ ] Reboot sonrasi ayar korumayi tekrar test et
- [ ] Release candidate notlarini hazirla

## Oncelik Sirasi

### P1
- [ ] Sulama algoritmasini non-blocking hale getir
- [ ] Emergency stop ve hata state gecislerini tek bir tutarli akis haline getir
- [ ] Parcel queue / siradaki parsel akisini tamamla ve otomatik modda en az 1 tam donguyu calistir
- [ ] GUI temel akislarini tamamla
- [ ] Kart uzerinde temel donanim dogrulamasini yap

### P2
- [ ] EEPROM kaliciligini saglamlastir
- [ ] Hata yonetimi ve guvenli kapanis akislarini tamamla
- [ ] Sensor hata senaryolarini test et
- [ ] Touch kalibrasyonunu EEPROM ile yukle/kaydet akisi ile tamamla
- [ ] README ve TODO durumunu senkron tut

### P3
- [ ] Font ve gorsel iyilestirmeleri yap
- [ ] Gecmis/log ekranlarini ekle
- [ ] Kod temizligi ve dokumantasyon iyilestirmeleri yap

## Cikis Kriterleri

- [ ] Cihaz acilisinda ana ekran stabil gelmeli
- [ ] Sensor degerleri ekranda anlamli sekilde gorunmeli
- [ ] Manuel modda vana ac/kapat calismali
- [ ] Otomatik modda en az 1 parsel dongusu tamamlanmali
- [ ] Ayarlar yeniden baslatma sonrasi korunmali
- [ ] Kritik hata durumunda tum vanalar guvenli sekilde kapanmali

## Teknik Borclar

- [ ] Placeholder font tablolarini gercek font verileri ile degistir
- [ ] GUI cizimlerini tekrarli full redraw yerine daha kontrollu guncelle
- [ ] Sensor katmaninda kullanilmayan fonksiyon ve warning'leri temizle
- [ ] `.ioc` dosyasini repoya ekleme kararini netlestir
- [ ] `CTRL_STATE_ERROR` ve `CTRL_STATE_EMERGENCY_STOP` akislarini teklestir
- [ ] `IRRIGATION_CTRL_AddToQueue` / queue doldurma ve bosaltma akislarini netlestir
- [ ] EEPROM load sirasinda CRC ve versiyon dogrulamasini zorunlu hale getir
- [ ] Touch kalibrasyon stub fonksiyonlarini gercek implementasyona cevir
