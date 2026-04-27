# Core Kapanis Checklist

Bu not, `Core` firmware katmaninin 2026-04-26 itibariyla yazilim tarafinda hangi noktada kapandigini ve sahada hangi maddelerin kabul testi olarak kaldigini ozetler.

## Dogrulanan Yazilim Kapsami

- [x] Temiz derleme alindi: `make clean && make -j4`
- [x] Firmware artefaktlari yeniden uretildi: `stm32/Debug/irrigation.elf`, `stm32/Debug/irrigation.hex`, `stm32/Debug/irrigation.bin`
- [x] Derleme cikisinda compiler warning gorulmedi
- [x] `dosing_controller`, `parcel_scheduler`, `fault_manager`, `irrigation_runtime`, `irrigation_persistence`, `irrigation_schedule_trigger` ve `irrigation_run_session` build icinde
- [x] Dozaj akisi blocking bekleme yerine state-machine akisi ile ilerliyor
- [x] Sulama fazlari `pre-flush`, `dosing`, `post-flush` olarak ayrildi
- [x] Runtime backup, restore guard ve alarm reset akislari firmware tarafinda tekillestirildi
- [x] Sistem parametreleri icin CRC / versiyon kontrolu ve varsayilan fallback akisi eklendi
- [x] GUI uzerinden degisen temel parametreler kaliciliga yaziliyor
- [x] pH/EC hedef dozlama icin `LINEAR` ve `FUZZY` duty secimi Params menusunden yapilabiliyor
- [x] Basit mod icin pencere, periyodik ve gundogumu ankrajli tetikleme modeli tanimli

## Build Ozeti

Son temiz build:

```text
Memory region         Used Size  Region Size  %age Used
CCMRAM:                   0 B        64 KB      0.00%
RAM:                  14208 B       128 KB     10.84%
FLASH:               140444 B       512 KB     26.79%

text/data/bss/dec: 139884 / 552 / 13656 / 154092
```

2026-04-27 Insight gateway protokol yukunden sonra:

```text
Memory region         Used Size  Region Size  %age Used
CCMRAM:                   0 B        64 KB      0.00%
RAM:                  14208 B       128 KB     10.84%
FLASH:               141784 B       512 KB     27.04%

text/data/bss/dec: 141224 / 552 / 13656 / 155432
```

## Core Kapanis Karari

`Core` firmware, yazilim mimarisi ve derlenebilirlik acisindan kapatilabilir durumda. Ticari veya saha kabul anlaminda kapanis icin kart uzerindeki fiziksel dogrulamalar tamamlanmali.

## Sahada Kalan Kabul Maddeleri

- [ ] Aktif sulama sirasinda runtime backup ve reboot restore akisini kart uzerinde dogrula
- [ ] Cihaz soguk acilista ana ekrana stabil dusuyor mu dogrula
- [ ] Vana cikislarini tek tek fiziksel yuk ile dogrula
- [ ] Touch alanlari ve koordinat kaymasi testini yap
- [ ] pH/EC sensor okumalarini bagli ve bagli degil senaryolarinda dogrula
- [ ] Otomatik modda en az 1 tam parsel dongusunu kart uzerinde tamamla
- [ ] Sensor hata, emergency stop ve kritik hata durumunda tum vanalarin guvenli kapandigini dogrula
- [x] USB CDC `PING` cevapsizligini USB/protokol tarafinda ayri incele

## Sonraki Kisa Rota

1. Kart ustu kabul testlerini `Docs/04_Test_Checklist.md` uzerinden isaretle.
2. Gecen testleri `TODO.md` cikis kriterlerine yansit.
3. Fiziksel kabulden sonra `Core v0.1` etiketi veya release notu hazirla.
