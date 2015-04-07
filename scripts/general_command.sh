#!/bin/bash

SCRIPT_DIR=jlink

source config.sh

DEVICE=nrf51822

SCRIPT=general.script

$JLINK -Device $DEVICE -If SWD $SCRIPT_DIR/$SCRIPT
