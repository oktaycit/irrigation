# USB Programlama Protokolu

Bu katman, sulama programlarini TFT ekrandan tek tek girmek yerine PC tarafindan
USB uzerinden yuklemek icin kullanilir. Mevcut firmware icinde USB CDC tasiyicisi
aktiftir ve komutlar `stm32/Core/Src/usb_config.c` uzerinden islenir.

## Hedef Mimari

- Fiziksel baglanti: STM32F407 `PA11/USB_DM` ve `PA12/USB_DP`
- Tasiyici: USB CDC (Virtual COM Port)
- Uygulama katmani: `stm32/Core/Src/usb_config.c`
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

### 2. Yardim komutu

```text
HELP
```

Ornek yanit:

```text
OK,HELP,PING
OK,HELP,INFO
OK,HELP,TELEM
OK,HELP,FAULT
OK,HELP,RUNTIME
OK,HELP,LIST
OK,HELP,GET,<id>
OK,HELP,SET,<id>,<enabled>,<start>,<end>,<mask>,<irr>,<wait>,<repeat>,<days>,<ph_x100>,<ec_x100>,<fert_a_pct>,<fert_b_pct>,<fert_c_pct>,<fert_d_pct>,<pre_flush_sec>,<post_flush_sec>
```

### 3. Cihaz bilgisi oku

```text
INFO
```

Yaniti:

```text
OK,DEVICE,<device_name>,<firmware_version>,<protocol_version>,<mcu>,<program_count>,<parcel_valve_count>,<dosing_valve_count>,<uptime_ms>
```

### 4. Canli telemetry snapshot oku

```text
TELEM
```

Yaniti:

```text
OK,TELEMETRY,<uptime_ms>,<control_state>,<program_state>,<is_running>,<active_program_id>,<current_parcel_id>,<remaining_sec>,<ph_x100>,<ec_x100>,<ph_status>,<ec_status>,<active_valve_mask>,<last_error>,<alarm_active>
```

### 5. Aktif fault/alarm olayi oku

```text
FAULT
```

Yaniti:

```text
OK,FAULT,<active>,<latched>,<manual_ack_required>,<acknowledged>,<last_error>,<valve_error_mask>,<recommended_state>,<alarm_text>
```

### 6. Runtime restore paketi oku

```text
RUNTIME
```

Yaniti:

```text
OK,RUNTIME,<valid>,<active_program_id>,<active_valve_index>,<repeat_index>,<program_state>,<active_valve_id>,<remaining_sec>,<ec_target_x100>,<ph_target_x100>,<error_code>
```

### 7. Tum programlari oku

```text
LIST
```

Ornek yanit:

```text
OK,LIST,BEGIN
PROGRAM,1,1,0600,0700,1,5,1,1,127,650,180,25,25,25,25,60,120
PROGRAM,2,0,0610,0710,2,5,1,1,127,650,180,25,50,20,30,60,120
OK,LIST,END
```

Alanlar:

```text
PROGRAM,<id>,<enabled>,<start_hhmm>,<end_hhmm>,<valve_mask>,<irrigation_min>,<wait_min>,<repeat_count>,<days_mask>,<ph_x100>,<ec_x100>,<fert_a_pct>,<fert_b_pct>,<fert_c_pct>,<fert_d_pct>,<pre_flush_sec>,<post_flush_sec>
```

### 8. Tek program oku

```text
GET,1
```

Yaniti:

```text
OK,PROGRAM,1,1,0600,0700,1,5,1,1,127,650,180,25,25,25,25,60,120
```

### 9. Program yaz

Mevcut firmware temel `SET` formunu kullanir:

```text
SET,1,1,0630,0730,3,12,2,1,62,640,175,25,50,20,30,60,120
```

Yaniti mevcut kaydin temel formatidir:

```text
OK,SET,1,1,0630,0730,3,12,2,1,62,640,175,25,50,20,30,60,120
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
- `fert_a_pct..fert_d_pct`: `0..100`, toplam 100 olmak zorunda degildir
- `pre_flush_sec`, `post_flush_sec`: `0..900`

## Uyumluluk Notu

- Cihaz yanitlari yeni firmware'de gubre oranlari ve flush sureleriyle doner.
- Masaustu araci, eski cihazlardan gelebilecek vana sure alanlarini iceren cevaplari
  yine de okuyabilecek sekilde geriye donuk uyumlulugu korur.
- `LIST` komutu toplu okumada tercih edilmelidir; tek tek `GET` yalnizca
  geriye donuk uyumluluk veya ariza ayiklama icin gerekir.

## Tasiyici Notu

USB CDC bridge bu repoda baglidir:

1. CDC RX paketleri halka tamponuna alinir.
2. `USB_CONFIG_Process()` bu tamponu okuyup satir bazli komut parser'ina besler.
3. `USB_CONFIG_TransportWrite()` yanitlari CDC uzerinden geri gonderir.

Bu sayede TFT yalnizca durum gostermek icin kullanilabilir; programlama isi PC
uygulamasi, terminal veya basit bir konfigurasyon araci tarafindan yapilir.
