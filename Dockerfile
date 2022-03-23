from archlinux:latest

run pacman -Syu --noconfirm
run pacman -Sy gcc gdb pwndbg openbsd-netcat --noconfirm

run mkdir -p /opt/sensconnect
run echo "UwillN3v3rH@ckM3!" > /opt/sensconnect/adminpasswd

expose 34756

run useradd -m sensconnect
workdir /home/sensconnect

run echo "H0w_D1d_You_Put_7haT_n00l_By73_0n_UR_k38o4rD_??!" > /home/sensconnect/flag.txt

copy sensconnect sensconnect
run strip sensconnect

run chown -R sensconnect:sensconnect /home/sensconnect/
user sensconnect
