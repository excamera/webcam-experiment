#!/bin/bash -ex

pacmd list-sources | grep -e device.string -e 'name:'
