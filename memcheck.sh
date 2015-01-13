#!/bin/bash

valgrind --leak-check=yes  --suppressions=./portaudio.supp ./duplex
