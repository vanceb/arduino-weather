Build
=====

    sudo docker build -t vanceb/datalogger .

Run
===

    sudo docker run -d --restart=always --name datalogger --device=/dev/ttyUSB0:/dev/ttyUSB0 --volume=/data/weather:/data vanceb/datalogger
