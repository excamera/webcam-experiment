#!/bin/bash

./my-camera --fps 15 --audio-source alsa_input.usb-Logitech_Logitech_USB_Headset-00.analog-mono --audio-sink alsa_output.usb-Logitech_Logitech_USB_Headset-00.analog-stereo.2  --camera /dev/video0 $*

