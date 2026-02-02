This is a simple python script that listens for incoming JSON data from a JVC IP camera
The JSON data is manually entered and just identifies the camera number and the alarm, which in this case is motion detect. 
The JSON data is parsed, and then sent via MQTT to a broker that is already installed on the network for both human and machine inputs. 
the broker maps inputs and keys up a 2-way radio and plays a pre-recorded message. 
So if camera 4 sends a motion detect, the user carrying a radio will hear the message "motion detected in the whatever area".   