#include <linux/dma-mapping.h>
#include <linux/vmalloc.h>
#include <linux/i2c.h>
#include "semi_touch_interface.h"

#define HAL_MAX_TRY         1

#if SEMI_TOUCH_DMA_TRANSFER

#define DMA_BUFFER_LENGTH                   (MAX_IO_BUFFER_LEN + 4)
unsigned char *dma_buff_virtual_addr = NULL;
dma_addr_t dma_buff_physical_addr = 0;

int i2c_write_bytes(struct hal_io_packet* ppacket)
{
    int ret, retry;
    struct i2c_client* client = ppacket->hal_adapter;
    
    memcpy(dma_buff_virtual_addr, (unsigned char*)ppacket, ppacket->io_length + sizeof(int));
    client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
    for(retry = 0; retry < HAL_MAX_TRY; retry++)
    {
        ret = i2c_master_send(client, (unsigned char *)dma_buff_physical_addr, ppacket->io_length + sizeof(int));
        if(ppacket->io_length + sizeof(int) == ret) break;
    }
    client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);
    
    return ret;
}

int i2c_read_bytes(struct hal_io_packet* ppacket)
{
    int ret, retry;
    struct i2c_client* client = ppacket->hal_adapter;
    
    memcpy(dma_buff_virtual_addr, (unsigned char*)ppacket, sizeof(int));
    client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
    for(retry = 0; retry < HAL_MAX_TRY; retry++)
    {
        ret = i2c_master_send(client, (unsigned char *)dma_buff_physical_addr, sizeof(int));
        if(sizeof(int) == ret) break;
    }
    client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);
    
    client->addr = ((client->addr & I2C_MASK_FLAG) | I2C_DMA_FLAG);
    for(retry = 0; retry < HAL_MAX_TRY; retry++)
    {
        ret = i2c_master_recv(client, (unsigned char *)dma_buff_physical_addr, ppacket->io_length);
        if(ppacket->io_length == ret) break;
    }
    memcpy(ppacket->io_buffer, dma_buff_virtual_addr, ppacket->io_length);
    client->addr = client->addr & I2C_MASK_FLAG & (~ I2C_DMA_FLAG);
    
    return ret;
}

int semi_touch_i2c_init(void)
{
    if (NULL == dma_buff_virtual_addr) 
    {
        st_dev.input->dev.coherent_dma_mask = DMA_BIT_MASK(32);
        dma_buff_virtual_addr = (u8 *)dma_alloc_coherent(&st_dev.input->dev, DMA_BUFFER_LENGTH, &dma_buff_physical_addr, GFP_KERNEL);

        if (!dma_buff_virtual_addr) 
        {
            kernel_log_d("Allocate DMA I2C Buffer failed!!");
            return -ENOMEM;
        }
    }

    return 0;
}

int semi_touch_i2c_i2c_exit(void)
{
    if (dma_buff_virtual_addr) 
    {
        dma_free_coherent(NULL, DMA_BUFFER_LENGTH, dma_buff_virtual_addr, dma_buff_physical_addr);
        dma_buff_virtual_addr = NULL;
        dma_buff_physical_addr = 0;
        kernel_log_d("Allocated DMA I2C Buffer release!!");
    }

    return 0;
}

#else
int i2c_write_bytes(struct hal_io_packet* ppacket)
{
    int ret, retry;
    struct i2c_msg msg;
    struct i2c_client* client = ppacket->hal_adapter;

    msg.addr  = client->addr;
    msg.flags = 0;
    msg.buf = (unsigned char*)ppacket;
    msg.len = ppacket->io_length + sizeof(int);

    for(retry = 0; retry < HAL_MAX_TRY; retry++)
    {
        ret = i2c_transfer(client->adapter, &msg, 1);
        if(1 == ret) break;
    }

    return ret;
}
int i2c_read_bytes(struct hal_io_packet* ppacket)
{
    int ret, retry;
    struct i2c_msg msg[2];
    struct i2c_client* client = ppacket->hal_adapter;

    msg[0].addr  = client->addr;
    msg[0].flags = 0;
    msg[0].buf = (unsigned char*)ppacket;
    msg[0].len = sizeof(int);
    msg[1].addr  = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = ppacket->io_buffer;
    msg[1].len = ppacket->io_length;

    for(retry = 0; retry < HAL_MAX_TRY; retry++)
    {
        ret = i2c_transfer(client->adapter, msg, 2);
        if(2 == ret) break;
    }

    return ret;
}
int semi_touch_i2c_init(void)
{
    return 0;
}
int semi_touch_i2c_i2c_exit(void)
{
    return 0;
}
#endif //SEMI_TOUCH_DMA_TRANSFER

