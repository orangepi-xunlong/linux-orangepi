workdir=$(pwd)
echo $workdir
sudo cp $workdir/NetworkManager_back.conf /etc/NetworkManager/NetworkManager.conf
sudo /etc/init.d/network-manager restart