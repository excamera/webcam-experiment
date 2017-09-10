#!/bin/bash

./my-camera  --delay 1 --fps 12 --camera /dev/video0 --audio-source alsa_input.usb-Logitech_Logitech_USB_Headset-00.analog-mono --audio-sink alsa_output.usb-Logitech_Logitech_USB_Headset-00.analog-stereo --after-file after.y4m --before-file before.y4m
