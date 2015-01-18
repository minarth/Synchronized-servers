#!/bin/bash

gnome-terminal -e "make run_s1"
gnome-terminal -e "make run_s2"

read

gnome-terminal -e "make run_c1"
gnome-terminal -e "make run_c1"
gnome-terminal -e "make run_c2"
gnome-terminal -e "make run_c2"
