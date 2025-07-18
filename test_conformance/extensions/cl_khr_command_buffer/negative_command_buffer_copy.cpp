//
// Copyright (c) 2024 The Khronos Group Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "basic_command_buffer.h"
#include "command_buffer_with_immutable_memory.h"
#include "imageHelpers.h"
#include <vector>

//--------------------------------------------------------------------------
template <bool check_image_support>
struct CommandBufferCopyBaseTest : BasicCommandBufferTest
{
    CommandBufferCopyBaseTest(cl_device_id device, cl_context context,
                              cl_command_queue queue)
        : BasicCommandBufferTest(device, context, queue)
    {}

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        in_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size, nullptr,
                                &error);
        test_error(error, "clCreateBuffer failed");

        out_mem = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size, nullptr,
                                 &error);
        test_error(error, "Unable to create buffer");

        if (check_image_support)
        {
            image = create_image_2d(context, CL_MEM_READ_WRITE, &format,
                                    img_width, img_height, 0, NULL, &error);
            test_error(error, "create_image_2d failed");

            buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size,
                                    nullptr, &error);
            test_error(error, "Unable to create buffer");
        }

        return CL_SUCCESS;
    }

    bool Skip() override
    {
        bool command_buffer_multi_device = is_extension_available(
            device, "cl_khr_command_buffer_multi_device");
        if (check_image_support)
        {
            cl_bool image_support;

            cl_int error =
                clGetDeviceInfo(device, CL_DEVICE_IMAGE_SUPPORT,
                                sizeof(image_support), &image_support, nullptr);
            test_error(error,
                       "clGetDeviceInfo for CL_DEVICE_IMAGE_SUPPORT failed");

            return (!image_support || BasicCommandBufferTest::Skip()
                    || command_buffer_multi_device);
        }
        return BasicCommandBufferTest::Skip() || command_buffer_multi_device;
    }

private:
    static constexpr size_t num_channels = 4;

protected:
    static constexpr size_t img_width = 512;
    static constexpr size_t img_height = 512;
    static constexpr size_t origin[3] = { 0, 0, 0 };
    static constexpr size_t region[3] = { img_width, img_height, 1 };
    static constexpr cl_image_format format = { CL_RGBA, CL_UNSIGNED_INT8 };
    static constexpr size_t data_size =
        img_width * img_height * num_channels * sizeof(uint8_t);
    clMemWrapper image;
    clMemWrapper buffer;
    clMemWrapper in_mem;
    clMemWrapper out_mem;
};

template <bool check_image_support>
constexpr size_t CommandBufferCopyBaseTest<check_image_support>::origin[3];
template <bool check_image_support>
constexpr size_t CommandBufferCopyBaseTest<check_image_support>::region[3];
template <bool check_image_support>
constexpr cl_image_format
    CommandBufferCopyBaseTest<check_image_support>::format;

namespace {

// CL_INVALID_COMMAND_QUEUE if command_queue is not NULL.
struct CommandBufferCopyBufferQueueNotNull
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyBufferKHR(command_buffer, queue, nullptr,
                                              in_mem, out_mem, 0, 0, data_size,
                                              0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_QUEUE,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_COMMAND_QUEUE",
                               TEST_FAIL);

        error = clCommandCopyBufferRectKHR(
            command_buffer, queue, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_QUEUE,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_COMMAND_QUEUE",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_COMMAND_QUEUE if command_queue is not NULL.
struct CommandBufferCopyImageQueueNotNull
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyImageToBufferKHR(
            command_buffer, queue, nullptr, image, buffer, origin, region, 0, 0,
            nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_QUEUE,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_COMMAND_QUEUE",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_CONTEXT if the context associated with command_queue,
// command_buffer, src_buffer, and dst_buffer are not the same.
struct CommandBufferCopyBufferDifferentContexts
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int SetUp(int elements) override
    {
        cl_int error = CommandBufferCopyBaseTest::SetUp(elements);
        test_error(error, "CommandBufferCopyBaseTest::SetUp failed");

        context1 = clCreateContext(0, 1, &device, nullptr, nullptr, &error);
        test_error(error, "Failed to create context");

        in_mem_ctx = clCreateBuffer(context1, CL_MEM_READ_ONLY, data_size,
                                    nullptr, &error);
        test_error(error, "clCreateBuffer failed");

        out_mem_ctx = clCreateBuffer(context1, CL_MEM_WRITE_ONLY, data_size,
                                     nullptr, &error);
        test_error(error, "clCreateBuffer failed");

        return CL_SUCCESS;
    }

    cl_int Run() override
    {
        cl_int error = clCommandCopyBufferKHR(
            command_buffer, nullptr, nullptr, in_mem_ctx, out_mem, 0, 0,
            data_size, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);


        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem_ctx, out_mem, origin,
            origin, region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);

        error = clCommandCopyBufferKHR(command_buffer, nullptr, nullptr, in_mem,
                                       out_mem_ctx, 0, 0, data_size, 0, nullptr,
                                       nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);

        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem_ctx, origin,
            origin, region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);


        return CL_SUCCESS;
    }
    clMemWrapper in_mem_ctx = nullptr;
    clMemWrapper out_mem_ctx = nullptr;
    clContextWrapper context1 = nullptr;
};

// CL_INVALID_CONTEXT if the context associated with command_queue,
// command_buffer, src_buffer, and dst_buffer are not the same.
struct CommandBufferCopyImageDifferentContexts
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int SetUp(int elements) override
    {
        cl_int error = CommandBufferCopyBaseTest::SetUp(elements);
        test_error(error, "CommandBufferCopyBaseTest::SetUp failed");

        context1 = clCreateContext(0, 1, &device, nullptr, nullptr, &error);
        test_error(error, "Failed to create context");

        image_ctx = create_image_2d(context1, CL_MEM_READ_WRITE, &format,
                                    img_width, img_height, 0, NULL, &error);
        test_error(error, "create_image_2d failed");

        buffer_ctx = clCreateBuffer(context1, CL_MEM_READ_WRITE, data_size,
                                    nullptr, &error);
        test_error(error, "Unable to create buffer");


        return CL_SUCCESS;
    }

    cl_int Run() override
    {
        cl_int error = clCommandCopyImageToBufferKHR(
            command_buffer, nullptr, nullptr, image_ctx, buffer, origin, region,
            0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);

        error = clCommandCopyImageToBufferKHR(command_buffer, nullptr, nullptr,
                                              image, buffer_ctx, origin, region,
                                              0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_CONTEXT,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_CONTEXT",
                               TEST_FAIL);


        return CL_SUCCESS;
    }
    clMemWrapper image_ctx = nullptr;
    clMemWrapper buffer_ctx = nullptr;
    clContextWrapper context1 = nullptr;
};

// CL_INVALID_SYNC_POINT_WAIT_LIST_KHR if sync_point_wait_list is NULL and
// num_sync_points_in_wait_list is > 0,
// or sync_point_wait_list is not NULL and num_sync_points_in_wait_list is 0,
// or if synchronization-point objects in sync_point_wait_list are not valid
// synchronization-points.
struct CommandBufferCopyBufferSyncPointsNullOrNumZero
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_sync_point_khr invalid_point = 0;

        cl_int error = clCommandCopyBufferKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, 0, 0, data_size,
            1, &invalid_point, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);

        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 1, &invalid_point, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);


        error = clCommandCopyBufferKHR(command_buffer, nullptr, nullptr, in_mem,
                                       out_mem, 0, 0, data_size, 1, nullptr,
                                       nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);


        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 1, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);


        cl_sync_point_khr point;
        error = clCommandBarrierWithWaitListKHR(
            command_buffer, nullptr, nullptr, 0, nullptr, &point, nullptr);
        test_error(error, "clCommandBarrierWithWaitListKHR failed");

        error = clCommandCopyBufferKHR(command_buffer, nullptr, nullptr, in_mem,
                                       out_mem, 0, 0, data_size, 0, &point,
                                       nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);

        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 0, &point, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_SYNC_POINT_WAIT_LIST_KHR if sync_point_wait_list is NULL and
// num_sync_points_in_wait_list is > 0,
// or sync_point_wait_list is not NULL and num_sync_points_in_wait_list is 0,
// or if synchronization-point objects in sync_point_wait_list are not valid
// synchronization-points.
struct CommandBufferCopyImageSyncPointsNullOrNumZero
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_sync_point_khr invalid_point = 0;

        cl_int error = clCommandCopyImageToBufferKHR(
            command_buffer, nullptr, nullptr, image, buffer, origin, region, 0,
            1, &invalid_point, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);


        error = clCommandCopyImageToBufferKHR(command_buffer, nullptr, nullptr,
                                              image, buffer, origin, region, 0,
                                              1, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);


        cl_sync_point_khr point;
        error = clCommandBarrierWithWaitListKHR(
            command_buffer, nullptr, nullptr, 0, nullptr, &point, nullptr);
        test_error(error, "clCommandBarrierWithWaitListKHR failed");

        error = clCommandCopyImageToBufferKHR(command_buffer, nullptr, nullptr,
                                              image, buffer, origin, region, 0,
                                              0, &point, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_SYNC_POINT_WAIT_LIST_KHR",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_COMMAND_BUFFER_KHR if command_buffer is not a valid
// command-buffer.
struct CommandBufferCopyBufferInvalidCommandBuffer
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyBufferKHR(nullptr, nullptr, nullptr, in_mem,
                                              out_mem, 0, 0, data_size, 0,
                                              nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_BUFFER_KHR,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_COMMAND_BUFFER_KHR",
                               TEST_FAIL);

        error = clCommandCopyBufferRectKHR(
            nullptr, nullptr, nullptr, in_mem, out_mem, origin, origin, region,
            0, 0, 0, 0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_BUFFER_KHR,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_COMMAND_BUFFER_KHR",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_COMMAND_BUFFER_KHR if command_buffer is not a valid
// command-buffer.
struct CommandBufferCopyImageInvalidCommandBuffer
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyImageToBufferKHR(
            nullptr, nullptr, nullptr, image, buffer, origin, region, 0, 0,
            nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_COMMAND_BUFFER_KHR,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_COMMAND_BUFFER_KHR",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_OPERATION if command_buffer has been finalized.
struct CommandBufferCopyBufferFinalizedCommandBuffer
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clFinalizeCommandBufferKHR(command_buffer);
        test_error(error, "clFinalizeCommandBufferKHR failed");

        error = clCommandCopyBufferKHR(command_buffer, nullptr, nullptr, in_mem,
                                       out_mem, 0, 0, data_size, 0, nullptr,
                                       nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_OPERATION",
                               TEST_FAIL);


        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_OPERATION",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_OPERATION if command_buffer has been finalized.
struct CommandBufferCopyImageFinalizedCommandBuffer
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_int error = clFinalizeCommandBufferKHR(command_buffer);
        test_error(error, "clFinalizeCommandBufferKHR failed");


        error = clCommandCopyImageToBufferKHR(command_buffer, nullptr, nullptr,
                                              image, buffer, origin, region, 0,
                                              0, nullptr, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_OPERATION",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

// CL_INVALID_VALUE if mutable_handle is not NULL.
struct CommandBufferCopyBufferMutableHandleNotNull
    : public CommandBufferCopyBaseTest<false>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_mutable_command_khr mutable_handle;

        cl_int error = clCommandCopyBufferKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, 0, 0, data_size,
            0, nullptr, nullptr, &mutable_handle);

        test_failure_error_ret(error, CL_INVALID_VALUE,
                               "clCommandCopyBufferKHR should return "
                               "CL_INVALID_VALUE",
                               TEST_FAIL);


        error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, out_mem, origin, origin,
            region, 0, 0, 0, 0, 0, nullptr, nullptr, &mutable_handle);

        test_failure_error_ret(error, CL_INVALID_VALUE,
                               "clCommandCopyBufferRectKHR should return "
                               "CL_INVALID_VALUE",
                               TEST_FAIL);


        return CL_SUCCESS;
    }
};

// CL_INVALID_VALUE if mutable_handle is not NULL.
struct CommandBufferCopyImageMutableHandleNotNull
    : public CommandBufferCopyBaseTest<true>
{
    using CommandBufferCopyBaseTest::CommandBufferCopyBaseTest;

    cl_int Run() override
    {
        cl_mutable_command_khr mutable_handle;

        cl_int error = clCommandCopyImageToBufferKHR(
            command_buffer, nullptr, nullptr, image, buffer, origin, region, 0,
            0, nullptr, nullptr, &mutable_handle);

        test_failure_error_ret(error, CL_INVALID_VALUE,
                               "clCommandCopyImageToBufferKHR should return "
                               "CL_INVALID_VALUE",
                               TEST_FAIL);

        return CL_SUCCESS;
    }
};

struct CommandBufferCopyToImmutableImage
    : public CommandBufferWithImmutableMemoryObjectsTest<
          CommandBufferCopyBaseTest<true>>
{
    using CommandBufferWithImmutableMemoryObjectsTest::
        CommandBufferWithImmutableMemoryObjectsTest;

    cl_int Run() override
    {
        cl_int error = clCommandFillImageKHR(
            command_buffer, nullptr, nullptr, src_image, fill_color_1, origin,
            region, 0, nullptr, nullptr, nullptr);

        test_error(error, "clCommandFillImageKHR failed");

        error = clCommandCopyImageKHR(command_buffer, nullptr, nullptr,
                                      src_image, dst_image, origin, origin,
                                      region, 0, 0, nullptr, nullptr);

        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyImageKHR is supposed to fail "
                               "with CL_INVALID_OPERATION when dst_image is "
                               "created with CL_MEM_IMMUTABLE_EXT",
                               TEST_FAIL);

        return CL_SUCCESS;
    }

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        src_image = create_image_2d(context, CL_MEM_READ_ONLY, &format,
                                    img_width, img_height, 0, nullptr, &error);
        test_error(error, "create_image_2d failed");

        size_t pixel_size = get_pixel_size(&format);
        size_t image_size =
            pixel_size * sizeof(cl_uchar) * img_width * img_height;

        std::vector<cl_uchar> imgptr(image_size);

        dst_image = create_image_2d(
            context, CL_MEM_IMMUTABLE_EXT | CL_MEM_COPY_HOST_PTR, &format,
            img_width, img_height, 0, imgptr.data(), &error);
        test_error(error, "create_image_2d failed");

        return CL_SUCCESS;
    }

    clMemWrapper dst_image;
    clMemWrapper src_image;
    static constexpr cl_uint pattern_1 = 0x05;
    const cl_uint fill_color_1[4] = { pattern_1, pattern_1, pattern_1,
                                      pattern_1 };
};

struct CommandBufferCopyToImmutableBuffer
    : public CommandBufferWithImmutableMemoryObjectsTest<
          CommandBufferCopyBaseTest<false>>
{
    using CommandBufferWithImmutableMemoryObjectsTest::
        CommandBufferWithImmutableMemoryObjectsTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyBufferKHR(command_buffer, nullptr, nullptr,
                                              in_mem, buffer, 0, 0, data_size,
                                              0, nullptr, nullptr, nullptr);
        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyBufferKHR is supposed to fail "
                               "with CL_INVALID_OPERATION when dst_buffer is "
                               "created with CL_MEM_IMMUTABLE_EXT",
                               TEST_FAIL);
        return CL_SUCCESS;
    }

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        in_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, data_size, nullptr,
                                &error);
        test_error(error, "clCreateBuffer failed");

        std::vector<cl_uchar> data(data_size);

        buffer =
            clCreateBuffer(context, CL_MEM_IMMUTABLE_EXT | CL_MEM_COPY_HOST_PTR,
                           data_size, data.data(), &error);
        test_error(error, "clCreateBuffer failed");

        return CL_SUCCESS;
    }
};

struct CommandBufferCopyBufferToImmutableImage
    : public CommandBufferWithImmutableMemoryObjectsTest<
          CommandBufferCopyBaseTest<true>>
{
    using CommandBufferWithImmutableMemoryObjectsTest::
        CommandBufferWithImmutableMemoryObjectsTest;

    cl_int Run() override
    {
        cl_int error = clCommandFillBufferKHR(
            command_buffer, nullptr, nullptr, buffer, &pattern_1,
            sizeof(pattern_1), 0, data_size, 0, nullptr, nullptr, nullptr);

        test_error(error, "clCommandFillBufferKHR failed");

        error = clCommandCopyBufferToImageKHR(command_buffer, nullptr, nullptr,
                                              buffer, image, 0, origin, region,
                                              0, 0, nullptr, nullptr);

        test_failure_error_ret(
            error, CL_INVALID_OPERATION,
            "clCommandCopyBufferToImageKHR is supposed to fail "
            "with CL_INVALID_OPERATION when dst_image is "
            "created with CL_MEM_IMMUTABLE_EXT",
            TEST_FAIL);

        return CL_SUCCESS;
    }

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size, nullptr,
                                &error);
        test_error(error, "Unable to create buffer");

        size_t pixel_size = get_pixel_size(&format);
        size_t image_size =
            pixel_size * sizeof(cl_uchar) * img_width * img_height;

        std::vector<cl_uchar> imgptr(image_size);

        image = create_image_2d(
            context, CL_MEM_IMMUTABLE_EXT | CL_MEM_COPY_HOST_PTR, &format,
            img_width, img_height, 0, imgptr.data(), &error);
        test_error(error, "create_image_2d failed");

        return CL_SUCCESS;
    }

    const uint8_t pattern_1 = 0x05;
};

struct CommandBufferCopyImageToImmutableBuffer
    : public CommandBufferWithImmutableMemoryObjectsTest<
          CommandBufferCopyBaseTest<true>>
{
    using CommandBufferWithImmutableMemoryObjectsTest::
        CommandBufferWithImmutableMemoryObjectsTest;

    cl_int Run() override
    {
        cl_int error = clCommandFillImageKHR(
            command_buffer, nullptr, nullptr, image, fill_color_1, origin,
            region, 0, nullptr, nullptr, nullptr);

        test_error(error, "clCommandFillImageKHR failed");

        error = clCommandCopyImageToBufferKHR(command_buffer, nullptr, nullptr,
                                              image, buffer, origin, region, 0,
                                              0, nullptr, nullptr, nullptr);

        test_failure_error_ret(
            error, CL_INVALID_OPERATION,
            "clCommandCopyImageToBufferKHR is supposed to fail "
            "with CL_INVALID_OPERATION when dst_buffer is "
            "created with CL_MEM_IMMUTABLE_EXT",
            TEST_FAIL);

        return CL_SUCCESS;
    }

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        image = create_image_2d(context, CL_MEM_READ_WRITE, &format, img_width,
                                img_height, 0, NULL, &error);
        test_error(error, "create_image_2d failed");

        std::vector<cl_uchar> data(data_size);

        buffer =
            clCreateBuffer(context, CL_MEM_IMMUTABLE_EXT | CL_MEM_COPY_HOST_PTR,
                           data_size, data.data(), &error);
        test_error(error, "Unable to create buffer");

        return CL_SUCCESS;
    }

    static constexpr cl_uint pattern_1 = 0x12;
    const cl_uint fill_color_1[4] = { pattern_1, pattern_1, pattern_1,
                                      pattern_1 };
};

struct CommandBufferCopyToImmutableBufferRect
    : public CommandBufferWithImmutableMemoryObjectsTest<
          CommandBufferCopyBaseTest<false>>
{
    using CommandBufferWithImmutableMemoryObjectsTest::
        CommandBufferWithImmutableMemoryObjectsTest;

    cl_int Run() override
    {
        cl_int error = clCommandCopyBufferRectKHR(
            command_buffer, nullptr, nullptr, in_mem, buffer, origin, origin,
            region, 0, 0, 0, 0, 0, nullptr, nullptr, nullptr);
        test_failure_error_ret(error, CL_INVALID_OPERATION,
                               "clCommandCopyBufferRectKHR is supposed to fail "
                               "with CL_INVALID_OPERATION when dst_buffer is "
                               "created with CL_MEM_IMMUTABLE_EXT",
                               TEST_FAIL);
        return CL_SUCCESS;
    }

    cl_int SetUp(int elements) override
    {
        cl_int error = BasicCommandBufferTest::SetUp(elements);
        test_error(error, "BasicCommandBufferTest::SetUp failed");

        in_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, data_size, nullptr,
                                &error);
        test_error(error, "clCreateBuffer failed");

        std::vector<cl_uchar> data(data_size);

        buffer =
            clCreateBuffer(context, CL_MEM_IMMUTABLE_EXT | CL_MEM_COPY_HOST_PTR,
                           data_size, data.data(), &error);
        test_error(error, "clCreateBuffer failed");

        return CL_SUCCESS;
    }
};
}

REGISTER_TEST(negative_command_buffer_command_copy_buffer_queue_not_null)
{
    return MakeAndRunTest<CommandBufferCopyBufferQueueNotNull>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_command_buffer_command_copy_image_queue_not_null)
{
    return MakeAndRunTest<CommandBufferCopyImageQueueNotNull>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_command_buffer_command_copy_buffer_different_contexts)
{
    return MakeAndRunTest<CommandBufferCopyBufferDifferentContexts>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_command_buffer_command_copy_image_different_contexts)
{
    return MakeAndRunTest<CommandBufferCopyImageDifferentContexts>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_buffer_sync_points_null_or_num_zero)
{
    return MakeAndRunTest<CommandBufferCopyBufferSyncPointsNullOrNumZero>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_image_sync_points_null_or_num_zero)
{
    return MakeAndRunTest<CommandBufferCopyImageSyncPointsNullOrNumZero>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_buffer_invalid_command_buffer)
{
    return MakeAndRunTest<CommandBufferCopyBufferInvalidCommandBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_command_buffer_command_copy_image_invalid_command_buffer)
{
    return MakeAndRunTest<CommandBufferCopyImageInvalidCommandBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_buffer_finalized_command_buffer)
{
    return MakeAndRunTest<CommandBufferCopyBufferFinalizedCommandBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_image_finalized_command_buffer)
{
    return MakeAndRunTest<CommandBufferCopyImageFinalizedCommandBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_buffer_mutable_handle_not_null)
{
    return MakeAndRunTest<CommandBufferCopyBufferMutableHandleNotNull>(
        device, context, queue, num_elements);
}

REGISTER_TEST(
    negative_command_buffer_command_copy_image_mutable_handle_not_null)
{
    return MakeAndRunTest<CommandBufferCopyImageMutableHandleNotNull>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_copy_to_immutable_buffer)
{
    return MakeAndRunTest<CommandBufferCopyToImmutableBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_copy_to_immutable_buffer_rect)
{
    return MakeAndRunTest<CommandBufferCopyToImmutableBufferRect>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_copy_image_to_immutable_buffer)
{
    return MakeAndRunTest<CommandBufferCopyImageToImmutableBuffer>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_copy_to_immutable_image)
{
    return MakeAndRunTest<CommandBufferCopyToImmutableImage>(
        device, context, queue, num_elements);
}

REGISTER_TEST(negative_copy_buffer_to_immutable_image)
{
    return MakeAndRunTest<CommandBufferCopyBufferToImmutableImage>(
        device, context, queue, num_elements);
}
