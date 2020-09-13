
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

/*******************************************************************************
Description:
    HLS pragmas can be used to optimize the design : improve throughput, reduce latency and 
    device resource utilization of the resulting RTL code
    This is vector addition example to demonstrate how HLS optimizations are used in kernel. 
*******************************************************************************/


#define BUFFER_SIZE 1024
#define BUFFER_OUTPUT_SIZE 32

extern "C" {
void knn(
        // const unsigned int *in1, // Read-Only Vector 1
        // const unsigned int *in2, // Read-Only Vector 2
        // unsigned int *out,       // Output Result
        // int size,                // Size in integer
		// int k,
		// int v1,
		// int v2
		const unsigned int* in, 
		unsigned int* out,
		int size,
		int k,
		int dim,
		const unsigned int* v

        )
{
// #pragma HLS INTERFACE m_axi port=in1  offset=slave bundle=gmem
// #pragma HLS INTERFACE m_axi port=in2  offset=slave bundle=gmem
// #pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
// #pragma HLS INTERFACE s_axilite port=in1  bundle=control
// #pragma HLS INTERFACE s_axilite port=in2  bundle=control
// #pragma HLS INTERFACE s_axilite port=out bundle=control
// #pragma HLS INTERFACE s_axilite port=size bundle=control
// #pragma HLS INTERFACE s_axilite port=k bundle=control
// #pragma HLS INTERFACE s_axilite port=v1 bundle=control
// #pragma HLS INTERFACE s_axilite port=v2 bundle=control
// #pragma HLS INTERFACE s_axilite port=return bundle=control

	#pragma HLS INTERFACE m_axi port=in  offset=slave bundle=gmem
	#pragma HLS INTERFACE m_axi port=out offset=slave bundle=gmem
	#pragma HLS INTERFACE m_axi port=v   offset=slave bundle=gmem

	#pragma HLS INTERFACE s_axilite port=in     bundle=control	
	#pragma HLS INTERFACE s_axilite port=out    bundle=control
	#pragma HLS INTERFACE s_axilite port=size   bundle=control
	#pragma HLS INTERFACE s_axilite port=k      bundle=control
	#pragma HLS INTERFACE s_axilite port=v      bundle=control
	#pragma HLS INTERFACE s_axilite port=return bundle=control


    unsigned int in_buffer[BUFFER_SIZE][dim];	
	unsigned int k_idx_buffer[BUFFER_OUTPUT_SIZE]; 
	int k_dist_buffer[BUFFER_OUTPUT_SIZE];


    // unsigned int v1_buffer[BUFFER_SIZE];    // Local memory to store vector1
    // unsigned int v2_buffer[BUFFER_SIZE];    // Local memory to store vector2
    // unsigned int k_idx_buffer[BUFFER_OUTPUT_SIZE];  // Local Memory to store result
	// int k_dist_buffer[BUFFER_OUTPUT_SIZE];
	
	for (int i = 0 ; i < k; i++) {
		k_dist_buffer[i] = 100000000;
	}

    for (int i = 0; i < size; i += BUFFER_SIZE)
    {
        int chunk_size = BUFFER_SIZE;
        if ((i + BUFFER_SIZE) > size) 
            chunk_size = size - i;

        // read1: for (int j = 0 ; j < chunk_size ; j++){
        //     in_buffer[j] = in[i + j];
        // }
        // read2: for (int j = 0 ; j < chunk_size ; j++){
        //     v2_buffer[j] = in2[i + j];
        // }

		read: for (int j = 0; j < chunk_size; j++) {
			for(int m = 0; m < dim; m++) {
				in_buffer[j][m] = in[i + j + m];
			}
		}

		for (int j = 0; j < chunk_size; j++) {
			int dist = 0;
			for(int m = 0; m < dim; m++) {
				int dj = in_buffer[j][m] - v[m];
				dist += dj * dj;
			}

			for (int m = 0 ; m < k ; m++) {
				if (k_dist_buffer[m] > dist) {
					for (int n = k - 1 ; n > m ; n--) {
						k_idx_buffer[n] = k_idx_buffer[n - 1];
						k_dist_buffer[n] = k_dist_buffer[n - 1];
					}
					k_idx_buffer[m] = i + j;
					k_dist_buffer[m] = dist;
					break;
				}
			}


		}

		// for (int j = 0; j < chunk_size ; j++) {
		// 	int x = v1_buffer[j];
		// 	int y = v2_buffer[j];
		// 	int dist = (v1 - x) * (v1 - x) + (v2 - y) * (v2 - y);
			
		// 	for (int m = 0 ; m < k ; m++) {
		// 		if (k_dist_buffer[m] > dist) {
		// 			for (int n = k - 1 ; n > m ; n--) {
		// 				k_idx_buffer[n] = k_idx_buffer[n - 1];
		// 				k_dist_buffer[n] = k_dist_buffer[n - 1];
		// 			}
		// 			k_idx_buffer[m] = i + j;
		// 			k_dist_buffer[m] = dist;
		// 			break;
		// 		}
		// 	}
		// }
    }

	write: for (int i = 0 ; i < k ; i++) {
		out[i] = k_idx_buffer[i];
	}
}
}
