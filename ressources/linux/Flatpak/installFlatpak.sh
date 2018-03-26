#!/bin/bash

flatpak-builder --repo=meinRepo2 --force-clean OpenModelica org.openmodelica.openmodelica.json
flatpak remote-add meinRepo2 meinRepo2 --no-gpg-verify
flatpak install meinRepo2 org.openmodelica.openmodelica

