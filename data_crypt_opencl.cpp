#include <iostream>
#include <CL/cl.h>
#include <chrono>

const char* aesEncryptionKernelSource = R"(
    __kernel void aesEncryptionKernel(__global uchar* data, __global uchar* key, int dataSize) {
        int i = get_global_id(0);
        if (i < dataSize) {
            data[i] = data[i] ^ key[i % 16];
        }
    }
)";

void generateData(unsigned char* data, int dataSize) {
    data[0] = rand() % 256;
    for (int i = 1; i < dataSize; ++i) {
        data[i] = (unsigned char)(data[i - 1] + i) % 256;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dataSize>" << std::endl;
        return 1;
    }

    int dataSize = atoi(argv[1]);
    unsigned char* data = (unsigned char*)malloc(dataSize);
    unsigned char* key = (unsigned char*)malloc(16);

    generateData(data, dataSize);
    generateData(key, 16);

    cl_platform_id platform;
    clGetPlatformIDs(1, &platform, nullptr);

    cl_device_id device;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, nullptr);

    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, nullptr);
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, nullptr, nullptr);

    cl_program program = clCreateProgramWithSource(context, 1, &aesEncryptionKernelSource, nullptr, nullptr);
    clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    cl_kernel kernel = clCreateKernel(program, "aesEncryptionKernel", nullptr);

    cl_mem d_data = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, dataSize, data, nullptr);
    cl_mem d_key = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 16, key, nullptr);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_data);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_key);
    clSetKernelArg(kernel, 2, sizeof(int), &dataSize);

    cl_event start, stop;

    size_t globalWorkSize = dataSize;

    auto startTime = std::chrono::high_resolution_clock::now();

    clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, &start);
    clFinish(queue);

    auto stopTime = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stopTime - startTime);

    std::cout << "Time taken: " << duration.count() / 1000.0 << " ms" << std::endl;

    clReleaseMemObject(d_data);
    clReleaseMemObject(d_key);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    free(data);
    free(key);

    return 0;
}
