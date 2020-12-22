#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>

#include <linux/scatterlist.h>
#include <linux/swap.h>
#include <linux/list.h>

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/seq_file.h>
#include <linux/module.h>
#include <linux/string_helpers.h>

#define RESULT_OK		0
#define RESULT_FAIL		1
#define RESULT_UNSUP_HOST	2
#define RESULT_UNSUP_CARD	3

#define BUFFER_SIZE	(64*1024)

/* This type MUST be 8 bit */
typedef unsigned char	BYTE;

/* These types MUST be 16 bit */
typedef short			SHORT;
typedef unsigned short	WORD;
typedef unsigned short	WCHAR;

/* These types MUST be 16 bit or 32 bit */
typedef int				INT;
typedef unsigned int	UINT;

/* These types MUST be 32 bit */
typedef long			LONG;
typedef unsigned long	DWORD;

struct mmc_fatfs_card {
	struct mmc_card	*card;
	struct mmc_host	*host;
	u8		mmc_card_status;
	u8		scratch[BUFFER_SIZE];
	u8		*buffer;
	DWORD n_sectors;
	int (*asyncMount)(void);
	void (*asyncRenewState)(int);
};

struct mmc_fatfs_async_req {
	struct mmc_async_req areq;
	struct mmc_fatfs_card *pfc;
};

static struct mmc_fatfs_card * g_fatfs_card = NULL;

/*
 * Configure correct block size in card
 */
static int mmc_fatfs_set_blksize(struct mmc_fatfs_card *pfc, unsigned size)
{
	return mmc_set_blocklen(pfc->card, size);
}

/*
 * Fill in the mmc_request structure given a set of transfer parameters.
 */
static void mmc_fatfs_prepare_mrq(struct mmc_fatfs_card *pfc,
	struct mmc_request *mrq, struct scatterlist *sg, unsigned sg_len,
	unsigned dev_addr, unsigned blocks, unsigned blksz, int write)
{
	BUG_ON(!mrq || !mrq->cmd || !mrq->data || !mrq->stop);

	if (blocks > 1) {
		mrq->cmd->opcode = write ?
			MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	} else {
		mrq->cmd->opcode = write ?
			MMC_WRITE_BLOCK : MMC_READ_SINGLE_BLOCK;
	}

	mrq->cmd->arg = dev_addr;
	if (!mmc_card_blockaddr(pfc->card))
		mrq->cmd->arg <<= 9;

	mrq->cmd->flags = MMC_RSP_R1 | MMC_CMD_ADTC;

	if (blocks == 1)
		mrq->stop = NULL;
	else {
		mrq->stop->opcode = MMC_STOP_TRANSMISSION;
		mrq->stop->arg = 0;
		mrq->stop->flags = MMC_RSP_R1B | MMC_CMD_AC;
	}

	mrq->data->blksz = blksz;
	mrq->data->blocks = blocks;
	mrq->data->flags = write ? MMC_DATA_WRITE : MMC_DATA_READ;
	mrq->data->sg = sg;
	mrq->data->sg_len = sg_len;

	mmc_set_data_timeout(mrq->data, pfc->card);
}

static int mmc_fatfs_busy(struct mmc_command *cmd)
{
	return !(cmd->resp[0] & R1_READY_FOR_DATA) ||
		(R1_CURRENT_STATE(cmd->resp[0]) == R1_STATE_PRG);
}

/*
 * Wait for the card to finish the busy state
 */
static int mmc_fatfs_wait_busy(struct mmc_fatfs_card *pfc)
{
	int ret, busy;
	struct mmc_command cmd = {0};

	busy = 0;
	do {
		memset(&cmd, 0, sizeof(struct mmc_command));

		cmd.opcode = MMC_SEND_STATUS;
		cmd.arg = pfc->card->rca << 16;
		cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;

		ret = mmc_wait_for_cmd(pfc->card->host, &cmd, 0);
		if (ret)
			break;

		if (!busy && mmc_fatfs_busy(&cmd)) {
			busy = 1;
			if (pfc->card->host->caps & MMC_CAP_WAIT_WHILE_BUSY)
				pr_info("%s: Warning: Host did not "
					"wait for busy state to end.\n",
					mmc_hostname(pfc->card->host));
		}
	} while (mmc_fatfs_busy(&cmd));

	return ret;
}

static int mmc_fatfs_check_result(struct mmc_fatfs_card *pfc,
				 struct mmc_request *mrq)
{
	int ret;

	BUG_ON(!mrq || !mrq->cmd || !mrq->data);

	ret = 0;

	if (!ret && mrq->cmd->error)
		ret = mrq->cmd->error;
	if (!ret && mrq->data->error)
		ret = mrq->data->error;
	if (!ret && mrq->stop && mrq->stop->error)
		ret = mrq->stop->error;
	if (!ret && mrq->data->bytes_xfered !=
		mrq->data->blocks * mrq->data->blksz)
		ret = RESULT_FAIL;

	if (ret == -EINVAL)
		ret = RESULT_UNSUP_HOST;

	return ret;
}

static int mmc_fatfs_check_result_async(struct mmc_card *card,
				       struct mmc_async_req *areq)
{
	struct mmc_fatfs_async_req *pfar =
		container_of(areq, struct mmc_fatfs_async_req, areq);

	mmc_fatfs_wait_busy(pfar->pfc);

	return mmc_fatfs_check_result(pfar->pfc, areq->mrq);
}

static void mmc_fatfs_nonblock_reset(struct mmc_request *mrq,
				    struct mmc_command *cmd,
				    struct mmc_command *stop,
				    struct mmc_data *data)
{
	memset(mrq, 0, sizeof(struct mmc_request));
	memset(cmd, 0, sizeof(struct mmc_command));
	memset(data, 0, sizeof(struct mmc_data));
	memset(stop, 0, sizeof(struct mmc_command));

	mrq->cmd = cmd;
	mrq->data = data;
	mrq->stop = stop;
}

static int mmc_fatfs_nonblock_transfer(struct mmc_fatfs_card *pfc,
				      struct scatterlist *sg, unsigned sg_len,
				      unsigned dev_addr, unsigned blocks,
				      unsigned blksz, int write, int count)
{
	struct mmc_request mrq1;
	struct mmc_command cmd1;
	struct mmc_command stop1;
	struct mmc_data data1;

	struct mmc_request mrq2;
	struct mmc_command cmd2;
	struct mmc_command stop2;
	struct mmc_data data2;

	struct mmc_fatfs_async_req fareq[2];
	struct mmc_async_req *done_areq;
	struct mmc_async_req *cur_areq = &fareq[0].areq;
	struct mmc_async_req *other_areq = &fareq[1].areq;
	int i;
	int ret;

	fareq[0].pfc = pfc;
	fareq[1].pfc = pfc;

	mmc_fatfs_nonblock_reset(&mrq1, &cmd1, &stop1, &data1);
	mmc_fatfs_nonblock_reset(&mrq2, &cmd2, &stop2, &data2);

	cur_areq->mrq = &mrq1;
	cur_areq->err_check = mmc_fatfs_check_result_async;
	other_areq->mrq = &mrq2;
	other_areq->err_check = mmc_fatfs_check_result_async;

	for (i = 0; i < count; i++) {
		mmc_fatfs_prepare_mrq(pfc, cur_areq->mrq, sg, sg_len, dev_addr,
				     blocks, blksz, write);
		done_areq = mmc_start_req(pfc->card->host, cur_areq, &ret);

		if (ret || (!done_areq && i > 0))
			goto err;

		if (done_areq) {
			if (done_areq->mrq == &mrq2)
				mmc_fatfs_nonblock_reset(&mrq2, &cmd2,
							&stop2, &data2);
			else
				mmc_fatfs_nonblock_reset(&mrq1, &cmd1,
							&stop1, &data1);
		}
		done_areq = cur_areq;
		cur_areq = other_areq;
		other_areq = done_areq;
		dev_addr += blocks;
	}

	done_areq = mmc_start_req(pfc->card->host, NULL, &ret);

	return ret;
err:
	return ret;
}

/*
 * Tests a basic transfer with certain parameters
 */
static int mmc_fatfs_simple_transfer(struct mmc_fatfs_card *pfc,
	struct scatterlist *sg, unsigned sg_len, unsigned dev_addr,
	unsigned blocks, unsigned blksz, int write)
{
	struct mmc_request mrq = {0};
	struct mmc_command cmd = {0};
	struct mmc_command stop = {0};
	struct mmc_data data = {0};

	mrq.cmd = &cmd;
	mrq.data = &data;
	mrq.stop = &stop;

	mmc_fatfs_prepare_mrq(pfc, &mrq, sg, sg_len, dev_addr,
		blocks, blksz, write);

	mmc_wait_for_req(pfc->card->host, &mrq);

	mmc_fatfs_wait_busy(pfc);

	return mmc_fatfs_check_result(pfc, &mrq);
}

int mmc_fatfs_probe(struct mmc_card *card)
{
	u64 size;
	char cap_str[10];
	
	
	if (!mmc_card_mmc(card) && !mmc_card_sd(card))
		return -ENODEV;

	if (!mmc_card_sd(card) && mmc_card_blockaddr(card)) {
		/*
		 * The EXT_CSD sector count is in number or 512 byte
		 * sectors.
		 */
		size = card->ext_csd.sectors;
	} else {
		/*
		 * The CSD capacity field is in units of read_blkbits.
		 * set_capacity takes units of 512 bytes.
		 */
		size = card->csd.capacity << (card->csd.read_blkbits - 9);
	}
	
	dev_info(&card->dev, "Card claimed for testing.\n");
	
	string_get_size(size << 9, STRING_UNITS_2,	cap_str, sizeof(cap_str));
	
	pr_info("%s: %s %s \n",	mmc_card_id(card), mmc_card_name(card), cap_str);
	g_fatfs_card->card = card;
	g_fatfs_card->host = card->host;
	g_fatfs_card->mmc_card_status = 1;
	g_fatfs_card->n_sectors = size;
	if (g_fatfs_card->asyncRenewState != NULL) {
		g_fatfs_card->asyncRenewState(1);
	}
	if (g_fatfs_card->asyncMount != NULL) {
		g_fatfs_card->asyncMount();
	}


	return 0;
}

DWORD mmc_fatfs_sectorcount(void)
{
	return g_fatfs_card->n_sectors;
}
EXPORT_SYMBOL_GPL(mmc_fatfs_sectorcount);

int mmc_fatfs_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
	int ret;
	struct scatterlist sg;

	if(g_fatfs_card->mmc_card_status == 0) {
		return 3;
	}

	mmc_claim_host(g_fatfs_card->host);

	if(g_fatfs_card->mmc_card_status == 0) {
		mmc_release_host(g_fatfs_card->host);       
		return 3;
	}    

	ret = mmc_fatfs_set_blksize(g_fatfs_card, 512);
	if (ret)
		return ret;

	sg_init_one(&sg, g_fatfs_card->buffer, 512*count);

	ret = mmc_fatfs_simple_transfer(g_fatfs_card, &sg, 1, sector, count, 512, 0);
	mmc_release_host(g_fatfs_card->host);

	memcpy(buff, g_fatfs_card->buffer, 512*count);

	return ret;
}
EXPORT_SYMBOL_GPL(mmc_fatfs_read);

int mmc_fatfs_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
	int ret;
	struct scatterlist sg;
	int size = 0;

	if(count*512 < BUFFER_SIZE)
	{
		size = count*512;
	}

	if(g_fatfs_card->mmc_card_status == 0) {
		return 3;
	}
	mmc_claim_host(g_fatfs_card->host);
    
	if(g_fatfs_card->mmc_card_status == 0) {
 	    mmc_release_host(g_fatfs_card->host);       
		return 3;
	}
    
	ret = mmc_fatfs_set_blksize(g_fatfs_card, 512);
	if (ret)
		return ret;


	memcpy(g_fatfs_card->buffer, buff, (512*count));

	sg_init_one(&sg, g_fatfs_card->buffer, 512*count);

//	ret = mmc_fatfs_simple_transfer(g_fatfs_card, &sg, 1, sector, count, 512, 1);
	ret = mmc_fatfs_nonblock_transfer(g_fatfs_card, &sg, 1, sector, count, 512, 1, 1);

//	printk("mmc_disk_write count[%d] sector[%d]\n", count, sector);

	mmc_release_host(g_fatfs_card->host);

	return ret;
}
EXPORT_SYMBOL_GPL(mmc_fatfs_write);

// function: detect whether card is exist
// @return:   0-card not exist; 1-card exist
int mmc_fatfs_cardstatus(void)
{
    if (g_fatfs_card->mmc_card_status == 0) {
        return 0;
    } else {
        return mmc_card_sd(g_fatfs_card->card) || mmc_card_mmc(g_fatfs_card->card);
    }
}
EXPORT_SYMBOL_GPL(mmc_fatfs_cardstatus);

void mmc_fatfs_sethandler(void* status, void* mount)
{
	if (g_fatfs_card != NULL) {
		g_fatfs_card->asyncRenewState = status;
		g_fatfs_card->asyncMount = mount;
	}
}
EXPORT_SYMBOL_GPL(mmc_fatfs_sethandler);

void mmc_fatfs_remove(struct mmc_card *card)
{
	g_fatfs_card->mmc_card_status = 0;
	g_fatfs_card->n_sectors = 0;
	if (g_fatfs_card->asyncRenewState != NULL) {
		g_fatfs_card->asyncRenewState(0);
	}
}

int  mmc_fatfs_init(void)
{
	g_fatfs_card = kzalloc(sizeof(struct mmc_fatfs_card), GFP_KERNEL);
	g_fatfs_card->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);

	return 0;
}

void  mmc_fatfs_exit(void)
{
	kfree(g_fatfs_card->buffer);
	kfree(g_fatfs_card);
}

