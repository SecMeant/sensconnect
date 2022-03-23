from archlinux:latest

run pacman -Syu --noconfirm
run pacman -S openbsd-netcat --noconfirm

run mkdir -p /opt/sensconnect
run echo "UwillN3v3rH@ckM3!" > /opt/sensconnect/adminpasswd

expose 34756

run useradd -m sensconnect
user sensconnect
workdir /home/sensconnect
