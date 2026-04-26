# Irrigation Project TODO

Durum ozeti: Proje calisan ve warning'siz derleme alan moduler firmware prototipi asamasinda. 2026-04-25 itibariyla `dosing_controller`, `parcel_scheduler`, `fault_manager`, `irrigation_runtime` ve `irrigation_persistence` katmanlari build icinde; `irrigation_control.c` icindeki baslatma/restore/persistence orkestrasyonu inceltildi, alarm reset akisi tekillestirildi ve reboot restore firmware guard'lari sertlestirildi. Kalan kritik is, bu restore davranisinin kart uzerinde dogrulanmasidir.

## Stratejik Referans

Bu dosya aktif uygulama listesi olarak kalir.

Segment bazli orta vade teknik backlog icin:

- [Docs/13_Teknik_Backlog.md](Docs/13_Teknik_Backlog.md)

Yol haritasi ve segment kararlari icin:

- [Docs/12_Urun_Yol_Haritasi.md](Docs/12_Urun_Yol_Haritasi.md)
- [Docs/11_Urun_Segmentasyonu.md](Docs/11_Urun_Segmentasyonu.md)
- [Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md](Docs/15_Turkiye_Pazar_Projeksiyon_Karsilastirmasi.md)

## Simdiki Sprint

### 2026-04-23 - Cekirdek Stabilizasyon
- [x] `HAL_Delay()` kullanan dozaj akislarini non-blocking state machine yapisina tasi
- [x] `dosing_controller`, `parcel_scheduler`, `fault_manager` ve runtime/persistence modullerini build icine al
- [x] Runtime backup yazarken aktif alarm bilgisini `FAULT_MGR` ile senkron tut
- [x] `irrigation_control.c` icindeki tekrar eden aktif-vana baslatma akisini `irrigation_run_session` yardimcilariyla azaltmaya basla
- [x] `irrigation_control.c` icindeki kalan scheduler / restore / persistence orkestrasyonunu daha da incelt
- [x] `CTRL_STATE_ERROR` ve `CTRL_STATE_EMERGENCY_STOP` reset akisini tekillestir
- [x] Runtime backup restore guard'larini firmware tarafinda sertlestir
- [x] DAPLink ile build flash/verify/reset smoke testini yap
- [ ] Aktif sulama sirasinda runtime backup ve reboot restore akisini kart uzerinde dogrula

### 2026-04-24 - GUI ve Operasyon Netligi
- [x] Ana ekrani gercek durum verileri ve kalan sure ile netlestir
- [x] Manuel vana kontrol ekranini tamamla
- [x] Alarm ekraninda fault / action metinlerini saha diliyle toparla
- [x] Placeholder ilerleme cubugunu gercek hesap ile degistir

### 2026-04-25 - Kalicilik ve Donanim
- [x] Sistem parametreleri icin CRC kontrolunu ve varsayilan fallback akisini sertlestir
- [x] GUI uzerinden degisen ayarlari EEPROM/flash katmanina kalici yaz
- [x] Yeniden baslatma sonrasi ayar koruma testini yap
- [ ] Vana cikislarini, touch alanlarini ve sensor okumalarini kart uzerinde dogrula
- [ ] Turkiye pazari icin ticari ilk mekanik hedefi 4+1 Venturi/Z-bypass olarak prototiple
- [ ] Kanal basina 50-600 l/sa kapasite ve 1:100 / 1:200 dozlama profillerini veri modeline ekle

## Oncelik Sirasi

### P1
- [x] Sulama algoritmasini non-blocking hale getir
- [x] Emergency stop ve hata state gecislerini tek bir tutarli akis haline getir
- [ ] Parcel queue / siradaki parsel akisini tamamla ve otomatik modda en az 1 tam donguyu calistir
- [ ] Parsel vanalari ve dozaj vanalari icin saha kontrol MVP kapsamini adim adim uygula
- [x] Sulama akisini `pre-flush`, `dosing`, `post-flush` fazlarina ayir
- [ ] `Core` segment icin minimum donanim ve minimum ayar akisini sabitle
- [ ] `Core` ticari hidrolik hedefini 4+1 Venturi/Z-bypass ve 110 mm ana hat uyumu olarak sabitle
- [x] GUI temel akislarini tamamla
- [ ] Kart uzerinde temel donanim dogrulamasini yap

### P2
- [x] EEPROM kaliciligini saglamlastir
- [ ] Hata yonetimi ve guvenli kapanis akislarini tamamla
- [ ] Sensor hata senaryolarini test et
- [ ] USB CDC `PING` cevapsizligini hedef USB/protokol tarafinda incele
- [ ] Touch kalibrasyonunu EEPROM ile yukle/kaydet akisi ile tamamla
- [ ] Program veri modelini statik saat alanlarindan `trigger_mode` ve flush parametrelerine dogru evrilt
- [ ] Program/ayar veri modeline kanal kapasitesi, dozlama orani ve nominal ana hat debisi alanlarini ekle
- [ ] Basit mod icin gun dogumu/periyot tabanli tetikleme modelini tanimla
- [ ] `Core` ve `Insight` icin feature flag veya urun seviyesi modelini tasarla
- [x] README ve TODO durumunu senkron tut

### P3
- [ ] Font ve gorsel iyilestirmeleri yap
- [ ] Gecmis/log ekranlarini ekle
- [ ] Kod temizligi ve dokumantasyon iyilestirmeleri yap
- [ ] LDR veya astronomik saat tabanli `light bucket` prototipini ekle
- [ ] Hedef hacim icin yazilimsal litre sayaci ve debi kalibrasyonunu ekle
- [ ] `Insight` segment icin sensor zengin dashboard ve tani alanlarini tasarla

## Cikis Kriterleri

- [ ] Cihaz acilisinda ana ekran stabil gelmeli
- [ ] Sensor degerleri ekranda anlamli sekilde gorunmeli
- [ ] Manuel modda vana ac/kapat calismali
- [ ] Otomatik modda en az 1 parsel dongusu tamamlanmali
- [x] Ayarlar yeniden baslatma sonrasi korunmali
- [ ] Kritik hata durumunda tum vanalar guvenli sekilde kapanmali

## Teknik Borclar

- [ ] Placeholder font tablolarini gercek font verileri ile degistir
- [ ] GUI cizimlerini tekrarli full redraw yerine daha kontrollu guncelle
- [ ] Sensor katmaninda kullanilmayan fonksiyon ve warning'leri temizle
- [x] `main.c` icindeki kullanilmayan board diagnostic/test fonksiyonlarini warning'siz build icin isaretle
- [ ] `.ioc` dosyasini repoya ekleme kararini netlestir
- [x] `CTRL_STATE_ERROR` ve `CTRL_STATE_EMERGENCY_STOP` akislarini teklestir
- [ ] `IRRIGATION_CTRL_AddToQueue` / queue doldurma ve bosaltma akislarini netlestir
- [ ] `irrigation_control.c` icindeki scheduler, dosing, restore ve persistence sorumluluklarini ayristir
- [x] EEPROM load sirasinda CRC ve versiyon dogrulamasini zorunlu hale getir
- [ ] Touch kalibrasyon stub fonksiyonlarini gercek implementasyona cevir
- [ ] `rtc_driver_stub.c` ve `hw_crc_stub.c` yerine hedef kart suruculerini bagla
- [ ] Multi-zone parsel bazli recete modeli ile global recete modelinin ayrimini netlestir
- [ ] Tek kod tabaninda iki urun seviyesini surdurmenin konfig yapisini netlestir
