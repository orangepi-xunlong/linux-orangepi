workdir=$(pwd)

echo "Create 5.8GHz Wireless HotSpot ..."
sudo cp $workdir/NetworkManager.conf /etc/NetworkManager/NetworkManager.conf
sudo /etc/init.d/network-manager restart
sleep 3

sudo cp $workdir/ip_forward /proc/sys/net/ipv4/ip_forward
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE

sudo cp $workdir/isc-dhcp-server /etc/default/isc-dhcp-server
sudo chmod 0755 /etc/default/isc-dhcp-server
sudo cp $workdir/dhcpd.conf /etc/dhcp/dhcpd.conf
sudo chmod 0755 /etc/dhcp/dhcpd.conf
sudo service isc-dhcp-server restart



sudo ifconfig wlan0 192.168.43.1 netmask 255.255.255.0
echo "*********************************************************************************"
echo "If you want to stop HotSpot,Press Crtl + C "
echo "'getwifi.sh' can slove 'device not managed' error when you're not useing AP mode"
echo "*********************************************************************************"
sudo hostapd $workdir/hostapd-5g.conf

sudo cp $workdir/NetworkManager_back.conf /etc/NetworkManager/NetworkManager.conf
sudo /etc/init.d/network-manager restart