Forked from github.com/Staars/RTL8822BS_BT

# Note

You should check if your kernel support 3-wire-protocol

```
sudo modprobe hci_uart
dmesg |grep Three-wire
```

it should have output like this:
```
Bluetooth: HCI UART protocol Three-wire (H5) registered
```

If no output : Unfortunately , you can't use this bluetooth

# How to use

install linux-header deb

```
cd bluetooth
make
sudo make install
sudo ./start_bt.sh
```


# For upstream

This is a dirty solution for mainline. only can make bluetooth work , but have the same issue as old way. Should waiting for realtek,vendor or other people provide the final solution.

patch:

```
--- a/drivers/bluetooth/btrtl.c
+++ b/drivers/bluetooth/btrtl.c
@@ -154,6 +154,19 @@ static const struct id_table ic_id_table
 	  .has_rom_version = true,
 	  .fw_name  = "rtl_bt/rtl8822cu_fw.bin",
 	  .cfg_name = "rtl_bt/rtl8822cu_config" },
+	  
+	/* 8822B with UART interface */
+	{ .match_flags = IC_MATCH_FL_LMPSUBV | IC_MATCH_FL_HCIREV |
+			 IC_MATCH_FL_HCIBUS,
+	  .lmp_subver = RTL_ROM_LMP_8822B,
+	  .hci_rev = 0x000b,
+	  .hci_ver = 0x07,
+	  .hci_bus = HCI_UART,
+	  .config_needed = true,
+	  .has_rom_version = true,
+	  .fw_name  = "rtl_bt/rtl8822bs_fw.bin",
+	  .cfg_name = "rtl_bt/rtl8822bs_config" },
+
 
 	/* 8822B */
 	{ IC_INFO(RTL_ROM_LMP_8822B, 0xb),
```


```
--- a/drivers/bluetooth/btrtl.c
+++ b/drivers/bluetooth/btrtl.c
@@ -802,6 +802,11 @@ int btrtl_get_uart_settings(struct hci_d
 		return -ENOENT;
 	}
 
+	/* Force set uart settings for rtl8822bs */
+	*device_baudrate = 0x0252c014;
+	*controller_baudrate = 115200;
+	*flow_control = 0;
+	
 	rtl_dev_dbg(hdev, "device baudrate = 0x%08x", *device_baudrate);
 	rtl_dev_dbg(hdev, "controller baudrate = %u", *controller_baudrate);
 	rtl_dev_dbg(hdev, "flow control %d", *flow_control);
```

example for device dts

```
//meson
&uart_A {
	status = "okay";
	pinctrl-0 = <&uart_a_pins>, <&uart_a_cts_rts_pins>;
	pinctrl-names = "default";
	uart-has-rtscts;

	bluetooth {
		//use 8822cs compatible to load hci_h5 and btrtl driver
		compatible = "realtek,rtl8822cs-bt";
		enable-gpios = <&gpio GPIOX_17 GPIO_ACTIVE_HIGH>;
	};
};

//sunxi
&uart1 {
	pinctrl-names = "default";
	pinctrl-0 = <&uart1_pins>, <&uart1_rts_cts_pins>;
	uart-has-rtscts;
	status = "okay";

	bluetooth {
		//use 8822cs compatible to load hci_h5 and btrtl driver
		compatible = "realtek,rtl8822cs-bt";
		device-wake-gpios = <&r_pio 1 2 GPIO_ACTIVE_HIGH>; /* PM2 */
		host-wake-gpios = <&r_pio 1 1 GPIO_ACTIVE_HIGH>; /* PM1 */
		enable-gpios = <&r_pio 1 4 GPIO_ACTIVE_HIGH>; /* PM4 */
	};
};
```


## RTL8822BS_BT

A modified version of rtk_hciattach and a script to turn on bluetooth on a mgv2000 with rtl8822bs. Probably this is not useful for other boxes.

On the mgv2000 (1/8) BT is connected via ttyAML1 (UART_A) , use GPIOX_17 for BT_EN pin , and uses 3-wire-protocol.

Copy firmware and config to /lib/firmware/rtlbt/ and remove the .bin-endings.

You must disconnect your USB-UART-adapter, if you use it as an external serial console. For me it is enough to unplug it from the USB-port and leave the cables connected to the PCB.
