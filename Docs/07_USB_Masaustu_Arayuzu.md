# USB Masaustu Arayuzu

Bu arac, karttaki USB CDC konfigurasyon portu uzerinden sulama program
parametrelerini okumak ve yazmak icin hazirlandi.

## Dosya

- Arayuz uygulamasi: [tools/usb_programmer_gui.py](/Users/oktaycit/Projeler/irrigation/tools/usb_programmer_gui.py)

## Baslatma

```bash
python3 tools/usb_programmer_gui.py
```

## Temel Kullanim

1. USB port listesinden kartin CDC portunu secin.
2. `Connect` ile baglantiyi dogrulayin.
3. `Read All` ile cihazdaki programlari cekin.
4. Soldan bir program secin.
5. Sag tarafta alanlari duzenleyin.
6. `Save Selected` ile karta yazin.

## Alanlar

- `Start`, `End`: `HHMM` veya `HH:MM`
- `Valve Mask`: aktif vanalari checkbox ile olusturur
- `Days Mask`: haftanin gunlerini checkbox ile olusturur
- `pH x100`: ornek `6.50 -> 650`
- `EC x100`: ornek `1.80 -> 180`

## Hata Ayiklama

GUI acilmadan sadece seri protokolu hizli test etmek icin:

```bash
python3 tools/usb_programmer_gui.py --port /dev/cu.usbmodem207E377C52321 --ping
python3 tools/usb_programmer_gui.py --port /dev/cu.usbmodem207E377C52321 --list
```

## Notlar

- Arac harici Python kutuphanesi istemez; `tkinter` ve POSIX seri ayarlari ile calisir.
- macOS uzerinde tipik port adi `/dev/cu.usbmodem*` olur.
- Program yazma islemi mevcut firmware'deki `SET` komutunu kullanir ve EEPROM'a kalici olarak kaydeder.
