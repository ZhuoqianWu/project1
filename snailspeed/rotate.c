/**
 * Copyright (c) 2024 MIT License by 6.106 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include "../utils/utils.h"
#include "my_utils.h"
#include <stdint.h>
#include <string.h> 

uint64_t bit_reverse64(uint64_t x) {
    x = ((x & 0x5555555555555555ULL) << 1) | ((x >> 1) & 0x5555555555555555ULL);
    x = ((x & 0x3333333333333333ULL) << 2) | ((x >> 2) & 0x3333333333333333ULL);
    x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4) | ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL);
    x = ((x & 0x00FF00FF00FF00FFULL) << 8) | ((x >> 8) & 0x00FF00FF00FF00FFULL);
    x = ((x & 0x0000FFFF0000FFFFULL) << 16) | ((x >> 16) & 0x0000FFFF0000FFFFULL);
    x = (x << 32) | (x >> 32);
    return x;
}

uint64_t bit_reverse8in64(uint64_t x) {
    x = ((x & 0x5555555555555555ULL) << 1)  | ((x >> 1) & 0x5555555555555555ULL);
    x = ((x & 0x3333333333333333ULL) << 2)  | ((x >> 2) & 0x3333333333333333ULL);
    x = ((x & 0x0F0F0F0F0F0F0F0FULL) << 4)  | ((x >> 4) & 0x0F0F0F0F0F0F0F0FULL);
    return x;
}


void transpose64_and_reverse1(uint8_t *img, uint32_t row_size) {
    #define row_of(r) (*(uint64_t*)(img + row_size * (r)))
    int j, k;
    uint64_t m, t;
    for (j = 0; j < 64; j++) {
        row_of(j) = bit_reverse8in64(bit_reverse64(row_of(j)));
    }
    for (j = 32, m = 0xFFFFFFFFULL; j; j >>= 1, m ^= m<<j) {
        for (k = 0; k < 64; k = ((k | j) + 1) & ~j) {
            t = (row_of(k) ^ (row_of(k|j) >> j)) & m;
            row_of(k) ^= t;
            row_of(k|j) ^= (t << j);
        }
    }
    for (j = 0; j < 64; j++) {
        row_of(j) = bit_reverse8in64(row_of(j));
    }
} 


#define rotateright(x, k) ((x) >> (k)) | ((x) << (64 - (k)))
#define rotateleft(x, k)  ((x) << (k)) | ((x) >> (64 - (k)))


void rotate_bit_matrix_simple(uint8_t *img, uint32_t block_i, uint32_t block_j, uint32_t block_size, uint32_t total_row_size, const bits_t N) {
    uint32_t block_start = block_i * block_size * total_row_size + block_j * block_size / 8;
    transpose64_and_reverse1(img + block_start, total_row_size);
}



// 主旋转函数
void rotate_bit_matrix(uint8_t *img, const bits_t N) {
    const uint32_t block_size = 64;
    const uint32_t row_size = bits_to_bytes(N);
    const uint32_t num_blocks = N / block_size;

    if (num_blocks == 1) {
        rotate_bit_matrix_simple(img, 0, 0, block_size, row_size, N);
        return;
    }

    for (uint32_t block_i = 0; block_i < (num_blocks + 1) / 2; block_i++) {
        for (uint32_t block_j = 0; block_j < num_blocks / 2; block_j++) {
            const uint32_t A_i = block_i;
            const uint32_t A_j = block_j;
            const uint32_t B_i = block_j;
            const uint32_t B_j = num_blocks - 1 - block_i;
            const uint32_t C_i = num_blocks - 1 - block_i;
            const uint32_t C_j = num_blocks - 1 - block_j;
            const uint32_t D_i = num_blocks - 1 - block_j;
            const uint32_t D_j = block_i;
            
            // 逐行交换（内存高效）
            for (uint32_t i = 0; i < block_size; i++) {
                uint8_t temp_row[block_size / 8];
                uint8_t *A_ptr = &img[(A_i * block_size + i) * row_size + (A_j * block_size / 8)];
                uint8_t *B_ptr = &img[(B_i * block_size + i) * row_size + (B_j * block_size / 8)];
                uint8_t *C_ptr = &img[(C_i * block_size + i) * row_size + (C_j * block_size / 8)];
                uint8_t *D_ptr = &img[(D_i * block_size + i) * row_size + (D_j * block_size / 8)];
                
                memcpy(temp_row, A_ptr, block_size / 8);
                memcpy(A_ptr, D_ptr, block_size / 8);
                memcpy(D_ptr, C_ptr, block_size / 8);
                memcpy(C_ptr, B_ptr, block_size / 8);
                memcpy(B_ptr, temp_row, block_size / 8);
            }
        }
    }

    for (uint32_t block_i = 0; block_i < num_blocks; block_i++) {
        for (uint32_t block_j = 0; block_j < num_blocks; block_j++) {
            rotate_bit_matrix_simple(img, block_i, block_j, block_size, row_size, N);
        }
    }

}

