/* SPDX-License-Identifier: GPL-2.0 */
/dts-v1/;

/ {
	description = KERNEL_NAME;
	#address-cells = <ADDR_CELLS>;

	images {
		kernel {
			description = KERNEL_NAME;
			data = /incbin/(IMAGE_BINARY);
			type = "kernel";
			arch = "riscv";
			os = "linux";
			compression = IMAGE_COMPRESSION;
			load = /bits/ ADDR_BITS <IMAGE_LOAD_ADDRESS>;
			entry = /bits/ ADDR_BITS <IMAGE_ENTRY_ADDRESS>;
			hash {
				algo = IMAGE_CHECK_ALGORITHM;
			};
		};
	};

	configurations {
		default = "conf-default";

		conf-default {
			description = "Generic Linux kernel";
			kernel = "kernel";
		};
	};
};
