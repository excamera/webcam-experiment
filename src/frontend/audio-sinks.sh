#!/bin/bash -ex

pacmd list-sinks | grep -e device.string -e 'name:'
