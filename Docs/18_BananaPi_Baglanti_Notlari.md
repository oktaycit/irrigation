# Banana Pi Baglanti Notlari

Bu dosya, kart ustu testlerde Banana Pi uzerinden ayni baglanti
parametrelerinin tekrar bulunmasi icin tutulur.

## SSH

- Hostname: `serapi`
- IP: `192.168.1.190`
- Kullanici: `sera`
- Parola: `sera`
- SSH komutu: `ssh sera@192.168.1.190`

## STM32 ve DAPLink Portlari

- STM32 USB CDC:
  `/dev/serial/by-id/usb-OktayCit_Irrigation_Controller_Config_Port_207E377C5232-if00`
- Tipik STM32 tty: `/dev/ttyACM1`
- DAPLink CMSIS-DAP:
  `/dev/serial/by-id/usb-ARM_DAPLink_CMSIS-DAP_07000001000000000000000000000000a5a5a5a597969908-if01`
- Tipik DAPLink tty: `/dev/ttyACM0`

## Gateway

- Servis/uygulama dizini: `/home/sera/irrigation-gateway`
- Calisan komut:
  `/usr/bin/python3 /home/sera/irrigation-gateway/app.py --host 0.0.0.0 --port 8080`
- Yerel ag health endpoint: `http://192.168.1.190:8080/health`
- Ping endpoint: `http://192.168.1.190:8080/api/ping`

## Flash / Verify Akisi

Banana Pi uzerinde OpenOCD 0.12.0 kurulu. DAPLink mass-storage kopyalama
`FAIL.TXT` icinde `The transfer timed out` verdigi icin firmware yukleme icin
SWD/OpenOCD yolu kullanilir.

1. Yerelde build al:

   ```bash
   make -j4
   ```

2. Firmware'i Banana Pi'ye kopyala:

   ```bash
   scp stm32/Debug/irrigation.elf stm32/Debug/irrigation.hex sera@192.168.1.190:/home/sera/
   ```

3. Banana Pi uzerinden flash/verify/reset:

   ```bash
   ssh sera@192.168.1.190 \
     'echo sera | sudo -S openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "program /home/sera/irrigation.elf verify reset exit"'
   ```

4. Flash sonrasi Banana Pi USB hub'ini yeniden enumerate et. Bu adimdan sonra
   STM32 CDC portu temiz sekilde geri geliyor:

   ```bash
   ssh sera@192.168.1.190 \
     'echo sera | sudo -S sh -c "echo 0 > /sys/bus/usb/devices/5-1/authorized"; sleep 5; echo sera | sudo -S sh -c "echo 1 > /sys/bus/usb/devices/5-1/authorized"; sleep 20'
   ```

5. Hatti dogrula:

   ```bash
   curl -fsS --max-time 5 http://192.168.1.190:8080/api/ping
   curl -fsS --max-time 8 http://192.168.1.190:8080/api/programs
   ```

## Dikkat

- Program 1 test sirasinda gecici devre disi birakilabilir; test sonunda eski
  etkin durumuna geri alinmalidir.
- Fiziksel vana, touch, sensor ve runtime restore kabul testleri hala ayrica
  sahada dogrulanmalidir.
