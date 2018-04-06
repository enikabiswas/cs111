#!/bin/bash

scp -r kubilay@lnxsrv09.seas.ucla.edu:~/cs111/lab3a ./temp3a
scp -r ./temp3a root@192.168.7.2:~/lab3a/new3a