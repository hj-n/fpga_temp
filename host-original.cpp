/**********
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include "host.hpp"
#include <chrono>
#include <cstdlib>

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::cout << "Usage: " << argv[0] << " <XCLBIN File> <DATA SIZE>" << std::endl;
		return EXIT_FAILURE;
	}
	
	//std::cout << "argv[2] : " << argv[2] << std::endl;

	int k = 3;
	int v1 = 100;
	int v2 = 100;
	int value_range = 200;

   	int data_size = atoi(argv[2]);

//	int data_size = 4096;

    std::string binaryFile = argv[1];
    // size_t vector_size_bytes = sizeof(int) * DATA_SIZE;
    size_t vector_size_bytes = sizeof(int) * data_size;
	size_t vector_output_size_bytes = sizeof(int) * k;
    cl_int err;
    unsigned fileBufSize;
    
	// Allocate Memory in Host Memory
    // std::vector<int,aligned_allocator<int>> source_in1(DATA_SIZE);
    // std::vector<int,aligned_allocator<int>> source_in2(DATA_SIZE);
    std::vector<int,aligned_allocator<int>> source_in1(data_size);
    std::vector<int,aligned_allocator<int>> source_in2(data_size);
	std::vector<int,aligned_allocator<int>> source_hw_results(k);

	std::cout << "K : " << k << std::endl;
	std::cout << "Input point (x, y) : (" << v1 << ", "  << v2 << ")"<< std::endl;
    std::cout << "Data size : " << data_size << std::endl;
    
	int k_idx[32];
	int k_dist[32];
	for (int i = 0 ; i < k; i++) {
		k_dist[i] = 100000000;
	}

    // Create the test data 
    for (int i = 0 ; i < data_size ; i++){
        source_in1[i] = rand() % value_range;
        source_in2[i] = rand() % value_range;
	}

	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

	for (int i = 0 ; i < data_size ; i++) {
		int dx = v1 - source_in1[i];
		int dy = v2 - source_in2[i];
		int dist = dx * dx + dy * dy;
		for (int m = 0 ; m < k ; m++) {
			if (k_dist[m] > dist) {
				for (int n = k - 1; n > m ; n--) {
					k_idx[n] = k_idx[n - 1];
					k_dist[n] = k_dist[n - 1];
				}
				k_idx[m] = i;
				k_dist[m] = dist;
				break;
			}
		}
    }

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "[CPU Calculation] Elapsed Time (ms) = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
		<< " ms" << std::endl;

    std::cout << "==============================" << std::endl;
	for (int i = 0; i < k; i++) {
		std::cout << k_idx[i] << " / " << k_dist[i] << std::endl;
	}


// OPENCL HOST CODE AREA START
	
// ------------------------------------------------------------------------------------
// Step 1: Get All PLATFORMS, then search for Target_Platform_Vendor (CL_PLATFORM_VENDOR)
//	   Search for Platform: Xilinx 
// Check if the current platform matches Target_Platform_Vendor
// ------------------------------------------------------------------------------------	
    std::vector<cl::Device> devices = get_devices("Xilinx");
    devices.resize(1);
    cl::Device device = devices[0];

// ------------------------------------------------------------------------------------
// Step 1: Create Context
// ------------------------------------------------------------------------------------
    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
	
// ------------------------------------------------------------------------------------
// Step 1: Create Command Queue
// ------------------------------------------------------------------------------------
    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err));

// ------------------------------------------------------------------
// Step 1: Load Binary File from disk
// ------------------------------------------------------------------		
    char* fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
	
// -------------------------------------------------------------
// Step 1: Create the program object from the binary and program the FPGA device with it
// -------------------------------------------------------------	
    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

// -------------------------------------------------------------
// Step 1: Create Kernels
// -------------------------------------------------------------
    OCL_CHECK(err, cl::Kernel krnl_knn_2d(program,"knn-enhance", &err));

// ================================================================
// Step 2: Setup Buffers and run Kernels
// ================================================================
//   o) Allocate Memory to store the results 
//   o) Create Buffers in Global Memory to store data
// ================================================================

// ------------------------------------------------------------------
// Step 2: Create Buffers in Global Memory to store data
//             o) buffer_in1 - stores source_in1
//             o) buffer_in2 - stores source_in2
//             o) buffer_ouput - stores Results
// ------------------------------------------------------------------	

// .......................................................
// Allocate Global Memory for source_in1
// .......................................................	
    OCL_CHECK(err, cl::Buffer buffer_in1   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            vector_size_bytes, source_in1.data(), &err));
// .......................................................
// Allocate Global Memory for source_in2
// .......................................................
    OCL_CHECK(err, cl::Buffer buffer_in2   (context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 
            vector_size_bytes, source_in2.data(), &err));
// .......................................................
// Allocate Global Memory for sourcce_hw_results
// .......................................................
   OCL_CHECK(err, cl::Buffer buffer_output(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, 
            vector_output_size_bytes, source_hw_results.data(), &err));

   // OCL_CHECK(err, cl::Buffer buffer_output(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, 
   //         vector_size_bytes, source_hw_results.data(), &err));

// ============================================================================
// Step 2: Set Kernel Arguments and Run the Application
//         o) Set Kernel Arguments
//              ----------------------------------------------------
//              Kernel Argument  Description
//              ----------------------------------------------------
//              in1   (input)     --> Input Vector1
//              in2   (input)     --> Input Vector2
//              out   (output)    --> Output Vector
//              size  (input)     --> Size of Vector in Integer
//         o) Copy Input Data from Host to Global Memory on the device
//         o) Submit Kernels for Execution
//         o) Copy Results from Global Memory, device to Host
// ============================================================================	
    int size = data_size;
    OCL_CHECK(err, err = krnl_knn_2d.setArg(0, buffer_in1));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(1, buffer_in2));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(2, buffer_output));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(3, size));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(4, k));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(5, v1));
    OCL_CHECK(err, err = krnl_knn_2d.setArg(6, v2));

	begin = std::chrono::steady_clock::now();

// ------------------------------------------------------
// Step 2: Copy Input data from Host to Global Memory on the device
// ------------------------------------------------------
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_in1, buffer_in2},0/* 0 means from host*/));	
	
// ----------------------------------------
// Step 2: Submit Kernels for Execution
// ----------------------------------------
//	
    // OCL_CHECK(err, err = q.enqueueTask(krnl_knn_2d));
	//
	cl::Event event;
    // OCL_CHECK(err, err = q.enqueueTask(krnl_knn_2d, NULL, $event));
	err = q.enqueueTask(krnl_knn_2d, NULL, &event);
	
// --------------------------------------------------
// Step 2: Copy Results from Device Global Memory to Host
// --------------------------------------------------
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_output},CL_MIGRATE_MEM_OBJECT_HOST));

	event.wait();

    q.finish();

	cl_ulong end_time = event.getProfilingInfo<CL_PROFILING_COMMAND_END>();
	cl_ulong start_time = event.getProfilingInfo<CL_PROFILING_COMMAND_START>();
	
	// std::cout << "end time " << end_time << std::endl;
	// std::cout << "start time " << start_time << std::endl;
	std::cout << "[FPGA Kernel] Elapsed Time (ms) = " << (end_time - start_time) / (float)1e6 << " ms" << std::endl;
	// std::cout << (int)1e6 << std::endl;
		
	end = std::chrono::steady_clock::now();
	std::cout << "[FPGA Host + Kernel] Elapsed Time (ms) = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() / 1000
		<< " ms" << std::endl;


// OPENCL HOST CODE AREA END

    // Compare the results of the Device to the simulation
    bool match = true;
	for (int i = 0 ; i < k; i++) {
		int idx = source_hw_results[i];
		int x = source_in1[idx];
		int y = source_in2[idx];
		int dist = (x - v1) * (x - v1) + (y - v2) * (y - v2);
		std::cout << idx <<  " (" << x << " , " << y << "), " << dist << std::endl;
		if (k_dist[i] != dist) {
			match = false;
		}
	}

// ============================================================================
// Step 3: Release Allocated Resources
// ============================================================================
    delete[] fileBuf;

    std::cout << "TEST " << (match ? "PASSED" : "FAILED") << std::endl; 
    return (match ? EXIT_SUCCESS : EXIT_FAILURE);
}

