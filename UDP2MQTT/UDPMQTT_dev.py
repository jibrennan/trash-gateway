#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  UDPMQTT.py
#  
#  Copyright 2025  <pi@raspberrypi>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  Jason Brennan: For Demo Only - Receive UDP alarm packets formatted in 
#  JSON, then send an MQTT message to the KTI-1000 broker
#  UDP: {"JVCDATA": {"camera": number, "alarm": "motion"}}
#  Up to 4 cameras are supported for the demo. 

import serial, socket, time, json
#using paho-mqtt 1.6.1
import paho.mqtt.client as mqtt

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('10.255.255.255',1))
        IP = s.getsockname()[0]
    except Exception:
        IP = '127.0.0.1'
    finally:
        s.close
        print("using: " + str.encode(IP));
    return IP

UDP_IP = str.encode(get_ip());
UDP_PORT = 7023;

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

# MQTT connection callback
def on_connect (client, userdata, flags, rc):
    #print("Connected with result code" +str(rc))
    client.subscribe("$SYS/#")
# we are only going to publish
mqttc = mqtt.Client()
mqttc.on_connect = on_connect
#connect to the MDNS name of the KTI-1000, or the loopback
mqttc.connect("peiconnect.local", 1883, 60)

while True:
    data, addr = sock.recvfrom(1024)
    #print("Received packet: %s" % data)
    apiData = json.loads(data)
    cameraID = json.dumps(apiData['JVCDATA']['camera'])
    #print(cameraID)
    mqttc.publish("flexBtn/SUP_1/state", "IN"+cameraID+"RaisingEvent")
