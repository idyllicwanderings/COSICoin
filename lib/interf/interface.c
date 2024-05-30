#include "interf/wrapper.h"

void interface_c(const uint32_t* block, uint32_t number_blocks, uint32_t* output) {

  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd == -1) {
	  perror("Error opening /dev/mem");
  }
  
  uint32_t* hw_base;
  // map the physical address to virtual memory
  hw_base = (uint32_t*)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPGA_BASE_ADDR);
  if(hw_base == MAP_FAILED) {
	  perror("Error mapping hardware address");
	  close(fd);
  }

  // to store values
  uint32_t out1, out2, out3, out4, out5, out6, out7, out8;
  // put data into hw reg
  hw_base[1] = block[16];  
  hw_base[2] = block[17];
  hw_base[3] = block[18];
  hw_base[4] = block[19];
  hw_base[5] = block[20];
  hw_base[6] = block[21];
  hw_base[7] = block[22];

  // hw_base[0] 
  // put last bit to 1 to start operation
  //second to last to one if input size 1024 and otherwise 512
  // prior bits: control signal for certain read out

  uint32_t start, first_read, second_read, third_read, fourth_read, fifth_read, first_output_read;
  uint32_t second_output_read, finish;

  if (number_blocks == 0x0){
    start = 0x1;
    first_read = 0x5;
    second_read = 0x9;
    third_read = 0x11;
    fourth_read = 0x0;
    fifth_read = 0x0;
    first_output_read = 0x81;
    second_output_read = 0x101;
    finish = 0x201;
  }
  else {
    start = 0x3;
    first_read = 0x7;
    second_read = 0xb;
    third_read = 0x13;
    fourth_read = 0x23;
    fifth_read = 0x43;
    first_output_read = 0x83;
    second_output_read = 0x103;
    finish = 0x203;
  }

  
  hw_base[0] = 0x00;
  hw_base[0] = start;
  hw_base[0] = first_read; //read second part
  hw_base[1] = block[23];
  hw_base[2] = block[24];
  hw_base[3] = block[25];
  hw_base[4] = block[26];
  hw_base[5] = block[27];
  hw_base[6] = block[28];
  hw_base[7] = block[29];
  hw_base[0] = second_read;  //read 3rd part
  hw_base[1] = block[30];
  hw_base[2] = block[31];
  hw_base[3] = block[0];
  hw_base[4] = block[1];
  hw_base[5] = block[2];
  hw_base[6] = block[3];
  hw_base[7] = block[4];
  hw_base[0] = third_read;  // go to compute stage or read 4th part
  if (number_blocks != 0x0){
  hw_base[1] = block[5];
  hw_base[2] = block[6];
  hw_base[3] = block[7];
  hw_base[4] = block[8];
  hw_base[5] = block[9];
  hw_base[6] = block[10];
  hw_base[7] = block[11];
  hw_base[0] = fourth_read;
  hw_base[1] = block[12];
  hw_base[2] = block[13];
  hw_base[3] = block[14];
  hw_base[4] = block[15];
  hw_base[0] = fifth_read; // go to compute
  }
  while (hw_base[0] == 0x0){ //computing result
  }
  hw_base[0] = first_output_read; //start output reading
  out1 = hw_base[1];
  out2 = hw_base[2];
  out3 = hw_base[3];
  out4 = hw_base[4];
  out5 = hw_base[5];
  out6 = hw_base[6];
  out7 = hw_base[7];
  hw_base[0] = second_output_read; //start output reading 2
  out8 = hw_base[1];
  hw_base[0] = finish; // go to state done
  hw_base[0] = 0x00;


  munmap(hw_base, 4096);
  close(fd);

  output[0] = out8;
  output[1] = out7;
  output[2] = out6;
  output[3] = out5;
  output[4] = out4;
  output[5] = out3;
  output[6] = out2;
  output[7] = out1;
}
