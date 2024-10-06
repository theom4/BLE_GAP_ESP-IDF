General Access Profile implementation with ESP32 in ESP-IDF using the nimBLE framework

Make sure you enable the nimBLE host in menuconfig

If you want to advertise a URI instead of a custom data, follow this:

```
rsp_fields.uri = USER_URI;
rsp_fields.uri_len = strlen(USER_URI);
```
For more details explaining how the GAP service works check this video :
https://youtu.be/lpuHE5jd0Vg?si=nNI2AIxBuET8wFaU
