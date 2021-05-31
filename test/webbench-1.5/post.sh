#!/bin/bash

./webbench -c 1000 -t 5 -2 -m "username=root&passwd=123" --post http://localhost:1234/test
