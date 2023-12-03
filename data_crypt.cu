#include <iostream>
#include <cuda_runtime.h>


// CUDA core for AES algorithm
__global__ void aesEncryptionKernel(unsigned char *data, unsigned char *key, int dataSize) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < dataSize) {
        data[i] = data[i] ^ key[i % 16];
    }
}

void generateData(unsigned char *data, int dataSize) {
    data[0] = rand() % 256;
    for (int i = 1; i < dataSize; ++i) {
        data[i] = (unsigned char) (data[i - 1] + i) % 256;
    }
}

int main(int argc, char *argv[]) {
    int dataSize = atoi(argv[1]); // unhandled exception if no argument is passed
    unsigned char *data = (unsigned char *) malloc(dataSize);
    unsigned char *key = (unsigned char *) malloc(16);

    generateData(data, dataSize);
    generateData(key, 16);

    unsigned char *d_data, *d_key;
    cudaMalloc(&d_data, dataSize);
    cudaMalloc(&d_key, 16);

    // copy data to device
    cudaMemcpy(d_data, data, dataSize, cudaMemcpyHostToDevice);
    cudaMemcpy(d_key, key, 16, cudaMemcpyHostToDevice);

    // variables to track time
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start, 0);
    int repeat = 10;

    for (int i = 0; i < repeat; i++)
        aesEncryptionKernel<<<(dataSize + 255) / 256, 256>>>(d_data, d_key, dataSize);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    // copy data back to host
    cudaMemcpy(data, d_data, dataSize, cudaMemcpyDeviceToHost);

    float milliseconds = 0;
    cudaEventElapsedTime(&milliseconds, start, stop);

    std::cout << "Time taken: " << milliseconds / repeat << " ms" << std::endl;
}