# How to run

- Connect all 3 devices (the pub/server, sub/client and broker) to the same network
- Change directory to where the mos2 folder is in, make sure mosquitto.exe is present
- run ``` ./mosquitto -c mosquitto.conf -v ```
- Check the IP address of the broker and update the code for mqtt_sub and mqtt_sub (line 23 ``` const char* mqtt_server = "192.168.221.8";```) and reflash the m5sticks
