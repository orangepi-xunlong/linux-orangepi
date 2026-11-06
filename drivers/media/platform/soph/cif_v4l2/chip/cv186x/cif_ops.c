#include "cif.h"
#include "pinctrl-cv186x.h"
#include <linux/of_gpio.h>
#include <vi_sys.h>
#define MIPI_IF
#define DVP_IF
#define BT601_IF
#define BT656_IF

#ifdef __CV181X__
#define SUBLVDS_IF
#define HISPI_IF
#define BT1120_IF
#define CUSTOM0_IF
#define BT_DEMUX_IF
#endif

extern int mclk0;
extern int mclk1;
extern int mclk2;
extern int mclk3;
extern int mclk4;
extern int mclk5;
extern int mclk6;
extern int lane_phase[LANE_SKEW_NUM];
extern int bypass_mac_clk;
extern unsigned int max_mac_clk;
static void __iomem *register_base;


static int LANE_IS_FRIST(uint16_t phy_mode, short lane_num, unsigned int devno)
{
	if (phy_mode == 0) {
		if (lane_num == CIF_PHY_LANE_0)
			return 1;
	} else if (phy_mode == 1) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_9)
				return 1;
		}
	} else if (phy_mode == 2) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		}
	} else if (phy_mode == 3) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_9)
				return 1;
		} else if (devno == 4) {
			if (lane_num == CIF_PHY_LANE_12)
				return 1;
		}
	} else if (phy_mode == 4) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		} else if (devno == 3) {
			if (lane_num == CIF_PHY_LANE_6)
				return 1;
		}
	} else if (phy_mode == 5) {
		if (devno == 0) {
			if (lane_num == CIF_PHY_LANE_0)
				return 1;
		}
	}
	return 0;
}

#ifndef FPGA_PORTING
static int _cif_set_clk_buffer(struct cif_ctx *ctx, int clk_port, int min_port, int max_port)
{
	if (ctx->phy_mode == 0) {
		if (clk_port == 0) {
			if (max_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 3 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			} else if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 4) {
			if (max_port == 4 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 4 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 4 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			} else if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 5) {
			if (max_port == 5 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		}
	} else if (ctx->phy_mode == 1) {
		if (clk_port == 0) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			}
		} else if (clk_port == 3) {
			if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P4);
			}
		} else if (clk_port == 4) {
			if (max_port == 4 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P3);
			}
		}
	} else if (ctx->phy_mode == 2) {
		if (clk_port == 0) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P2);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 2 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 2 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			} else if (max_port == 2 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
				cif_set_clk_dir(ctx, CIF_CLK_P22P1);
			}
		}
	} else if (ctx->phy_mode == 3) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			}
		} else if (clk_port == 4) {
			if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P42P5);
			}
		} else if (clk_port == 5) {
			if (max_port == 5 && min_port == 4) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			} else if (max_port == 5 && min_port == 5) {
				cif_set_clk_dir(ctx, CIF_CLK_P52P4);
			}
		}
	} else if (ctx->phy_mode == 4) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		} else if (clk_port == 2) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P22P3);
			}
		} else if (clk_port == 3) {
			if (max_port == 3 && min_port == 2) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			} else if (max_port == 3 && min_port == 3) {
				cif_set_clk_dir(ctx, CIF_CLK_P32P2);
			}
		}
	} else if (ctx->phy_mode == 5) {
		if (clk_port == 0) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P02P1);
			}
		} else if (clk_port == 1) {
			if (max_port == 1 && min_port == 0) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			} else if (max_port == 1 && min_port == 1) {
				cif_set_clk_dir(ctx, CIF_CLK_P12P0);
			}
		}
	}

	return 0;
}
#endif

const struct sync_code_s default_sync_code = {
	.norm_bk_sav = 0xAB0,
	.norm_bk_eav = 0xB60,
	.norm_sav = 0x800,
	.norm_eav = 0x9D0,
	.n0_bk_sav = 0x2B0,
	.n0_bk_eav = 0x360,
	.n1_bk_sav = 0x6B0,
	.n1_bk_eav = 0x760,
};

static struct cvi_link *ctx_to_link(const struct cif_ctx *ctx)
{
	return container_of(ctx, struct cvi_link, cif_ctx);
}

const char *_to_string_input_mode(enum input_mode_e input_mode)
{
	switch (input_mode) {
	case INPUT_MODE_MIPI:
		return "MIPI";
	case INPUT_MODE_SUBLVDS:
		return "SUBLVDS";
	case INPUT_MODE_HISPI:
		return "HISPI";
	case INPUT_MODE_CMOS:
		return "CMOS";
	case INPUT_MODE_BT1120:
		return "BT1120";
	case INPUT_MODE_BT601:
		return "INPUT_MODE_BT601";
	case INPUT_MODE_BT656_9B:
		return "INPUT_MODE_BT656_9B";
	case INPUT_MODE_BT656_9B_DDR:
		return "INPUT_MODE_BT656_9B_DDR";
	case INPUT_MODE_CUSTOM_0:
		return "INPUT_MODE_CUSTOM_0";
	case INPUT_MODE_BT_DEMUX:
		return "INPUT_MODE_BT_DEMUX";
	default:
		return "unknown";
	}
}

const char *_to_string_mac_clk(enum rx_mac_clk_e mac_clk)
{
	switch (mac_clk) {
	case RX_MAC_CLK_150M:
		return "150MHZ";
	case RX_MAC_CLK_200M:
		return "200MHZ";
	case RX_MAC_CLK_300M:
		return "300MHZ";
	case RX_MAC_CLK_400M:
		return "400MHZ";
	case RX_MAC_CLK_500M:
		return "500MHZ";
	case RX_MAC_CLK_600M:
		return "600MHZ";
	case RX_MAC_CLK_900M:
		return "900MHZ";
	default:
		return "unknown";
	}
}

const char *_to_string_raw_data_type(enum raw_data_type_e raw_data_type)
{
	switch (raw_data_type) {
	case RAW_DATA_8BIT:
		return "RAW8";
	case RAW_DATA_10BIT:
		return "RAW10";
	case RAW_DATA_12BIT:
		return "RAW12";
	case YUV422_8BIT:
		return "YUV422_8BIT";
	case YUV422_10BIT:
		return "YUV422_10BIT";
	default:
		return "unknown";
	}
}

const char *_to_string_mipi_wdr_mode(enum mipi_wdr_mode_e wdr)
{
	switch (wdr) {
	case MIPI_WDR_MODE_NONE:
		return "NONE";
	case MIPI_WDR_MODE_VC:
		return "VC";
	case MIPI_WDR_MODE_DT:
		return "DT";
	case MIPI_WDR_MODE_DOL:
		return "DOL";
	case MIPI_WDR_MODE_MANUAL:
		return "MANUAL";
	default:
		return "unknown";
	}
}

const char *_to_string_wdr_mode(enum wdr_mode_e wdr)
{
	switch (wdr) {
	case CIF_WDR_MODE_NONE:
		return "NONE";
	case CIF_WDR_MODE_2F:
		return "2To1";
	case CIF_WDR_MODE_3F:
		return "3To1";
	case CIF_WDR_MODE_DOL_2F:
		return "DOL2To1";
	case CIF_WDR_MODE_DOL_3F:
		return "DOL3To1";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_sync_mode(enum lvds_sync_mode_e mode)
{
	switch (mode) {
	case LVDS_SYNC_MODE_SOF:
		return "SOF";
	case LVDS_SYNC_MODE_SAV:
		return "SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_bit_endian(enum lvds_bit_endian endian)
{
	switch (endian) {
	case LVDS_ENDIAN_LITTLE:
		return "LITTLE";
	case LVDS_ENDIAN_BIG:
		return "BIG";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_vsync_type(enum lvds_vsync_type_e type)
{
	switch (type) {
	case LVDS_VSYNC_NORMAL:
		return "NORMAL";
	case LVDS_VSYNC_SHARE:
		return "SHARE";
	case LVDS_VSYNC_HCONNECT:
		return "HCONNECT";
	default:
		return "unknown";
	}
}

const char *_to_string_lvds_fid_type(enum lvds_fid_type_e type)
{
	switch (type) {
	case LVDS_FID_NONE:
		return "FID_NONE";
	case LVDS_FID_IN_SAV:
		return "FID_IN_SAV";
	default:
		return "unknown";
	}
}

const char *_to_string_mclk(enum cam_pll_freq_e freq)
{
	switch (freq) {
	case CAMPLL_FREQ_NONE:
		return "CAMPLL_FREQ_NONE";
	case CAMPLL_FREQ_37P125M:
		return "CAMPLL_FREQ_37P125M";
	case CAMPLL_FREQ_25M:
		return "CAMPLL_FREQ_25M";
	case CAMPLL_FREQ_27M:
		return "CAMPLL_FREQ_27M";
	case CAMPLL_FREQ_24M:
		return "CAMPLL_FREQ_24M";
	case CAMPLL_FREQ_26M:
		return "CAMPLL_FREQ_26M";
	default:
		return "unknown";
	}
}

const char *_to_string_csi_decode(enum csi_decode_fmt_e fmt)
{
	switch (fmt) {
	case DEC_FMT_YUV422_8:
		return "yuv422-8";
	case DEC_FMT_YUV422_10:
		return "yuv422-10";
	case DEC_FMT_RAW8:
		return "raw8";
	case DEC_FMT_RAW10:
		return "raw10";
	case DEC_FMT_RAW12:
		return "raw12";
	default:
		return "unknown";
	}
}

const char *_to_string_dlane_state(enum mipi_dlane_state_e state)
{
	switch (state) {
	case HS_IDLE:
		return "hs_idle";
	case HS_SYNC:
		return "hs_sync";
	case HS_SKEW_CAL:
		return "skew_cal";
	case HS_ALT_CAL:
		return "alt_cal";
	case HS_PREAMPLE:
		return "preample";
	case HS_HST:
		return "hs_hst";
	case HS_ERR:
		return "hs_err";
	default:
		return "unknown";
	}
}

const char *_to_string_deskew_state(enum mipi_deskew_state_e state)
{
	switch (state) {
	case DESKEW_IDLE:
		return "idle";
	case DESKEW_START:
		return "start";
	case DESKEW_DONE:
		return "done";
	default:
		return "unknown";
	}
}

static void cif_dump_dev_attr(struct cvi_cif_dev *dev,
			      struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	int i;

	dev_dbg(_dev, "devno = %d, input_mode = %s, mac_clk = %s\n",
		attr->devno,
		_to_string_input_mode(attr->input_mode),
		_to_string_mac_clk(attr->mac_clk));
	dev_dbg(_dev, "mclk%d = %s\n",
		attr->mclk.cam, _to_string_mclk(attr->mclk.freq));
	dev_dbg(_dev, "width = %d, height = %d\n",
		attr->img_size.width, attr->img_size.height);
	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		struct mipi_dev_attr_s *mipi = &attr->mipi_attr;

		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(mipi->raw_data_type));

		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s", i,
				mipi->lane_id[i],
				mipi->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_mipi_wdr_mode(mipi->wdr_mode));
		for (i = 0; i < WDR_VC_NUM; i++) {
			dev_dbg(_dev, "data_type[%d] = 0x%x\n",
				i, mipi->data_type[i]);
		}
	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		int j;

		dev_dbg(_dev, "wdr_mode = %s\n",
			_to_string_wdr_mode(lvds->wdr_mode));
		dev_dbg(_dev, "sync_mode = %s\n",
			_to_string_lvds_sync_mode(lvds->sync_mode));
		dev_dbg(_dev, "raw_data_type = %s\n",
			_to_string_raw_data_type(lvds->raw_data_type));
		dev_dbg(_dev, "data_endian = %s\n",
			_to_string_bit_endian(lvds->data_endian));
		dev_dbg(_dev, "sync_code_endian = %s\n",
			_to_string_bit_endian(lvds->sync_code_endian));
		for (i = 0; i < MIPI_LANE_NUM + 1; i++) {
			dev_dbg(_dev, "lane_id[%d] = %d, pn_swap = %s ", i,
				lvds->lane_id[i],
				lvds->pn_swap[i] ? "True" : "False");
		}
		dev_dbg(_dev, "sync code = {\n");
		for (i = 0; i < MIPI_LANE_NUM; i++) {
			dev_dbg(_dev, "\t{\n");
			for (j = 0; j < WDR_VC_NUM+1; j++) {
				dev_dbg(_dev,
					"\t\t{ %3x, %3x, %3x, %3x },\n",
					lvds->sync_code[i][j][0],
					lvds->sync_code[i][j][1],
					lvds->sync_code[i][j][2],
					lvds->sync_code[i][j][3]);
			}
			dev_dbg(_dev, "\t},\n");
		}
		dev_dbg(_dev, "}\n");
		dev_dbg(_dev, "vsync_type = %s\n",
			_to_string_lvds_vsync_type(lvds->vsync_type.sync_type));
		dev_dbg(_dev, "fid = %s\n",
			_to_string_lvds_fid_type(lvds->fid_type.fid));

	}
	break;
	case INPUT_MODE_CMOS:
		break;
	case INPUT_MODE_BT1120:
		break;
	default:
		break;
	}
}

static void cif_show_dev_attr(struct seq_file *m,
			      struct combo_dev_attr_s *attr)
{
	int i;
	char buf[32] = {0};
	char buf2[32] = {0};
	char *ptr;
	struct mipi_dev_attr_s *mipi;

	seq_printf(m, "%8s%10s%10s%10s%10s%12s%16s\n",
		"Devno", "WorkMode", "DataType", "WDRMode",
		"SyncMode", "DataEndian", "SyncCodeEndian");

	seq_printf(m, "%8d%10s", attr->devno,
		_to_string_input_mode(attr->input_mode));

	switch (attr->input_mode) {
	case INPUT_MODE_MIPI: {
		mipi = &attr->mipi_attr;
		ptr = buf;

		seq_printf(m, "%10s%10s%10s%12s%16s\n",
			_to_string_raw_data_type(mipi->raw_data_type),
			_to_string_mipi_wdr_mode(mipi->wdr_mode),
			"N/A", "N/A", "N/A");

		seq_printf(m, "%17s%28s\n", "LinkId", "PN Swap");
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", mipi->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%15s%30s\n", buf, buf2);

	}
	break;
	case INPUT_MODE_SUBLVDS:
	case INPUT_MODE_HISPI: {
		struct lvds_dev_attr_s *lvds = &attr->lvds_attr;
		char *ptr = buf;

		seq_printf(m, "%10s%10s%10s%12s%16s\n",
			_to_string_raw_data_type(lvds->raw_data_type),
			_to_string_wdr_mode(lvds->wdr_mode),
			_to_string_lvds_sync_mode(lvds->sync_mode),
			_to_string_bit_endian(lvds->data_endian),
			_to_string_bit_endian(lvds->sync_code_endian));

		seq_printf(m, "%17s%28s\n", "LinkId", "PN Swap");
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->lane_id[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		ptr = buf2;
		for (i = 0; i < CIF_LANE_NUM; i++) {
			sprintf(ptr, "%2d,", lvds->pn_swap[i]);
			ptr += 3;
		}
		*(ptr - 1) = '\0';
		seq_printf(m, "%15s%30s\n", buf, buf2);
	}
	break;
	case INPUT_MODE_CMOS:
	case INPUT_MODE_BT1120:
		seq_printf(m, "%10s%10s%15s%10s%12s%16s\n",
			"N/A", "N/A", "N/A", "N/A", "N/A", "N/A");
		break;
	default:
		break;
	}
}

static void cif_show_mipi_sts(struct seq_file *m,
			      struct cvi_link *link)
{
	struct cvi_csi_status *sts = &link->sts_csi;

	seq_printf(m, "%6s%7s%7s%7s%6s%9s%9s\n",
		   "Devno", "EccErr", "CrcErr", "HdrErr", "WcErr", "fifofull", "decode");
	seq_printf(m, "%6d%7d%7d%7d%6d%9d%9s\n",
		   link->attr.devno,
		   sts->errcnt_ecc, sts->errcnt_crc, sts->errcnt_hdr,
		   sts->errcnt_wc, sts->fifo_full,
		   _to_string_csi_decode(cif_get_csi_decode_fmt(&link->cif_ctx)));
}

static void cif_show_phy_sts(struct seq_file *m,
			      struct cvi_link *link)
{
	union mipi_phy_state state;

	cif_get_csi_phy_state(&link->cif_ctx, &state);

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
		   "Physical:", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8");
	seq_printf(m, "%11s%9x%9x%9x%9x%9x%9x%9x%9x%9x\n",
		   " ",
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_0),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_1),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_2),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_3),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_4),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_5),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_6),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_7),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_8));

	seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
		   "Physical:", "D9", "D10", "D11", "D12", "D13", "D14", "D15", "D16", "D17");
	seq_printf(m, "%11s%9x%9x%9x%9x%9x%9x%9x%9x%9x\n",
		   " ",
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_9),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_10),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_11),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_12),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_13),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_14),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_15),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_16),
		   cif_get_lane_data(&link->cif_ctx, CIF_PHY_LANE_17));

	switch (link->cif_ctx.mac_num)
	{
		case CIF_MAC_0:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7");
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d2_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d3_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d4_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d5_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d6_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d7_datahs_state));
			seq_printf(m, "%11s%9s%9s%9s%9s%9s\n",
				"Digital:", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9d%9d%9d%9d%9s\n",
				" ",
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		case CIF_MAC_3:
		case CIF_MAC_4:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "D2", "D3", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9s%9s%9s%9s%9d%9d%9d%9d%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d2_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d3_datahs_state),
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		case CIF_MAC_1:
		case CIF_MAC_2:
		case CIF_MAC_5:
		{
			seq_printf(m, "%11s%9s%9s%9s%9s%9s%9s%9s\n",
				"Digital:", "D0", "D1", "CK_HS", "CK_ULPS", "CK_STOP", "CK_ERR", "Deskew");
			seq_printf(m, "%11s%9s%9s%9d%9d%9d%9d%9s\n",
				" ",
				_to_string_dlane_state(state.bits_datahs.d0_datahs_state),
				_to_string_dlane_state(state.bits_datahs.d1_datahs_state),
				state.bits_ckhs.clk_hs_state,
				state.bits_ckhs.clk_ulps_state,
				state.bits_ckhs.clk_stop_state,
				state.bits_ckhs.clk_err_state,
				_to_string_deskew_state(state.bits_ckhs.deskew_state));
			break;
		}
		default:
			break;
	}
}

int proc_cif_show(struct seq_file *m, void *v)
{
	struct cvi_cif_dev *dev = (struct cvi_cif_dev *)m->private;
	int i;

	seq_printf(m, "\nModule: [MIPI_RX], Build Time[%s]\n",
			UTS_VERSION);
	seq_puts(m, "\n------------Combo DEV ATTR--------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on)
			cif_show_dev_attr(m, &dev->link[i].attr);

	seq_puts(m, "\n------------MIPI info-------------------\n");
	for (i = 0; i < MAX_LINK_NUM; i++)
		if (dev->link[i].is_on
			&& (dev->link[i].attr.input_mode == INPUT_MODE_MIPI)) {
			cif_show_mipi_sts(m, &dev->link[i]);
			cif_show_phy_sts(m, &dev->link[i]);
		}

	return 0;
}

static u8 *dbg_type[] = {
	"reset",
	"hs_s",
	"snsr_r",
	"snsr_on",
	"bt_fmt",
	"mac_clk"
};

static void dbg_print_usage(struct device *dev)
{
	dev_info(dev, "\n------------DBG USAGE-------------------\n");
	dev_info(dev, "reset [devno]: reset mipi-rx error count\n");
	dev_info(dev, "hs_s [devno] [value]: set mipi-rx hs_settle, value : 0~255, def: 16\n");
	dev_info(dev, "snsr_r [devno] [reset]: reset : 1 - reset, 0 - unreset sensor gpio\n");
	dev_info(dev, "snsr_on [cam_num] [on] [mclk]: enable/disable sensor clk\n");
	dev_info(dev, "                               cam_num : 0 - cam0, 1 - cam1\n");
	dev_info(dev, "                               on : 0 - off, 1 - on\n");
	dev_info(dev, "                               mclk : 0 - off, 1 - 37.125M, 2 - 25M, 3 - 27M\n");
	dev_info(dev, "                                    : 4 - 24M, 5 - 26M\n");
	dev_info(dev, "bt_fmt [devno] [0~3]: set bt format CbY/CrY/YCb/YCr\n");
	dev_info(dev, "mac_clk [devno] [200/400/600]: set mac clock (MHz)\n");
}

int dbg_hdler(struct cvi_cif_dev *dev, char const *input)
{
	struct cvi_link *link = &dev->link[0];
	struct cif_ctx *ctx;
	//int reset;
	u32 num;
	u8 str[80] = {0};
	u8 t = 0;
	u32 a, v, v2;
	u8 i, n;
	u8 *p;

	num = sscanf(input, "%s %d %d %d", str, &a, &v, &v2);
	if (num > 4) {
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	dev_info(link->dev, "input = %s %d\n", str, num);
	/* convert to lower case for following type compare */
	p = str;
	for (; *p; ++p)
		*p = tolower(*p);
	n = ARRAY_SIZE(dbg_type);
	for (i = 0; i < n; i++) {
		if (!strcmp(str, dbg_type[i])) {
			t = i;
			break;
		}
	}
	if (i == n) {
		dev_info(link->dev, "unknown type(%s)!\n", str);
		dbg_print_usage(link->dev);
		return -EINVAL;
	}

	switch (t) {
	case 0:
		/* reset */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		if (link->is_on) {
			link->sts_csi.errcnt_ecc = 0;
			link->sts_csi.errcnt_crc = 0;
			link->sts_csi.errcnt_wc = 0;
			link->sts_csi.errcnt_hdr = 0;
			link->sts_csi.fifo_full = 0;
			cif_clear_csi_int_sts(ctx);
			cif_unmask_csi_int_sts(ctx, 0x0F);
		}

		break;
	case 1:
		/* hs-settle */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_hs_settle(ctx, v);
		break;
	case 2:
		/* sensor reset */
		// if (a > MAX_LINK_NUM)
		// 	return -EINVAL;

		// link = &dev->link[a];
		// ctx = &link->cif_ctx;

		// reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

		// if (!gpio_is_valid(link->snsr_rst_pin))
		// 	return 0;
		// if (v)
		// 	gpio_direction_output(link->snsr_rst_pin, reset);
		// else
		// 	gpio_direction_output(link->snsr_rst_pin, !reset);
		break;
	case 3:
		/* sensor enable */
		if (a >= MAX_LINK_NUM)
			return -EINVAL;
		if (!a)
			mclk0 = v2;
		else if (a == 1)
			mclk1 = v2;
		else if (a == 2)
			mclk2 = v2;
		else if (a == 3)
			mclk3 = v2;
		else if (a == 4)
			mclk4 = v2;
		else if (a == 5)
			mclk5 = v2;
		else if (a == 6)
			mclk6 = v2;
		link = &dev->link[a];
		_cif_enable_snsr_clk(dev, a, v);
		break;
	case 4:
		/* bt format out */
		if (a > MAX_LINK_NUM)
			return -EINVAL;

		link = &dev->link[a];
		ctx = &link->cif_ctx;

		cif_set_bt_fmt_out(ctx, v);
		break;
	case 5:
		/* set mac clock */
		if (v <= 150)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_150M);
		else if (v <= 200)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_200M);
		else if (v <= 300)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_300M);
		else if (v <= 400)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_400M);
		else if (v <= 500)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_500M);
		else if (v <= 600)
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_600M);
		else
			_cif_set_mac_clk(dev, a, RX_MAC_CLK_900M);
		break;
	default:
		dbg_print_usage(link->dev);
		break;
	}

	return 0;
}

#ifndef FPGA_PORTING
int cif_set_output_clk_edge(struct cvi_cif_dev *dev,
				   struct clk_edge_s *clk_edge)
{
	struct cif_ctx *ctx = &dev->link[clk_edge->devno].cif_ctx;
	enum phy_lane_id_e phy_id;

	dev->link[clk_edge->devno].clk_edge = clk_edge->edge;

	for (phy_id = CIF_PHY_LANE_0; phy_id < CIF_PHY_LANE_NUM; phy_id++) {
		cif_set_clk_edge(ctx, phy_id,
				 (clk_edge->edge == CLK_UP_EDGE)
				 ? CIF_CLK_RISING : CIF_CLK_FALLING);
	}

	return 0;
}
#endif

static void cif_reset_param(struct cvi_link *link)
{
	link->is_on = 0;
	link->clk_edge = CLK_UP_EDGE;
	link->msb = OUTPUT_NORM_MSB;
	link->crop_top = 0;
	link->distance_fp = 0;
	memset(&link->param, 0, sizeof(struct cif_param));
	memset(&link->attr, 0, sizeof(struct combo_dev_attr_s));
	memset(&link->sts_csi, 0, sizeof(struct cvi_csi_status));
	memset(&link->sts_lvds, 0, sizeof(struct cvi_lvds_status));
}

static int mac_toggle_reset(struct cvi_cif_dev *dev, uint32_t mac_num)
{
	u32 reg_value;

	// spin_lock(&dev->lock);
	register_base = ioremap(VIP_SYS_ADDRESS, sizeof(u32));
	if (!register_base) {
		CIF_PR(CIF_ERROR, "Failed to map register address\n");
		return -ENOMEM;
	}

	reg_value = readl(register_base);

	if (mac_num == 0) {
		SET_BIT(reg_value, 2, 1);
	} else if (mac_num == 1) {
		SET_BIT(reg_value, 3, 1);
	} else if (mac_num == 2) {
		SET_BIT(reg_value, 4, 1);
	} else if (mac_num == 3) {
		SET_BIT(reg_value, 5, 1);
	} else if (mac_num == 4) {
		SET_BIT(reg_value, 6, 1);
	} else if (mac_num == 5) {
		SET_BIT(reg_value, 7, 1);
	} else if (mac_num == 6) {
		SET_BIT(reg_value, 8, 1);
	} else if (mac_num == 7) {
		SET_BIT(reg_value, 9, 1);
	} else {
		CIF_PR(CIF_ERROR, "Invalid mac number\n");
	}

	writel(reg_value, register_base);
	udelay(20);

	if (mac_num == 0) {
		SET_BIT(reg_value, 2, 0);
	} else if (mac_num == 1) {
		SET_BIT(reg_value, 3, 0);
	} else if (mac_num == 2) {
		SET_BIT(reg_value, 4, 0);
	} else if (mac_num == 3) {
		SET_BIT(reg_value, 5, 0);
	} else if (mac_num == 4) {
		SET_BIT(reg_value, 6, 0);
	} else if (mac_num == 5) {
		SET_BIT(reg_value, 7, 0);
	} else if (mac_num == 6) {
		SET_BIT(reg_value, 8, 0);
	} else if (mac_num == 7) {
		SET_BIT(reg_value, 9, 0);
	} else {
		CIF_PR(CIF_ERROR, "Invalid mac number\n");
	}

	writel(reg_value, register_base);
	iounmap(register_base);
	// spin_unlock(&dev->lock);

	return 0;
}

int cif_reset_mipi(struct cvi_cif_dev *dev, uint32_t devno)
{
	struct cvi_link *link = &dev->link[devno];
	union vi_sys_reset mask;
	mask.raw = 0;

	/* mask the interrupts */
	if (link->is_on)
		cif_mask_csi_int_sts(&link->cif_ctx, 0x1F);

	/* reset phy */
	// if (link->phy_reset && link->phy_apb_reset) {
	// 	reset_control_assert(link->phy_reset);
	// 	reset_control_assert(link->phy_apb_reset);
	// 	udelay(5);
	// 	reset_control_deassert(link->phy_reset);
	// 	reset_control_deassert(link->phy_apb_reset);
	// }

	switch (devno) {
	case 0:
		mask.b.csi_mac0 = 1;
		break;
	case 1:
		mask.b.csi_mac1 = 1;
		break;
	case 2:
		mask.b.csi_mac2 = 1;
		break;
	case 3:
		mask.b.csi_mac3 = 1;
		break;
	case 4:
		mask.b.csi_mac4 = 1;
		break;
	case 5:
		mask.b.csi_mac5 = 1;
		break;
	case 6:
		mask.b.csi_mac6 = 1;
		break;
	case 7:
		mask.b.csi_mac7 = 1;
		break;
	default:
		break;
	}

	/* sw reset mac by vip register */
	mac_toggle_reset(dev, link->cif_ctx.mac_num);

	/* reset parameters. */
	cif_reset_param(link);

	return 0;
}

int cif_set_crop_top(struct cvi_cif_dev *dev,
			    struct crop_top_s *crop)
{
	struct cif_ctx *ctx = &dev->link[crop->devno].cif_ctx;

	dev->link[crop->devno].crop_top = crop->crop_top;
	/* strip the top info line. */
	cif_crop_info_line(ctx, crop->crop_top, crop->update);

	return 0;
}

int cif_set_windowing(struct cvi_cif_dev *dev,
			    struct cif_crop_win_s *win)
{
	struct cif_ctx *ctx = &dev->link[win->devno].cif_ctx;
	struct cif_crop_s crop;

	crop.x = win->x;
	crop.y = win->y;
	crop.w = win->w;
	crop.h = win->h;
	crop.enable = win->enable;

	cif_set_crop(ctx, &crop);

	return 0;
}

int cif_set_wdr_manual(struct cvi_cif_dev *dev,
			      struct manual_wdr_s *manual)
{
	struct cif_ctx *ctx = &dev->link[manual->devno].cif_ctx;
	struct cif_param *param = &dev->link[manual->devno].param;

	param->hdr_manual = manual->attr.manual_en;
	param->hdr_shift = manual->attr.l2s_distance;
	param->hdr_vsize = manual->attr.lsef_length;
	param->hdr_rm_padding = manual->attr.discard_padding_lines;

	cif_hdr_manual_config(ctx, param, !!manual->attr.update);

	return 0;
}

int cif_set_lvds_fp_vs(struct cvi_cif_dev *dev,
			      struct vsync_gen_s *vs)
{
	struct cif_ctx *ctx = &dev->link[vs->devno].cif_ctx;

	dev->link[vs->devno].distance_fp = vs->distance_fp;
	cif_set_lvds_vsync_gen(ctx, vs->distance_fp);

	return 0;
}

int cif_bt_fmt_out(struct cvi_cif_dev *dev,
			  struct bt_fmt_out_s *fmt_out)
{
	struct cif_ctx *ctx = &dev->link[fmt_out->devno].cif_ctx;

	dev->link[fmt_out->devno].bt_fmt_out = (enum ttl_bt_fmt_out)fmt_out->fmt_out;
	cif_set_bt_fmt_out(ctx, (enum ttl_bt_fmt_out)fmt_out->fmt_out);

	return 0;
}

static int cif_reset_snsr_gpio(struct cvi_cif_dev *dev,
			       unsigned int devno, uint8_t on)
{
	struct cvi_link *link;
	int reset;

	if (devno >= MAX_LINK_NUM)
		return -EINVAL;

	link = &dev->link[devno];
	reset = (link->snsr_rst_pol == OF_GPIO_ACTIVE_LOW) ? 0 : 1;

	if (!gpio_is_valid(link->snsr_rst_pin))
		return 0;
	if (on)
		gpio_direction_output(link->snsr_rst_pin, reset);
	else
		gpio_direction_output(link->snsr_rst_pin, !reset);

	return 0;
}

static int LANE_IS_PORT1(int x)
{
	if (x < CIF_PHY_LANE_3) {
		return 0;
	} else if (x < CIF_PHY_LANE_6) {
		return 1;
	} else if (x < CIF_PHY_LANE_9) {
		return 2;
	} else if (x < CIF_PHY_LANE_12) {
		return 3;
	} else if (x < CIF_PHY_LANE_15) {
		return 4;
	} else if (x < CIF_PHY_LANE_NUM) {
		return 5;
	} else {
		return -EINVAL;
	}
}

#define IS_SAME_PORT(x, y)	(!((LANE_IS_PORT1(x))^(LANE_IS_PORT1(y))))

#define PAD_CTRL_PU		BIT(2)
#define PAD_CTRL_PD		BIT(3)
#define PSD_CTRL_OFFSET(n)	((5 - n) * 8)

#ifdef MIPI_IF
static int _cif_set_attr_mipi(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct mipi_dev_attr_s *attr,
				  uint32_t devno)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, mipi_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_csi *csi = &param->cfg.csi;
	struct mipi_demux_info_s *info = &attr->demux;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	uint32_t value;

	param->type = CIF_TYPE_CSI;

	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		csi->fmt = CSI_RAW_8;
		csi->decode_type = 0x2A;
		break;
	case RAW_DATA_10BIT:
		csi->fmt = CSI_RAW_10;
		csi->decode_type = 0x2B;
		break;
	case RAW_DATA_12BIT:
		csi->fmt = CSI_RAW_12;
		csi->decode_type = 0x2C;
		break;
	case RAW_DATA_16BIT:
		csi->fmt = CSI_RAW_16;
		//csi->decode_type = 0x2C;
		break;
	case YUV422_8BIT:
		csi->fmt = CSI_YUV422_8B;
		csi->decode_type = 0x1E;
		break;
	case YUV422_10BIT:
		csi->fmt = CSI_YUV422_10B;
		csi->decode_type = 0x1F;
		break;
	default:
		return -EINVAL;
	}
	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used =  LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);
		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	csi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);

	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);

	// if (csi->lane_num == 8) {
	// 	for (i = 0; (i < csi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (csi->lane_num == 4) {
	// 	for (i = 0; (i < csi->lane_num + 1); i++) {
	// 		if (!i)
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_CLK]);
	// 		else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 		else
	// 			cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 					lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 	}
	// } else if (csi->lane_num > 0) {
	// 	/* if clk and data are in the same port.*/
	// 	if (clk_port > 0) {
	// 		for (i = 0; i < (csi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CLK]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_DATA]);
	// 		}
	// 	} else {
	// 		for (i = 0; i < (csi->lane_num + 1); i++) {
	// 			if (!i)
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_CLK]);
	// 			else if (IS_SAME_PORT(attr->lane_id[0], attr->lane_id[i]))
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_NEAR]);
	// 			else
	// 				cif_set_lane_deskew(ctx, attr->lane_id[i],
	// 						lane_phase[LANE_SKEW_CROSS_DATA_FAR]);
	// 		}
	// 	}
	// } else {
	// 	return -EINVAL;
	// }
#endif

	/* config the dphy seting. */
	if (attr->dphy.enable)
		cif_set_hs_settle(ctx, attr->dphy.hs_settle);
	/* config the wdr mode. */
	switch (attr->wdr_mode) {
	case MIPI_WDR_MODE_NONE:
		break;
	case MIPI_WDR_MODE_DT:
		csi->hdr_mode = CSI_HDR_MODE_DT;
		for (i = 0; i < MAX_WDR_FRAME_NUM; i++)
			csi->data_type[i] = attr->data_type[i];
		break;
	case MIPI_WDR_MODE_MANUAL:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	case MIPI_WDR_MODE_VC:
		csi->hdr_mode = CSI_HDR_MODE_VC;
		break;
	case MIPI_WDR_MODE_DOL:
		csi->hdr_mode = CSI_HDR_MODE_DOL;
		break;
	default:
		return -EINVAL;
	}
	/* config vc mapping */
	if (info->demux_en) {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = info->vc_mapping[i];
		}
	} else {
		for (i = 0; i < MIPI_DEMUX_NUM; i++) {
			csi->vc_mapping[i] = i;
		}
	}

	param->hdr_en = (attr->wdr_mode != MIPI_WDR_MODE_NONE);
	cif_streaming(ctx, 1, param->hdr_en);

	return 0;
}
#endif //MIPI_IF

#ifdef SUBLVDS_IF
static int _cif_set_lvds_vsync_type(struct cif_ctx *ctx,
				    struct lvds_dev_attr_s *attr,
				    struct param_sublvds *sublvds)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;
	struct cvi_link *link = ctx_to_link(ctx);

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT1;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 15;
		cif_set_lvds_vsync_gen(ctx, 15);
		break;
	case LVDS_VSYNC_HCONNECT:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT2;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 1;
		cif_set_lvds_vsync_gen(ctx, 1);
		sublvds->hdr_hblank[0] = type->hblank1;
		sublvds->hdr_hblank[1] = type->hblank2;
		sublvds->h_size = combo->img_size.width;
		/* [TODO] use other api to strip the info line*/
		/* strip the top info line. */
		cif_crop_info_line(ctx, 1, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif //MIPI_IF

#ifdef SUBLVDS_IF
static int _cif_set_lvds_vsync_type(struct cif_ctx *ctx,
				    struct lvds_dev_attr_s *attr,
				    struct param_sublvds *sublvds)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;
	struct cvi_link *link = ctx_to_link(ctx);

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT1;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 15;
		cif_set_lvds_vsync_gen(ctx, 15);
		break;
	case LVDS_VSYNC_HCONNECT:
		sublvds->hdr_mode = CIF_SLVDS_HDR_PAT2;
		/* [TODO] use other api to set the fp */
		link->distance_fp = 1;
		cif_set_lvds_vsync_gen(ctx, 1);
		sublvds->hdr_hblank[0] = type->hblank1;
		sublvds->hdr_hblank[1] = type->hblank2;
		sublvds->h_size = combo->img_size.width;
		/* [TODO] use other api to strip the info line*/
		/* strip the top info line. */
		cif_crop_info_line(ctx, 1, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_sublvds(struct cvi_cif_dev *dev,
				 struct cif_ctx *ctx,
				 struct lvds_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct cvi_link *link = ctx_to_link(ctx);
	struct param_sublvds *sublvds = &param->cfg.sublvds;
	struct sublvds_sync_code *sc;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_SUBLVDS;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		sublvds->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		sublvds->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		sublvds->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_MSB;

	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		sublvds->endian = CIF_SLVDS_ENDIAN_LSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		sublvds->endian = CIF_SLVDS_ENDIAN_MSB;
		sublvds->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode != LVDS_SYNC_MODE_SAV)
		return -EINVAL;
	/* config the lane id*/
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used = LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);

		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	sublvds->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);
#endif

	/* config the sync code */
	memcpy(&sublvds->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &sublvds->sync_code.slvds;
	sc->n0_lef_sav = attr->sync_code[0][0][0];
	sc->n0_lef_eav = attr->sync_code[0][0][1];
	sc->n1_lef_sav = attr->sync_code[0][0][2];
	sc->n1_lef_eav = attr->sync_code[0][0][3];
	sc->n0_sef_sav = attr->sync_code[0][1][0];
	sc->n0_sef_eav = attr->sync_code[0][1][1];
	sc->n1_sef_sav = attr->sync_code[0][1][2];
	sc->n1_sef_eav = attr->sync_code[0][1][3];
	sc->n0_lsef_sav = attr->sync_code[0][2][0];
	sc->n0_lsef_eav = attr->sync_code[0][2][1];
	sc->n1_lsef_sav = attr->sync_code[0][2][2];
	sc->n1_lsef_eav = attr->sync_code[0][2][3];

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CIF_WDR_MODE_NONE:
		/* [TODO] use other api to set the fp */
		link->distance_fp = 6;
		cif_set_lvds_vsync_gen(ctx, 6);
		break;
	case CIF_WDR_MODE_DOL_2F:
	case CIF_WDR_MODE_DOL_3F:
		/* [TODO] 3 exposure hdr hw is not ready. */
		/* config th Vsync type */
		rc = _cif_set_lvds_vsync_type(ctx, attr, sublvds);
		if (rc < 0)
			return rc;
		break;
	default:
		return -EINVAL;
	}
	param->hdr_en = (attr->wdr_mode != CIF_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CIF_WDR_MODE_NONE);

	return 0;
}
#endif // SUBLVDS_IF

#ifdef HISPI_IF
static int _cif_set_hispi_vsync_type(struct cif_ctx *ctx,
				     struct lvds_dev_attr_s *attr,
				     struct cif_param *param)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct lvds_vsync_type_s *type = &attr->vsync_type;

	switch (type->sync_type) {
	case LVDS_VSYNC_NORMAL:
		break;
	case LVDS_VSYNC_SHARE:
		param->hdr_manual = combo->wdr_manu.manual_en;
		param->hdr_shift = combo->wdr_manu.l2s_distance;
		param->hdr_vsize = combo->wdr_manu.lsef_length;
		param->hdr_rm_padding = combo->wdr_manu.discard_padding_lines;
		cif_hdr_manual_config(ctx, param, !!combo->wdr_manu.update);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int _cif_set_attr_hispi(struct cif_dev *dev,
			       struct cif_ctx *ctx,
			       struct lvds_dev_attr_s *attr,
				   uint32_t devno)
{
	struct combo_dev_attr_s *combo =
		container_of(attr, struct combo_dev_attr_s, lvds_attr);
	struct cif_param *param = ctx->cur_config;
	struct param_hispi *hispi = &param->cfg.hispi;
	struct hispi_sync_code *sc;
	uint32_t tbl = 0x1FF;
	int i, j = 0, clk_port = 0, max_port = 0, min_port = 5, frist_lane_used = 0;
	int rc = 0;
	uint32_t value;

	param->type = CIF_TYPE_HISPI;
	/* config the bit mode. */
	switch (attr->raw_data_type) {
	case RAW_DATA_8BIT:
		hispi->fmt = CIF_SLVDS_8_BIT;
		break;
	case RAW_DATA_10BIT:
		hispi->fmt = CIF_SLVDS_10_BIT;
		break;
	case RAW_DATA_12BIT:
		hispi->fmt = CIF_SLVDS_12_BIT;
		break;
	default:
		return -EINVAL;
	}
	/* config the endian. */
	if (attr->data_endian == LVDS_ENDIAN_BIG &&
	    attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_LITTLE &&
		   attr->sync_code_endian == LVDS_ENDIAN_BIG) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_MSB;
	} else if (attr->data_endian == LVDS_ENDIAN_BIG &&
		   attr->sync_code_endian == LVDS_ENDIAN_LITTLE) {
		hispi->endian = CIF_SLVDS_ENDIAN_LSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	} else {
		hispi->endian = CIF_SLVDS_ENDIAN_MSB;
		hispi->wrap_endian = CIF_SLVDS_ENDIAN_LSB;
	}
	/* check the sync mode. */
	if (attr->sync_mode == LVDS_SYNC_MODE_SOF) {
		hispi->mode = CIF_HISPI_MODE_PKT_SP;
	} else if (attr->sync_mode == LVDS_SYNC_MODE_SAV) {
		hispi->mode = CIF_HISPI_MODE_STREAM_SP;
		hispi->h_size = combo->img_size.width;
	} else {
		return -EINVAL;
	}
	/* config the lane id */
	for (i = 0; i < CIF_LANE_NUM; i++) {
		if (attr->lane_id[i] < 0)
			continue;
		if (attr->lane_id[i] >= CIF_PHY_LANE_NUM)
			return -EINVAL;

		frist_lane_used = LANE_IS_FRIST(ctx->phy_mode, attr->lane_id[i], devno);

		if (!i)
			clk_port = LANE_IS_PORT1(attr->lane_id[i]);
		else {
			if (LANE_IS_PORT1(attr->lane_id[i]) >= max_port) {
				max_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
			if (LANE_IS_PORT1(attr->lane_id[i]) <= min_port) {
				min_port = LANE_IS_PORT1(attr->lane_id[i]);
			}
		}
		cif_set_rx_bus_config(ctx, i, attr->lane_id[i]);
		cif_set_lane_id(ctx, i, attr->lane_id[i], attr->pn_swap[i]);
		/* clear pad ctrl pu/pd */
		if (dev->pad_ctrl) {
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]));
			value = ioread32(dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
			value &= ~(PAD_CTRL_PU | PAD_CTRL_PD);
			iowrite32(value, dev->pad_ctrl + PSD_CTRL_OFFSET(attr->lane_id[i]) + 4);
		}
		tbl &= ~(1<<attr->lane_id[i]);
		j++;
	}
	hispi->lane_num = j - 1;
	while (ffs(tbl)) {
		uint32_t idx = ffs(tbl) - 1;

		cif_set_lane_id(ctx, j++, idx, 0);
		tbl &= ~(1 << idx);
	}
	//frist line for each port should be turned on, as the MAC will only receive clocks from one wire
	set_rx_frist_lane_enable(ctx, frist_lane_used);
	/* config  clock buffer direction.
	 * 1. When clock is between 0~2 and 1c4d, direction is P0->P1.
	 * 2. When clock is between 3~5 and 1c4d, direction is P1->P0.
	 * 3. When clock and data is between 0~2 and 1c2d, direction bit is freerun.
	 * 4. When clock is between 0~2 but data is not, direction is P0->P1.
	 * 5. When clock and data is between 3~5 and 1c2d and mac1 is used, direction bit is freerun.
	 * 6. When clock is between 3~5 but data is not, direction is P1->P0.
	 */
#ifndef FPGA_PORTING
	if (_cif_set_clk_buffer(ctx, clk_port, min_port, max_port) != 0) {
		return -EINVAL;
	}
	if (min_port >= 0 && max_port < 2) {
		cif_set_group(ctx, 0);
	} else if (min_port >= 2 && max_port < 4) {
		cif_set_group(ctx, 1);
	} else if (min_port >= 4 && max_port < 6) {
		cif_set_group(ctx, 2);
	} else if (min_port >= 0 && max_port < 4) {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
	} else if (min_port >= 2 && max_port < 6) {
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	} else {
		cif_set_group(ctx, 0);
		cif_set_group(ctx, 1);
		cif_set_group(ctx, 2);
	}

	cif_set_pd(ctx, attr->lane_id[0]);
#endif

	/* config the sync code */
	memcpy(&hispi->sync_code, &default_sync_code,
	       sizeof(default_sync_code));
	sc = &hispi->sync_code.hispi;
	sc->t1_sol = attr->sync_code[0][0][0];
	sc->t1_eol = attr->sync_code[0][0][1];
	sc->t1_sof = attr->sync_code[0][0][2];
	sc->t1_eof = attr->sync_code[0][0][3];
	sc->t2_sol = attr->sync_code[0][1][0];
	sc->t2_eol = attr->sync_code[0][1][1];
	sc->t2_sof = attr->sync_code[0][1][2];
	sc->t2_eof = attr->sync_code[0][1][3];
	sc->vsync_gen = sc->t1_sof;

	/* config the wdr */
	switch (attr->wdr_mode) {
	case CIF_WDR_MODE_NONE:
	case CIF_WDR_MODE_2F:
	case CIF_WDR_MODE_3F: /* [TODO] 3 exposure hdr hw is not ready. */
		break;
	default:
		return -EINVAL;
	}
	/* config th Vsync type */
	rc = _cif_set_hispi_vsync_type(ctx, attr, param);
	if (rc < 0)
		return rc;

	param->hdr_en = (attr->wdr_mode != CIF_WDR_MODE_NONE);
	/* [TODO] config the fid type. */
	cif_streaming(ctx, 1, attr->wdr_mode != CIF_WDR_MODE_NONE);

	return 0;
}
#endif // HISPI_IF

#ifdef DVP_IF
#ifndef FPGA_PORTING
static void cif_config_pinmux(enum ttl_src_e vi, uint32_t pad)
{
	if (vi == TTL_VI_SRC_VI0) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX14P, VI0_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX14N, VI0_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX15P, VI0_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX15N, VI0_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX16P, VI0_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX16N, VI0_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX17P, VI0_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX17N, VI0_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX11N, VI0_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX11P, VI0_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX10N, VI0_D10, PHY);
				break;
			case 11:
				PINMUX_CONFIG(PAD_MIPI_RX10P, VI0_D11, PHY);
				break;
			case 12:
				PINMUX_CONFIG(PAD_MIPI_RX9N, VI0_D12, PHY);
				break;
			case 13:
				PINMUX_CONFIG(PAD_MIPI_RX9P, VI0_D13, PHY);
				break;
			case 14:
				PINMUX_CONFIG(PAD_MIPI_RX8N, VI0_D14, PHY);
				break;
			case 15:
				PINMUX_CONFIG(PAD_MIPI_RX8P, VI0_D15, PHY);
				break;
			case 16:
				PINMUX_CONFIG(PAD_MIPI_RX12N, VI0_D16, PHY);
				break;
			case 17:
				PINMUX_CONFIG(PAD_MIPI_RX13P, VI0_D17, PHY);
				break;
			case 18:
				PINMUX_CONFIG(PAD_MIPI_RX13N, VI0_D18, PHY);
				break;
			case 19:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI0_D19, PHY);
				break;
			case 20:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI0_D20, PHY);
				break;
			case 21:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI0_D21, PHY);
				break;
			case 22:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI0_D22, PHY);
				break;
			case 23:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI0_D23, PHY);
				break;
			case 24:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI0_D24, PHY);
				break;
			case 25:
				PINMUX_CONFIG(PAD_MIPI_RX6N, VI0_D25, PHY);
				break;
			case 26:
				PINMUX_CONFIG(PAD_MIPI_RX7P, VI0_D26, PHY);
				break;
			case 27:
				PINMUX_CONFIG(PAD_MIPI_RX7N, VI0_D27, PHY);
				break;
			default:
				break;
		}
	} else if (vi == TTL_VI_SRC_VI1) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX8P, VI1_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX8N, VI1_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX9P, VI1_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX9N, VI1_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX10P, VI1_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX10N, VI1_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX11P, VI1_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX11N, VI1_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI1_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI1_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI1_D10, PHY);
				break;
			case 11:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI1_D11, PHY);
				break;
			case 12:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI1_D12, PHY);
				break;
			case 13:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI1_D13, PHY);
				break;
			case 14:
				PINMUX_CONFIG(PAD_MIPI_RX6N, VI1_D14, PHY);
				break;
			case 15:
				PINMUX_CONFIG(PAD_MIPI_RX7P, VI1_D15, PHY);
				break;
			case 16:
				PINMUX_CONFIG(PAD_MIPI_RX7N, VI1_D16, PHY);
				break;
			default:
				break;
		}
	} else if (vi == TTL_VI_SRC_VI2) {
		switch (pad) {
			case 0:
				PINMUX_CONFIG(PAD_MIPI_RX2P, VI2_D0, PHY);
				break;
			case 1:
				PINMUX_CONFIG(PAD_MIPI_RX2N, VI2_D1, PHY);
				break;
			case 2:
				PINMUX_CONFIG(PAD_MIPI_RX3P, VI2_D2, PHY);
				break;
			case 3:
				PINMUX_CONFIG(PAD_MIPI_RX3N, VI2_D3, PHY);
				break;
			case 4:
				PINMUX_CONFIG(PAD_MIPI_RX4P, VI2_D4, PHY);
				break;
			case 5:
				PINMUX_CONFIG(PAD_MIPI_RX4N, VI2_D5, PHY);
				break;
			case 6:
				PINMUX_CONFIG(PAD_MIPI_RX5P, VI2_D6, PHY);
				break;
			case 7:
				PINMUX_CONFIG(PAD_MIPI_RX5N, VI2_D7, PHY);
				break;
			case 8:
				PINMUX_CONFIG(PAD_MIPI_RX0N, VI2_D8, PHY);
				break;
			case 9:
				PINMUX_CONFIG(PAD_MIPI_RX1P, VI2_D9, PHY);
				break;
			case 10:
				PINMUX_CONFIG(PAD_MIPI_RX1N, VI2_D10, PHY);
				break;
			default:
				break;
		}
	}
}

#endif//FPGA_PORTING

static int _cif_set_attr_cmos(struct cvi_cif_dev *dev,
			      struct cif_ctx *ctx,
			      struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		/*if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)*/
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	switch (attr->ttl_attr.raw_data_type) {
	case RAW_DATA_8BIT:
		ttl->sensor_fmt = TTL_SENSOR_8_BIT;
		break;
	case RAW_DATA_10BIT:
		ttl->sensor_fmt = TTL_SENSOR_10_BIT;
		break;
	case RAW_DATA_12BIT:
		ttl->sensor_fmt = TTL_SENSOR_12_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (attr->ttl_attr.ttl_fmt) {
	case TTL_SYNC_PAT:
		ttl->fmt = TTL_SYNC_PAT_SENSOR;
		break;
	case TTL_VHS_11B:
		ttl->fmt = TTL_VHS_SENSOR;
		break;
	case TTL_VDE_11B:
		ttl->fmt = TTL_VDE_SENSOR;
		break;
	case TTL_VSDE_11B:
		ttl->fmt = TTL_VSDE_SENSOR;
		break;
	default:
		return -EINVAL;
	}

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->vi_sel = VI_RAW;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // DVP_IF

#ifdef BT1120_IF
static int _cif_set_attr_bt1120(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->fmt = TTL_SYNC_PAT_17B_BT1120;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT1120;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 9 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 8 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT1120_IF

#ifdef BT601_IF
static int _cif_set_attr_bt601(struct cvi_cif_dev *dev,
				       struct cif_ctx *ctx,
				       struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		//if (attr->ttl_attr.vi0_clk == 0)
		//	PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		//else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	switch (attr->ttl_attr.raw_data_type) {
	case RAW_DATA_8BIT:
		ttl->sensor_fmt = TTL_SENSOR_8_BIT;
		break;
	case RAW_DATA_10BIT:
		ttl->sensor_fmt = TTL_SENSOR_10_BIT;
		break;
	case RAW_DATA_12BIT:
		ttl->sensor_fmt = TTL_SENSOR_12_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (attr->ttl_attr.ttl_fmt) {
	case TTL_VHS_11B:
		ttl->fmt = TTL_VHS_11B_BT601;
		break;
	case TTL_VHS_19B:
		ttl->fmt = TTL_VHS_19B_BT601;
		break;
	case TTL_VDE_11B:
		ttl->fmt = TTL_VDE_11B_BT601;
		break;
	case TTL_VDE_19B:
		ttl->fmt = TTL_VDE_19B_BT601;
		break;
	case TTL_VSDE_11B:
		ttl->fmt = TTL_VSDE_11B_BT601;
		break;
	case TTL_VSDE_19B:
		ttl->fmt = TTL_VSDE_19B_BT601;
		break;
	default:
		return -EINVAL;
	}

	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0x23 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xbf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT601_IF

#ifdef BT_DEMUX_IF
static int _cif_set_attr_bt_demux(struct cvi_cif_dev *dev,
					struct cif_ctx *ctx,
					struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_btdemux *btdemux = &param->cfg.btdemux;
	struct cvi_link *link = ctx_to_link(ctx);
	struct bt_demux_attr_s *info = &attr->bt_demux_attr;
#ifndef FPGA_PORTING
	enum ttl_src_e vi = attr->ttl_attr.vi;
#else
	enum ttl_src_e vi = TTL_VI_SRC_VI1;
#endif
	int i;

#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		printk("BT_DEMUX can not use VI0\n");
		return -EINVAL;
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->bt_demux_attr.func[i] < 0)
			continue;
		if (attr->bt_demux_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		//cif_set_mac_vi_pinmux(&dev->link[ctx->cur_mac_num].cif_ctx,
		//	(enum ttl_vi_from_e)vi, i, attr->bt_demux_attr.func[i]);
		cif_set_mac_vi_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->bt_demux_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->bt_demux_attr.func[i]);
#endif
	}
	param->type = CIF_TYPE_BT_DMUX;
	btdemux->fmt = TTL_SYNC_PAT_9B_BT656;
	btdemux->width = attr->img_size.width - 1;
	btdemux->height = attr->img_size.height - 1;
	btdemux->clk_inv = link->clk_edge;
	btdemux->v_fp = (!info->v_fp) ? 0x0f : info->v_fp;
	btdemux->h_fp = (!info->h_fp) ? 0x0f : info->h_fp;
	btdemux->sync_code_part_A[0] = info->sync_code_part_A[0];
	btdemux->sync_code_part_A[1] = info->sync_code_part_A[1];
	btdemux->sync_code_part_A[2] = info->sync_code_part_A[2];
	for (i = 0; i < BT_DEMUX_NUM; i++) {
		btdemux->sync_code_part_B[i].sav_vld = info->sync_code_part_B[i].sav_vld;
		btdemux->sync_code_part_B[i].sav_blk = info->sync_code_part_B[i].sav_blk;
		btdemux->sync_code_part_B[i].eav_vld = info->sync_code_part_B[i].eav_vld;
		btdemux->sync_code_part_B[i].eav_blk = info->sync_code_part_B[i].eav_blk;
	}
	btdemux->demux = (enum cif_btdmux_mode_e)info->mode;
	btdemux->yc_exchg = info->yc_exchg;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT_DEMUX_IF

#ifdef BT656_IF
static int _cif_set_attr_bt656_9b(struct cvi_cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		//if (attr->ttl_attr.vi0_clk == 0)
		//	PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		//else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}

static int _cif_set_attr_bt656_9b_ddr(struct cvi_cif_dev *dev,
				  struct cif_ctx *ctx,
				  struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	struct cvi_link *link = ctx_to_link(ctx);
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		printk("BT_656_DDR can not use VI0\n");
		return -EINVAL;
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_mac_vi_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_SYNC_PAT_9B_BT656;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->clk_inv = link->clk_edge;
	ttl->vi_sel = VI_BT656;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 0xf : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 0xf : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // BT656_IF

#ifdef	CUSTOM0_IF
static int _cif_set_attr_custom0(struct cvi_cif_dev *dev,
				struct cif_ctx *ctx,
				struct combo_dev_attr_s *attr)
{
	struct cif_param *param = ctx->cur_config;
	struct param_ttl *ttl = &param->cfg.ttl;
	enum ttl_src_e vi = attr->ttl_attr.vi;
	int i;

	if (vi == TTL_VI_SRC_VI2)
		return -EINVAL;

	/* config the pinmux */
	for (i = 0; i < VI_FUNC_NUM; i++) {
		if (attr->ttl_attr.func[i] < 0)
			continue;
		if (attr->ttl_attr.func[i] >= MAX_PAD_NUM)
			return -EINVAL;
		cif_set_ttl_pinmux(ctx, (enum ttl_vi_from_e)vi, i, attr->ttl_attr.func[i]);
#ifndef FPGA_PORTING
		cif_config_pinmux(vi, attr->ttl_attr.func[i]);
#endif
	}
#ifndef FPGA_PORTING
	if (vi == TTL_VI_SRC_VI0){
		if (attr->ttl_attr.vi0_clk == 0)
			PINMUX_CONFIG(PAD_MIPI_RX12P, VI0_CLK0, PHY);
		else if (attr->ttl_attr.vi0_clk == 1)
			PINMUX_CONFIG(PAD_MIPI_RX6P, VI0_CLK1, PHY);
	} else if (vi == TTL_VI_SRC_VI1) {
		PINMUX_CONFIG(PAD_MIPI_RX6P, VI1_CLK0, PHY);
	} else {
		PINMUX_CONFIG(PAD_MIPI_RX0P, VI2_CLK0, PHY);
	}
#endif
	param->type = CIF_TYPE_TTL;
	ttl->vi_from = (enum ttl_vi_from_e)vi;
	ttl->fmt_out = TTL_BT_FMT_OUT_CBYCRY;
	ttl->fmt = TTL_CUSTOM_0;
	ttl->width = attr->img_size.width - 1;
	ttl->height = attr->img_size.height - 1;
	ttl->sensor_fmt = TTL_SENSOR_12_BIT;
	ttl->vi_sel = VI_BT601;
	ttl->v_bp = (!attr->ttl_attr.v_bp) ? 4095 : attr->ttl_attr.v_bp;
	ttl->h_bp = (!attr->ttl_attr.h_bp) ? 4 : attr->ttl_attr.h_bp;

	cif_streaming(ctx, 1, 0);

	return 0;
}
#endif // CUSTOM0_IF

static inline int cif_set_mac_clk(struct cvi_cif_dev *dev, uint32_t devno,
		enum rx_mac_clk_e mac_clk)
{
	return _cif_set_mac_clk(dev, devno, mac_clk);
}

int cif_set_dev_attr(struct cvi_cif_dev *dev,
			    struct combo_dev_attr_s *attr)
{
	struct device *_dev = dev->miscdev.this_device;
	enum input_mode_e input_mode = attr->input_mode;
	struct cif_ctx *ctx;
	struct cif_param *param;
	struct combo_dev_attr_s *rx_attr;
	int rc = 0;

	/* force the btdmux to devno 0*/
#ifdef FPGA_PORTING
	if (input_mode == INPUT_MODE_BT_DEMUX) {
		attr->devno = CIF_MAC_VI_0;
	}
#endif

	ctx = &dev->link[attr->devno].cif_ctx;
	param = &dev->link[attr->devno].param;
	rx_attr = &dev->link[attr->devno].attr;


	printk("************************************************************************");
	printk("macclk%d = %s\n",attr->mclk.cam, _to_string_mac_clk(attr->mac_clk));
	printk("wdr_mode%d = %s\n",attr->mclk.cam, _to_string_mipi_wdr_mode(attr->mipi_attr.wdr_mode));

	cif_dump_dev_attr(dev, attr);

	memset(param, 0, sizeof(*param));
	ctx->cur_config = param;
	ctx->phy_mode = attr->cif_mode;
	printk("phy_mode%d\n", ctx->phy_mode);

	memcpy(rx_attr, attr, sizeof(*rx_attr));

#ifndef FPGA_PORTING
	/* Setting for serial interface. */
	if (input_mode == INPUT_MODE_MIPI ||
	    input_mode == INPUT_MODE_SUBLVDS ||
	    input_mode == INPUT_MODE_HISPI) {
		struct clk_edge_s clk_edge;

		/* set the default clock edge. */
		clk_edge.devno = attr->devno;
		clk_edge.edge = CLK_DOWN_EDGE;
		//cif_set_output_clk_edge(dev, &clk_edge);
	}
#endif
	/* set mac clk */
	if (attr->devno < CIF_MAC_VI_0) {
		rc = cif_set_mac_clk(dev, attr->devno, attr->mac_clk);
		if (rc < 0)
			return rc;
	}

	/* decide the mclk */
	switch (attr->mclk.cam)
	{
	case 0:
		mclk0 = attr->mclk.freq;
		break;
	case 1:
		mclk1 = attr->mclk.freq;
		break;
	case 2:
		mclk2 = attr->mclk.freq;
		break;
	case 3:
		mclk3 = attr->mclk.freq;
		break;
	case 4:
		mclk4 = attr->mclk.freq;
		break;
	case 5:
		mclk5 = attr->mclk.freq;
		break;
	case 6:
		mclk6 = attr->mclk.freq;
		break;
	default:
		break;
	}

	switch (input_mode) {
#ifdef MIPI_IF
	case INPUT_MODE_MIPI:
		rc = _cif_set_attr_mipi(dev, ctx, &rx_attr->mipi_attr, attr->devno);
		break;
#endif
#ifdef SUBLVDS_IF
	case INPUT_MODE_SUBLVDS:
		rc = _cif_set_attr_sublvds(dev, ctx, &rx_attr->lvds_attr);
		break;
#endif
#ifdef HISPI_IF
	case INPUT_MODE_HISPI:
		rc = _cif_set_attr_hispi(dev, ctx, &rx_attr->lvds_attr);
		break;
#endif
#ifdef DVP_IF
	case INPUT_MODE_CMOS:
		rc = _cif_set_attr_cmos(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT1120_IF
	case INPUT_MODE_BT1120:
		rc = _cif_set_attr_bt1120(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT601_IF
	case INPUT_MODE_BT601:
		rc = _cif_set_attr_bt601(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT656_IF
	case INPUT_MODE_BT656_9B:
		rc = _cif_set_attr_bt656_9b(dev, ctx, rx_attr);
		break;
	case INPUT_MODE_BT656_9B_DDR:
		rc = _cif_set_attr_bt656_9b_ddr(dev, ctx, rx_attr);
		break;
#endif
#ifdef CUSTOM0_IF
	case INPUT_MODE_CUSTOM_0:
		rc = _cif_set_attr_custom0(dev, ctx, rx_attr);
		break;
#endif
#ifdef BT_DEMUX_IF
	case INPUT_MODE_BT_DEMUX:
		rc = _cif_set_attr_bt_demux(dev, ctx, rx_attr);
		break;
#endif
	default:
		return -EINVAL;
	}
	if (rc < 0) {
		dev_err(_dev, "set attr fail %d\n", rc);
		return rc;
	}

	dev->link[attr->devno].is_on = 1;
	/* unmask the interrupts */
	if (attr->devno < CIF_MAX_CSI_NUM)
		cif_unmask_csi_int_sts(ctx, 0x1F);

	return 0;
}

int cif_start_stream(struct cvi_cif_dev *dev, struct combo_dev_attr_s *attr)
{
	int rc = 0;

	rc = cif_reset_snsr_gpio(dev, attr->devno, 1);
	if (rc) {
		CIF_PR(CIF_ERROR, "reset sns gpio down fail : %d\n", rc);
	}

	rc = cif_reset_mipi(dev, attr->devno);
	if (rc) {
		CIF_PR(CIF_ERROR, "reset mipi fail : %d\n", rc);
	}

	rc = cif_set_dev_attr(dev, attr);
	if (rc) {
		CIF_PR(CIF_ERROR, "SetMipiAttr fail : %d\n", rc);
	}

	rc = _cif_enable_snsr_clk(dev, attr->mclk.cam, attr->mclk.freq != CAMPLL_FREQ_NONE);
	if (rc) {
		CIF_PR(CIF_ERROR, "enable sns clk fail : %d\n", rc);
	}

	cif_reset_snsr_gpio(dev, attr->devno, 0);
	if (rc) {
		CIF_PR(CIF_ERROR, "reset sns gpio up fail : %d\n", rc);
	}

	return 0;
}