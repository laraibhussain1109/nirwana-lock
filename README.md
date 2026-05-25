# TrueCollabs Smart Gate Cloud

This repo now contains:
1. A Django + Django REST Framework backend.
2. A frontend dashboard (`/`) to control gate operations.
3. ESP32 sketch cloud polling so commands can be sent from `truecollabs.pythonanywhere.com`.

## Run locally

```bash
python -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
python manage.py migrate
python manage.py createsuperuser
python manage.py runserver
```

## PythonAnywhere notes

- Set WSGI file to point to `gatecloud.wsgi.application`.
- Ensure `ALLOWED_HOSTS` includes `truecollabs.pythonanywhere.com`.
- Run `python manage.py migrate` in PythonAnywhere bash console.
- Create one `Device` in Django Admin with:
  - `device_id`: e.g. `esp32-gate-01`
  - `token`: a strong shared secret

## ESP32 integration

In `sketch.ino` set:
- `cloudHost = "https://truecollabs.pythonanywhere.com"`
- `cloudDeviceId` to your Django device id
- `cloudToken` to the same token saved in Django Admin

ESP32 will:
- Poll `/api/cloud/next-command/` every 2 seconds.
- POST status to `/api/cloud/heartbeat/` every 5 seconds.


## ESP32 Servo test sketch (for OPEN/CLOSE validation)

Use `esp32_servo_cloud_test.ino` to test only cloud command reception with a servo motor on **D18 / GPIO18**.

### Steps
1. In Django admin, create a `Device`:
   - `device_id = esp32-servo-test-01`
   - `token = CHANGE_ME_TOKEN` (or update both sides to your own value)
2. Edit WiFi credentials and token in `esp32_servo_cloud_test.ino`.
3. Flash the sketch to ESP32.
4. Open Serial Monitor at 115200 baud.
5. From dashboard, queue `OPEN` and `CLOSE` commands.
6. Verify serial logs and servo movement.
