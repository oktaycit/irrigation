# USB Programlama Protokolu

Bu katman, sulama programlarini TFT ekrandan tek tek girmek yerine PC tarafindan
USB uzerinden yuklemek icin hazirlandi. Mevcut repoda USB CDC tasiyicisi henuz
ekli degildir; ancak program import/export ve dogrulama mantigi hazirdir.

## Hedef Mimari

- Fiziksel baglanti: STM32F407 `PA11/USB_DM` ve `PA12/USB_DP`
- Tasiyici: USB CDC (Virtual COM Port)
- Uygulama katmani: `Core/Src/usb_config.c`
- Kalicilik: `IRRIGATION_CTRL_SetProgram()` + EEPROM kaydi

## Desteklenen Komutlar

Her komut satir sonu ile bitmelidir: `\r\n`

### 1. Baglanti testi

```text
PING
```

Yaniti:

```text
OK,PONG
```

### 2. Tum programlari oku

```text
LIST
```

Ornek yanit:

```text
OK,LIST,BEGIN
PROGRAM,1,1,0600,0700,1,5,1,1,127,650,180
PROGRAM,2,0,0610,0710,2,5,1,1,127,650,180
OK,LIST,END
```

Alanlar:

```text
PROGRAM,<id>,<enabled>,<start_hhmm>,<end_hhmm>,<valve_mask>,<irrigation_min>,<wait_min>,<repeat_count>,<days_mask>,<ph_x100>,<ec_x100>
```

### 3. Tek program oku

```text
GET,1
```

Yaniti:

```text
OK,PROGRAM,1,1,0600,0700,1,5,1,1,127,650,180
```

### 4. Program yaz

```text
SET,1,1,0630,0730,3,12,2,1,62,640,175
```

Yaniti:

```text
OK,SET,1,1,0630,0730,3,12,2,1,62,640,175
```

## Alan Kurallari

- `id`: `1..8`
- `enabled`: `0` veya `1`
- `start_hhmm`, `end_hhmm`: `0000..2359`
- `valve_mask`: `1..255`, sadece mevcut vana bitleri kullanilmali
- `irrigation_min`: `1..999`
- `wait_min`: `0..999`
- `repeat_count`: `1..99`
- `days_mask`: `1..127`
- `ph_x100`: `0..1400`
- `ec_x100`: `0..2000`

## USB CDC Entegrasyon Notu

USB CDC eklendiginde cihaz tarafinda yapilacak baglama cok dar olacaktir:

1. USB RX callback icinde gelen baytlar `USB_CONFIG_FeedRx()` fonksiyonuna aktarilacak.
2. `USB_CONFIG_TransportWrite()` CDC transmit fonksiyonuna baglanacak.
3. Istenirse polling yerine tamamen callback tabanli kullanilacak.

Bu sayede TFT yalnizca durum gostermek icin kullanilabilir; programlama isi PC
uygulamasi, terminal veya basit bir konfig araci tarafindan yapilir.
