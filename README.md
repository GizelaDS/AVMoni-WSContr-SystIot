# AVMoni-WSContr-SystIot

This project uses a set of wireless nodes (NodeMCU) to monitor the ambient temperature and humidity, light intensity and soil moisture, in addition to the water level in the reservoir or tank. Likewise, the system allows controlling the supply of water or irrigation, manually or automatically with thresholds configurable by the user from the graphical interface. The MQTT protocol (RPi) is used for communication, both locally (Node-RED) and remotely (ThingSpeak) for long-term data storage.

[Here][Video] you can find a video demonstrating how the prototype works.

Also, you can see the data of the stored variables [here][ThingSpeak]

[Video]: https://youtu.be/4u-_d6KiZlE
[ThingSpeak]: https://thingspeak.com/channels/1314546

## Notes
- Use your network's IP addresses.
- Use your network's ssid and password.
- Create and use your own certificates/keys for TLS.
- Create your channel in ThingSpeak, then use your own WriteAPIKey and FieldIDs to store variable data.
