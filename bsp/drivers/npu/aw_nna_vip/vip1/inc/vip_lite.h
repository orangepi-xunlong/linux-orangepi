/******************************************************************************\
|* Copyright (c) 2017-2023 by Vivante Corporation.  All Rights Reserved.      *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
\******************************************************************************/

#ifndef _VIP_LITE_H
#define _VIP_LITE_H

#include "vip_lite_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

/*!
 *\brief The VIP lite API for Convolution Neural Network application on CPU/MCU/DSP type of embedded environment.
 *\details This VIP lite APIs is not thread-safe if vpmdENABLE_MULTIPLE_TASK is set to 0,
           user must guarantee to call these APIs in a proper way.
           But defines vpmdENABLE_MULTIPLE_TASK 1, VIPLite can support multiple task(multiple thread/process).
           and it's thread-safe.
 *Memory allocation and file io functions used inside driver internal would depend on working enviroment.

 *\defgroup group_global   Data Type Definitions and Global APIs,
 *\                        brief Data type definition and global APIs that are used in the VIPLite
 *\defgroup group_buffer   Buffer API,
                           The API to manage input/output buffers
 *\defgroup group_network  Network API
                           The API to manage networks
 */

/* !\brief The data format list for buffer
 * \ingroup group_buffer
 * \version 2.0
 */
typedef enum _vip_buffer_format_e
{
    /*! \brief A float type of buffer data */
    VIP_BUFFER_FORMAT_FP32       = 0,
    /*! \brief A half float type of buffer data */
    VIP_BUFFER_FORMAT_FP16       = 1,
    /*! \brief A 8 bit unsigned integer type of buffer data */
    VIP_BUFFER_FORMAT_UINT8      = 2,
    /*! \brief A 8 bit signed integer type of buffer data */
    VIP_BUFFER_FORMAT_INT8       = 3,
    /*! \brief A 16 bit unsigned integer type of buffer data */
    VIP_BUFFER_FORMAT_UINT16     = 4,
    /*! \brief A 16 signed integer type of buffer data */
    VIP_BUFFER_FORMAT_INT16      = 5,
    /*! \brief A char type of data */
    VIP_BUFFER_FORMAT_CHAR       = 6,
    /*! \brief A bfloat 16 type of data */
    VIP_BUFFER_FORMAT_BFP16      = 7,
    /*! \brief A 32 bit integer type of data */
    VIP_BUFFER_FORMAT_INT32      = 8,
    /*! \brief A 32 bit unsigned signed integer type of buffer */
    VIP_BUFFER_FORMAT_UINT32     = 9,
    /*! \brief A 64 bit signed integer type of data */
    VIP_BUFFER_FORMAT_INT64      = 10,
    /*! \brief A 64 bit unsigned integer type of data */
    VIP_BUFFER_FORMAT_UINT64     = 11,
    /*! \brief A 64 bit float type of buffer data */
    VIP_BUFFER_FORMAT_FP64       = 12,
    /*! \brief A signed 4bits tensor */
    VIP_BUFFER_FORMAT_INT4       = 13,
    /*! \brief A unsigned 4bits tensor */
    VIP_BUFFER_FORMAT_UINT4      = 14,
}   vip_buffer_format_e;

/* !\brief The quantization format list for buffer data
  * \ingroup group_buffer
  * \version 1.0
 */
typedef enum _vip_buffer_quantize_format_e
{
    /*! \brief Not quantized format */
    VIP_BUFFER_QUANTIZE_NONE                    = 0,
    /*! \brief A quantization data type which specifies the fixed point position for whole tensor. */
    VIP_BUFFER_QUANTIZE_DYNAMIC_FIXED_POINT     = 1,
    /*! \brief A quantization data type which has scale value and zero point to match with TF and
               Android NN API for whole tensor. */
    VIP_BUFFER_QUANTIZE_TF_ASYMM                = 2,
    /*! \brief A max vaule support quantize format */
    VIP_BUFFER_QUANTIZE_MAX,
}   vip_buffer_quantize_format_e;

/* !\brief The memory type for vip buffer
  * \ingroup group_buffer
  * \version 1.2.2
 */
typedef enum _vip_buffer_memory_type_e
{
    /*! \brief Not memory type. default memory type.
        use for allocate video memory from driver calling vip_create_buffer.
    */
    VIP_BUFFER_MEMORY_TYPE_DEFAULT              = 0x000,
    /*! \brief Create a VIP buffer from the Host (logical, physical). */
    VIP_BUFFER_MEMORY_TYPE_HOST                 = 0x001,
    /*! \brief Create a VIP buffer from DMA_BUF */
    VIP_BUFFER_MEMORY_TYPE_DMA_BUF              = 0x003,
    /*! \brief The max memory type */
    VIP_BUFFER_MEMORY_TYPE_MAX,
}   vip_buffer_memory_type_e;

/* \brief The list of create network type
  * \ingroup group_network
  * \version 1.0
 */
typedef enum _vip_create_network_type_e
{
    /*!< \brief NONE */
    VIP_CREATE_NETWORK_FROM_NONE        = 0x00,
    /*!< \brief Create network from a file path */
    VIP_CREATE_NETWORK_FROM_FILE        = 0x01,
    /*!< \brief Create network from buffer, NBG has been loaded in this buffer before */
    VIP_CREATE_NETWORK_FROM_MEMORY      = 0x02,
    /*!< \brief Create network from flash */
    VIP_CREATE_NETWORK_FROM_FLASH       = 0x04,

    VIP_CREATE_NETWORK_MAX,
} vip_create_network_type_e;

/* \brief The list of duplicate network type.
    The original network can't be destroy if the dup network is running or will be run later.
  * \ingroup group_network
 */
typedef enum _vip_dup_network_type_e
{
    /*!< \brief NONE */
    VIP_DUP_NONE                   = 0x00,
    /*!< \brief Duplicate command for sharing weight with another network
        1. Sharing weight with original network.
        2. The original network has the same input/output shape as the dup network.
        3. Only the input/output addresses of network are difference between the original network with dup network.
    */
    VIP_DUP_FOR_CMD_BY_NETWORK     = 0x01,
    /*!< \brief Duplicate command for sharing weight with difference network(NBGs)
        1. Sharing weight with original network.
        2. Share weight between networks with the same network topology.
        For example, to support different shapes of input, such as 640x480, 480x640 and 640x960.
    */
    VIP_DUP_FOR_CMD_BY_NBG         = 0x02,

    /*!< \brief Indicate that the dup network is duplicated from NBG file patch */
    VIP_DUP_FROM_NBG_FILE          = 0x100,
    /*!< \brief Indicate that the dup network is duplicated from NBG in memory */
    VIP_DUP_FROM_NBG_MEMORY        = 0x200,
    /*!< \brief Indicate that the dup network is duplicated from NBG in flash */
    VIP_DUP_FROM_NBG_FLASH         = 0x400,
    /*!< \brief Indicate that the dup network is duplicated from network object */
    VIP_DUP_FROM_NETWORK           = 0x800,

    VIP_DUP_NETWORK_MAX,
} vip_dup_network_type_e;

/* \brief An enumeration property that specifies which power management operation to execute.
  * \ingroup group_global
  * \version 1.2
 */
typedef enum _vip_power_property_e
{
    VIP_POWER_PROPERTY_NONE          = 0x0000,
    /*!< \brief specify the VIP frequency */
    VIP_POWER_PROPERTY_SET_FREQUENCY = 0x0001,
    /*!< \brief power off VIP hardware */
    VIP_POWER_PROPERTY_OFF           = 0x0002,
    /*!< \brief power on VIP hardware */
    VIP_POWER_PROPERTY_ON            = 0x0004,
    /*!< \brief stop VIP perform network */
    VIP_POWER_PROPERTY_STOP          = 0x0008,
    /*!< \brief start VIP perform network */
    VIP_POWER_PROPERTY_START         = 0x0010,
    VIP_POWER_PROPERTY_MAX
} vip_power_property_e;

/* \brief query hardware caps property
  * \ingroup group_global
 */
typedef enum _vip_query_hardware_property_e
{
    /*!< \brief the customer ID of this VIP/NPU, the returned value is vip_uint32_t */
    VIP_QUERY_HW_PROP_CID                = 0,
    /*!< \brief the number of deivce, the returned value is vip_uint32_t */
    VIP_QUERY_HW_PROP_DEVICE_COUNT       = 1,
    /*!< \brief the number of core count for each device, the returned value is vip_uint32_t * device_count */
    VIP_QUERY_HW_PROP_CORE_COUNT_EACH_DEVICE  = 2,
    VIP_QUERY_HW_PROP_MAX,
}  vip_query_hardware_property_e;

/* \brief The list of properties of a network.
  * \ingroup group_network
  * \version 1.0
 */
typedef enum _vip_network_property_e
{
    /* query network */
    /*!< \brief The number of layers in this network, the returned value is vip_uint32_t */
    VIP_NETWORK_PROP_LAYER_COUNT  = 0,
    /*!< \brief The number of input in this network, the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_INPUT_COUNT  = 1,
    /*!< \brief The number of output in this network, the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_OUTPUT_COUNT = 2,
    /*!< \brief The network name, the returned value is vip_char_t[64] */
    VIP_NETWORK_PROP_NETWORK_NAME = 3,
    /*!< \brief address information of wait-link, command, input-output buffers for viplite-Agent trigger,
          not used if only use viplite. the returned value is <tt>\ref vip_address_info_t
          */
    VIP_NETWORK_PROP_ADDRESS_INFO = 4,
    /*!< \brief read interruput irq register value for cleaning up IRQ.
            the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_READ_REG_IRQ = 5,
    /*!< \brief The size of memory pool, the returned value is vip_uint32_t*/
    VIP_NETWORK_PROP_MEMORY_POOL_SIZE = 6,

    /*!< \brief The network profling data, the returned value is vip_inference_profile_t */
    VIP_NETWORK_PROP_PROFILING = 7,

    /*!< \brief The the number of core for this network, the returned value is vip_uint8_t */
    VIP_NETWORK_PROP_CORE_COUNT  = 8,

    /* set network */
    /* set network property should be called before vip_prepare_network */
    /*!< \brief set network to enable change PPU parameters feature for this vip_network.
            the vip_set_network value param used to indicates disable or enable this feature.
            vip_uint32_t *value is 1, enable change ppu param.
            vip_uint32_t *value is 0, disable change ppu param */
    VIP_NETWORK_PROP_CHANGE_PPU_PARAM  = 64,
    /*!< \brief set memory pool buffer for network. networks can share a memory pool buffer.
         the set value is <tt>\ref vip_buffer<tt>  */
    VIP_NETWORK_PROP_SET_MEMORY_POOL   = 65,
    /*!< \brief set device id for network. networks can be submitted this vip device. */
    VIP_NETWORK_PROP_SET_DEVICE_ID     = 66,
    /*!< \brief set priority of network. 0 ~ 255, 0 indicates the lowest priority. */
    VIP_NETWORK_PROP_SET_PRIORITY      = 67,
    /*!< \brief set time out of network. unit: ms */
    VIP_NETWORK_PROP_SET_TIME_OUT      = 68,
    /*!< \brief set a memory for partial of full pre-load coeff data to this memory.
       This memory can't be freed until the network is released. */
    VIP_NETWORK_PROP_SET_COEFF_MEMORY  = 69,
}  vip_network_property_e;

/* \brief The list of properties of a group.
  * \ingroup group_network
  * \version 1.0
 */
typedef enum _vip_group_property_e
{
    /* query group */
    /*!< \brief The group profling data, the returned value is vip_inference_profile_t */
    VIP_GROUP_PROP_PROFILING = 0,

    /* set group */
    /* set group property should be called before vip_add_network()
       and all network in group runs on same device */
    /*!< \brief set device id for group. networks in group can be submitted this vip device.
     * This prop should be called before vip_prepare_network */
    VIP_GROUP_PROP_SET_DEVICE_ID     = 64,
    /*!< \brief setting inference timeout value for group. unit: ms */
    VIP_GROUP_PROP_SET_TIME_OUT      = 68,
}  vip_group_property_e;

/* \brief The list of property of an input or output.
  * \ingroup group_buffer
  * \version 1.0
 */
typedef enum _vip_buffer_property_e
{
    /*!< \brief The quantization format, the returned value is <tt>\ref vip_buffer_quantize_format_e </tt> */
    VIP_BUFFER_PROP_QUANT_FORMAT         = 0,
    /*!< \brief The number of dimension for this input, the returned value is vip_uint32_t */
    VIP_BUFFER_PROP_NUM_OF_DIMENSION     = 1,
    /*!< \brief The size of each dimension for this input, the returned value is vip_uint32_t * num_of_dim */
    VIP_BUFFER_PROP_SIZES_OF_DIMENSION   = 2,
    /*!< \brief The data format for this input, the returned value is <tt>\ref vip_buffer_format_e</tt> */
    VIP_BUFFER_PROP_DATA_FORMAT          = 3,
    /*!< \brief The position of fixed point for dynamic fixed point, the returned value is vip_uint8_t */
    VIP_BUFFER_PROP_FIXED_POINT_POS      = 4,
    /*!< \brief The scale value for TF quantization format, the returned value is vip_float_t */
    VIP_BUFFER_PROP_TF_SCALE             = 5,
    /*!< \brief The zero point for TF quantization format, the returned value is vip_uint32_t */
    VIP_BUFFER_PROP_TF_ZERO_POINT        = 6,
    /*!< \brief The name for network's inputs and outputs, the returned value is vip_char_t[64] */
    VIP_BUFFER_PROP_NAME                 = 7,
}   vip_buffer_property_e;

/* \brief The list of property of operation vip_buffer type.
  * \ingroup group_buffer
  * \version 1.3
 */
typedef enum _vip_buffer_operation_type_e
{
    /*!< \brief None operation </tt> */
    VIP_BUFFER_OPER_TYPE_NONE         = 0,
     /*!< \brief Flush the vip buffer </tt> */
    VIP_BUFFER_OPER_TYPE_FLUSH        = 1,
    /*!< \brief invalidate the vip buffer </tt> */
    VIP_BUFFER_OPER_TYPE_INVALIDATE   = 2,
    VIP_BUFFER_OPER_TYPE_MAX,
} vip_buffer_operation_type_e;

typedef struct _vip_network  *vip_network;
typedef struct _vip_buffer   *vip_buffer;
typedef struct _vip_group    *vip_group;


/*! \brief Input parameter for vip_create_buffer
  * \ingroup group_buffer
 */
typedef struct _vip_buffer_create_params_t
{
     /*!< \brief The number of dimensions specified in *sizes*/
    vip_uint32_t   num_of_dims;
    /*!< \brief The pointer to an array of dimension */
    vip_uint32_t   sizes[6];
    /*!< \brief Data format for the tensor, see <tt>\ref vip_buffer_format_e </tt> */
    vip_enum       data_format;
    /*!< \brief Quantized format see <tt>\ref vip_buffer_quantize_format_e </tt>. */
    vip_enum       quant_format;
    /*<! \brief The union of quantization information */
    union {
        struct {
            /*!< \brief Specifies the fixed point position when the input element type is int16,
                        if 0 calculations are performed in integer math */
            vip_int32_t fixed_point_pos;
        } dfp;

        struct {
            /*!< \brief Scale value for the quantized value */
            vip_float_t        scale;
            /*!< \brief  A 32 bit integer, in range [0, 255] */
            vip_int32_t        zeroPoint;
        } affine;
     }
     quant_data;
    /*<! \brief The type of this VIP buffer memory. see vip_buffer_memory_type_e.*/
    vip_uint32_t memory_type;
} vip_buffer_create_params_t;

typedef struct _vip_power_frequency_t
{
    /*
        The VIP core clock scale percent. fscale_percent value should be 1~~100.
        100 means that full clock frequency.
        1 means that minimum clock frequency.
    */
    vip_uint8_t fscale_percent;
} vip_power_frequency_t;

/*! \brief network performance parameter
 * \ingroup group_network
*/
typedef struct _vip_inference_profile_t
{
    /* the time of inference the network, unit us, microsecond */
    vip_uint32_t inference_time;
    /* the VIP's cycle of inference the network */
    vip_uint32_t total_cycle;
} vip_inference_profile_t;

/*! \brief change PPU parameters
 * \ingroup group_network
*/
typedef struct _vip_ppu_param_t
{
    /* work-dim should equal to PPU application work-dim seeting when generating NBG file */
    vip_uint32_t work_dim;
    vip_uint32_t global_offset[3];
    vip_uint32_t global_scale[3];
    vip_uint32_t local_size[3];
    vip_uint32_t global_size[3];
} vip_ppu_param_t;


/***** API Prototypes. *****/

/*! \brief  Get VIPLite driver version.
 * \return <tt>\ref vip_uint32_t </tt>
 * \ingroup group_global
 */
VIP_API
vip_uint32_t vip_get_version(
    void
    );

/*! \brief  Initial VIP Hardware, VIP lite software environment and power on VIP hardware.
 * \details when vpmdENABLE_MULTIPLE_TASK set to 0,
            This function should be only called once before using VIP hardware if.
            when vpmdENABLE_MULTIPLE_TASK set to 1,
            vip_init can be called multiple times, but should paired with vip_destroy.
            vip_init should be called in every process.
            only need call vip_init once in multi-thread.
 * VIP lite driver would construct some global variable for this call.Also
 * it will reset VIP and initialize VIP hardware to a ready state to accept jobs.
 * \return <tt>\ref vip_status_e </tt>
 * \ingroup group_global
 * \version 1.0
 */
VIP_API
vip_status_e vip_init(
    void
    );

/*! \brief Terminate VIP lite driver and shut down VIP hardware.
 * \details This function should be the last function called by application.
            vip_destroy should paired with vip_init called.
 * After it, no VIP lite API should be called except <tt>\ref vip_init </tt>
 * \return <tt>\ref vip_status_e </tt>
 * \ingroup group_global
 * \version 1.0
 * \notes vip_destroy should be called in the same thread as vip_init.
 */
VIP_API
vip_status_e vip_destroy(
    void
    );

/*! \brief Queries hardware caps information. This function shold be called after calling vip_init.
 *\param property, the query property enum.
 *\param size, the size of value buffer.
 *\param value, the value buffer of returns.
 * \ingroup group_global
*/
VIP_API
vip_status_e vip_query_hardware(
    IN vip_query_hardware_property_e property,
    IN vip_uint32_t size,
    OUT void *value
    );

/*! \brief Create a input or output buffer with specified parameters.
 *\details The buffer object always takes [w, h, c, n] order,
           there is no padding/hole between lines/slices/batches.
 *\param [in] create_param The pointer to <tt>\ref vip_buffer_create_params_t </tt> structure.
 *\param [in] size_of_param The size of create_param pointer.
 *\param [out] buffer  An opaque handle for the new buffer object if the request is executed successfully.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
 */
VIP_API
vip_status_e vip_create_buffer(
    IN vip_buffer_create_params_t *create_param,
    IN vip_uint32_t size_of_param,
    OUT vip_buffer *buffer
    );

/*! \brief Create a buffer from user contiguous or scatter non-contiguous physical address.
          the vip_buffer created by this APi doesn't support flush CPU cache in driver.
          So the physical memory should be a non-cache buffer or flush CPU on Host control.
          not map user space logical on Linux.
 *\param [in] create_param The pointer to <tt>\ref vip_buffer_create_params_t </tt> structure.
 *\param [in] physical_table Physical address table. should be wraped for VIP hardware.
 *\param [in] size_table The size of physical memory for each physical_table element.
 *\param [in] physical_num The number of physical table element.
              physical_num is 1 when create buffer from contiguous phyiscal.
 *\param [out] buffer. vip lite buffer object.
 *\return <tt>\ref vip_status_e </tt>
*\ingroup group_buffer
*/
VIP_API
vip_status_e vip_create_buffer_from_physical(
    IN const vip_buffer_create_params_t *create_param,
    IN const vip_address_t *physical_table,
    IN const vip_uint32_t *size_table,
    IN vip_uint32_t physical_num,
    OUT vip_buffer *buffer
    );

/*! \brief Create a vip buffer with specified parameters.
           The vip_buffer can be used to input, output, memory pool and so on.
           NOTE: driver will operation CPU cache when call vip_flush_buffer API.
           application should call vip_flush_buffer API if the memory handle have CPU cache.
 after write data into this buffer, APP should call vip_flush_buffer(VIP_BUFFER_OPER_TYPE_FLUSH)
 before CPU read date from this buffer. APP should call vip_flush_buffer(VIP_BUFFER_OPER_TYPE_INVALIDATE)
 *\ when MMU disabled, create buffer from a contiguous physical memory.
 *\ when MMU enabled, create buffer from a contiguous physical memory or
         logical address(convert to physical in kenrel pace).
 *\details The buffer object always takes [w, h, c, n] order,
           there is no padding/hole between lines/slices/batches.
 *\param [in] create_param The pointer to <tt>\ref vip_buffer_create_params_t </tt> structure.
 *\param [in] handle_logical The logical address of the handle.
              create vip buffer from the logical address.
 *\param [in] the handle_size should be aligned to 64byte(vpmdCPU_CACHE_LINE_SIZE) for easy flash CPU cache.
 *\param [out] buffer. vip lite buffer object.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.1
*/
VIP_API
vip_status_e vip_create_buffer_from_handle(
    IN const vip_buffer_create_params_t *create_param,
    IN const vip_ptr handle_logical,
    IN vip_uint32_t handle_size,
    OUT vip_buffer *buffer
    );

/*! \brief Create a vip buffer from user fd(file descriptor).
         only  support create buffer from dma-buf on Linux.
         the vip_buffer created by this APi doesn't support flush CPU cache in driver.
         So the dma-buf should be a non-cache buffer or flush CPU on Host control.
 *\param [in] create_param The pointer to <tt>\ref vip_buffer_create_params_t </tt> structure.
 *\param [in] fd user memory file descriptor.
 *\param [in] memory_size The size of user memory.
              the handle_size should be aligned to 64byte(vpmdCPU_CACHE_LINE_SIZE) for easy flash CPU cache.
 *\param [out] buffer. vip lite buffer object.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
*/
VIP_API
vip_status_e vip_create_buffer_from_fd(
    IN const vip_buffer_create_params_t *create_param,
    IN vip_uint32_t fd,
    IN vip_uint32_t memory_size,
    OUT vip_buffer *buffer
    );

/*! \brief  Destroy a buffer object which was created before.
 *\param [in] buffer The opaque handle of buffer to be destroyed.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_status_e vip_destroy_buffer(
    IN vip_buffer buffer
    );

/*! \brief Map a buffer to get the CPU accessible address for read or write
 *\param [in] buffer The handle of buffer to be mapped.
 *\return A pointer that application can use to read or write the buffer data.
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
void * vip_map_buffer(
    IN vip_buffer buffer
    );

/*! \brief Unmap a buffer which was mapped before.
 *\param [in] buffer The handle of buffer to be unmapped.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_status_e vip_unmap_buffer(
    IN vip_buffer buffer
    );

/*! \brief Get the size of bytes allocated for the buffer.
 *\param [in] buffer The handle of buffer to be queried.
 *\return <tt>\ref the size of bytes </tt>
 *\ingroup group_buffer
 *\version 1.0
*/
VIP_API
vip_uint32_t vip_get_buffer_size(
    IN vip_buffer buffer
    );

/*! \brief operation the vip buffer CPU chace. flush, invalidate cache.
  You should call vip_flush_buffer to flush buffer for input.
  and invalidate buffer for network's output if these memories with CPU cache.
*\param buffer The vip buffer object.
*\param the type of this operation. see vip_buffer_operation_type_e.
*\ingroup group_buffer
*/
VIP_API
vip_status_e vip_flush_buffer(
    IN vip_buffer buffer,
    IN vip_buffer_operation_type_e type
    );

/*! \brief Create a network object from the given binary data.
 *\details The binary is generated by the binary graph generator and it's a blob binary.
 *\VIP lite Driver could interprete it to create a network object.
 *\param [in] data The pointer to the binary graph. it can be a file path or a memory pointer, depending on type.
 *\param [in] size_of_data The byte size of data object. the byte size of NBG buffer.
              You can ignore it if create network form fil path.
 *\param [in] type how to create a network object. please refer to vip_create_network_type_e enum.
 *\param [out] network An opaque handle to the new network object if the request is executed successfully
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_create_network(
    IN const void *data,
    IN vip_uint32_t size_of_data,
    IN vip_create_network_type_e type,
    OUT vip_network *network
    );

/*
@brief  Duplicate a network for sharing weight.
        The original network can't be destroy if the dup network is running or will be run later.
@param  data, NBG file path, the buffer of NBG
              different data according to type(vip_dup_network_type_e) is from file/memory/flash.
              The data object can be set to VIP_NULL when type is VIP_DUP_NETWORK_FROM_NETWORK.
@param  size_of_data, the bytes size of data. the byte size of NBG buffer.
    the size can be set to 0 when *data is NBG file path.
@param  type, vip_dup_network_type_e.
*\param  network, original network to be dup.
*\param  dup_network, network object which created by duplicated.
*/
VIP_API
vip_status_e vip_dup_network(
    IN const void *data,
    IN vip_uint32_t size_of_data,
    IN vip_dup_network_type_e type,
    IN vip_network network,
    OUT vip_network *dup_network
    );

/*! \brief Weak dup a vip_network object.
    The weak dup netowrk copy new command buffer. and share coefficient data and ppu instruction with original network.
    Notes:
      1. The original network can't be destroy if the weak dup network is running or will be run later.
      2. The original network has the same input/output shape as the dup network.
      3. Only the input/output addresses of network are difference between the original network with dup network.
    eg: Used to support batch network.
*\param network, original network to be dup.
*\param dup_network, output network.
*\return <tt>\ref vip_status_e </tt>
*\version 1.0
*/
VIP_API
vip_status_e vip_weak_dup_network(
    IN vip_network network,
    OUT vip_network *dup_network
    );

/*! \brief Destroy a network object
 *\details Release all resources allocated for this network.
 *\param [in] network The opaque handle to the network to be destroyed
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_destroy_network(
    IN vip_network network
    );

/*! \brief Configure network property. configure network. this API should be called before calling vip_prepare_network.
 *\details Configure network's layer inputs/outputs information
 *\param [in] network A property <tt>\ref vip_network_property_e </tt> to be configuied.
 *\return <tt>\ref vip_status_e </tt>
 */
VIP_API
vip_status_e vip_set_network(
    IN vip_network network,
    IN vip_enum property,
    IN void *value
    );

/*! \brief Query a property of the network object.
 *\details User can use this API to get any properties from a network.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] property A property <tt>\ref vip_network_property_e </tt> to be queried.
 *\param [out] value A pointer to memory to store the return value,
               different property could return different type/size of value.
 * please see comment of <tt>\ref vip_network_property_e </tt> for detail.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_network(
    IN vip_network network,
    IN vip_enum property,
    OUT void *value
    );

/*! \brief Prepare a network to run on VIP.
 *\details This function only need to be called once to prepare a network and make it ready to execute on VIP hardware.
 * It would do all heavy-duty work, including allocate internal memory resource for this network,
   deploy all operation's resource
 * to internal memory pool, allocate/generate command buffer for this network,
   patch command buffer for the resource in the internal memory
 * allocations. If this function is called more than once, driver will silently ignore it.
                If this function is executed successfully, this network is prepared.
 *\param [in] network The opaque handle to the network which need to be prepared.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_prepare_network(
    IN vip_network network
    );

/*! \brief Query a property of a specific input of a given network.
 *\details The specified input/property/network must be valid, otherwise VIP_ERROR_INVALID_ARGUMENTS will be returned.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] index Specify which input to be queried in case there are multiple inputs in the network
 *\param [in] property Specify which property application wants to know, see <tt>\ref vip_buffer_property_e </tt>
 *\param [out] value Returned value, the details type/size, please refer to the comment of <tt>\ref vip_input_property_e </tt>
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_input(
    IN vip_network network,
    IN vip_uint32_t index,
    IN vip_enum property,
    OUT void *value
    );

/*! \brief Query a property of a specific output of a given network.
 *\details The specified output/property/network must be valid, otherwise VIP_ERROR_INVALID_ARGUMENTS will be returned.
 *\param [in] network The opaque handle to the network to be queried
 *\param [in] index Specify which output to be queried in case there are multiple outputs in the network
 *\param [in] property Specify which property application wants to know, see <tt>\ref vip_buffer_property_e </tt>
 *\param [out] value Returned value, the details type/size, please refer to the comment of <tt>\ref vip_input_property_e </tt>
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_output(
    IN vip_network network,
    IN vip_uint32_t index,
    IN vip_enum property,
    OUT void *value
    );

/*! \brief Attach an input buffer to the specified index of the network.
 *\details All the inputs of the network need to be attached to a valid input buffer before running a network, otherwise
 * VIP_ERROR_MISSING_INPUT_OUTPUT will be returned when calling <tt> \ref vip_run_network </tt>.
   When attaching an input buffer
 * to the network, driver would patch the network command buffer to fill in this input buffer address.
   This function could be called
 * multiple times to let application update the input buffers before next network execution.
   The network must be prepared by <tt>\ref vip_prepare_network </tt> before
 * attaching an input.
 *\param [in] network The opaque handle to a network which we want to attach an input buffer
 *\param [in] index The index specify which input in the network will be set
 *\param [in] input The opaque handle to a buffer which will be attached to the network.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_set_input(
    IN vip_network network,
    IN vip_uint32_t index,
    IN vip_buffer input
    );

/*! \brief Attach an output buffer to the specified index of the network.
 *\details All the outputs of the network need to be attached to a valid output buffer before running a network, otherwise
 * VIP_ERROR_MISSING_INPUT_OUTPUT will be returned when calling <tt> \ref vip_run_network </tt>.
    When attaching an output buffer
 * to the network, driver would patch the network command buffer to fill in this output buffer address.
    This function could be called
 * multiple times to let application update the output buffers before next network execution.
   The network must be prepared by <tt>\ref vip_prepare_network </tt> before
 * attaching an output.
 *\param [in] network The opaque handle to a network which we want to attach an output buffer
 *\param [in] index The index specify which output in the network will be set
 *\param [in] output The opaque handle to a buffer which will be attached to the network.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_set_output(
    IN vip_network network,
    IN vip_uint32_t index,
    IN vip_buffer output
    );

/*! \brief. Kick off the network execution and send command buffer of this network to VIP hardware.
 *\details This function can be called multiple times.
           Every time it's called it would do inference with current attached
 * input buffers and output buffers. It would return until VIP finish the execution.
         If the network is not ready to execute
 * for some reason like not be prepared by <tt>\ref vip_prepare_network </tt>,
   it would fail with status reported.
 *\param [in] network The opaque handle to the network to be executed.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_run_network(
    IN vip_network network
    );

/*! \brief Finish using this network to do inference.
 *\details This function is paired with <tt>\ref vip_prepare_network </tt>.
           It's suggested to be called once after <tt>\ref vip_prepare_network </tt> called.
 * If it's called more than that, it will be silently ignored.
   If the network is not prepared but finished is called, it's silently ignored too.
 * This function would release all internal memory allocations which are allocated when
   the network is prepared. Since the preparation of network takes much time,
 * it is suggested that if the network will be still used later, application should not
   finish the network unless there is no much system resource remained for other
 * networks. The network object is still alive unitl it's destroyed by <tt>\ref vip_destroy_network </tt>.
 *\param [in] network The opaque handle to the network which will be finished.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_finish_network(
    IN vip_network network
    );

/*! \brief. Kick off the network execution and send command buffer of this network to VIP hardware.
*\details This function is similar to <tt>\ref vip_run_network </tt> except that it returns
          immediately without waiting for HW to complete the commands.
*\param [in] network The opaque handle to the network to be executed.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_trigger_network(
    IN vip_network network
    );

/*! \brief. Run tasks in group,these tasks is added by vip_add_network.
            The order of executuion of tasks is call vip_add_network.
*\details This function is similar to <tt>\ref vip_run_group </tt> except that it returns
          immediately without waiting for HW to complete the commands.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object
*\param the number of task will be run.
        eg: num is 4, the 0, 1, 2, 3 taks index in group will be run(inference).
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_trigger_group(
    IN vip_group group,
    IN vip_uint32_t num
    );

/*! \brief. Explicitly wait for HW to finish executing the submitted commands.
*\details This function waits for HW to complete the commands.
          This should be called once CPU needs to access the network currently being run.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_wait_network(
    IN vip_network network
    );

/*! \brief. Explicitly wait for HW to finish executing the submitted task in group.
*\details This function waits for HW to complete the submitted commands in group.
          This should be called once CPU needs to access the group currently being run.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_wait_group(
    IN vip_group group
    );

/*! \brief. Cancle network running on vip hardware after network is commited.
*\details This function is cancel network running on vip hardware.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_cancel_network(
    IN vip_network network
    );

/*! \brief. give user applications more control over power management for VIP cores.
*\details. control VIP core frequency and power status by property. see vip_power_property_e.
*\param ID of the managed device. device_id is 0 if VIP is single core.
*\param perperty Control VIP core frequency and power status by property. see vip_power_property_e.
*\param value The value for vip_power_property_e property.
       Please see vip_power_frequency_t if property is setting to VIP_POWER_PROPERTY_SET_FREQUENCY.
*\return <tt>\ref vip_status_e </tt>
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_power_management(
    IN vip_uint32_t device_id,
    IN vip_power_property_e property,
    IN void *value
    );

/*! \brief. Create a vip_group object to run multiple tasks(network or node)
            and without interrupt between each task.
*\return <tt>\ref vip_status_e </tt>
*\param count The maximum number of tasks supports by this group.
*\param group Return vip_group object be created.
*\version 1.0
*/
VIP_API
vip_status_e vip_create_group(
    IN vip_uint32_t count,
    OUT vip_group *group
    );

/*! \brief. Destroy group object which created by vip_create_group.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object/
*\version 1.0
*/
VIP_API
vip_status_e vip_destroy_group(
    IN vip_group group
    );

/*
@brief set group property. configure group. this API should be called before calling vip_run_group.
@param group The group object which created by vip_create_group().
@param property The property be set. see vip_group_property_e.
@param value The set data.
*/
VIP_API
vip_status_e vip_set_group(
    IN vip_group group,
    IN vip_enum property,
    IN void *value
    );

/*! \brief Query a property of the group object.
 *\param [in] group The group object which created by vip_create_group().
 *\param [in] property A property <tt>\ref vip_group_property_e </tt> to be queried.
 *\param [out] value A pointer to memory to store the return value,
               different property could return different type/size of value.
 * please see comment of <tt>\ref vip_group_property_e </tt> for detail.
 *\return <tt>\ref vip_status_e </tt>
 *\ingroup group_network
 *\version 1.0
 */
VIP_API
vip_status_e vip_query_group(
    IN vip_group group,
    IN vip_enum property,
    OUT void *value
    );

/*! \brief. add a vip_network object into group.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object, network be added into group.
*\param network vip_network added into group.
*\version 1.0
*/
VIP_API
vip_status_e vip_add_network(
    IN vip_group group,
    IN vip_network network
    );

/*! \brief. run tasks in group. only issue a interrupt after tasks complete.
            These tasks is added by vip_add_network.
            The order of executuion of tasks is call vip_add_network.
*\return <tt>\ref vip_status_e </tt>
*\param group vip_group object
*\param the number of task will be run.
        eg: num is 4, the 0, 1, 2, 3 taks index in group will be run(inference).
*\version 1.0
*/
VIP_API
vip_status_e vip_run_group(
    IN vip_group group,
    IN vip_uint32_t num
    );

/*! \brief. change PPU engine parameters.
            change local size, global size, global offset and global scale.
*\return <tt>\ref vip_status_e </tt>
*\param network The network object should be changed.
*\param param PPU parameters
*\param index The index of PPU node, not used. please set to zero.
*\ingroup group_network
*\version 1.0
*/
VIP_API
vip_status_e vip_set_ppu_param(
    IN vip_network network,
    IN vip_ppu_param_t *param,
    IN vip_uint32_t index
    );

#ifdef  __cplusplus
}
#endif

#endif
