FROM debian:9.5-slim
RUN apt update && apt install -y build-essential grep make sed wget nano udev libboost-all-dev bison flex texinfo sdcc libusb-1.0-0-dev libqtgui4  && apt-get clean 

ADD ./wixel-linux-110623-amd64.tar.gz /
# docker build will auto untar on add.
RUN ln -s /wixel_linux/wixelcmd /usr/local/bin/wixelcmd
RUN ln -s /wixel_linux/wixelconfig /usr/local/bin/wixelconfig
RUN cp /wixel_linux/99-pololu.rules /etc/udev/rules.d/

WORKDIR /home
