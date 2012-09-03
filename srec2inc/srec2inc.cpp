/**
 *	srec2inc.cpp - A utility for Freescale DSP563XX ROM
 *		SREC files should be generated using the following flags:
 *		 srec -S -R -A3 xxx.cld
 *		  -S forces S0 to indicate the DSP memory space
 *		  -R reverses the order of bytes from lo -> hi to hi -> lo
 *		  -A3 forces S2 records to be used with 24-bit addressing
 *
 *		This program is useful for DSP implementations that are 
 *		 embedded into an MCU project without a filesystem.
 *
 *		Usage: srec2inc -N -I -O
 *				-N = size of arrays (evenly divisable by 3). Default = 18 (min. is 9)
 *				-I = path to input file
 *				-O = path to output file. With no parameter default.inc is
 *					 created in the programs directory
 *		Example: srec2inc -N18 -Iinput.srec -Ooutput.inc
 */

/**
 *  Copyright (c) 2009 Sam Grove, MIT License
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *   
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *   
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <string.h>
#include <stdint.h>

#include "atoh.h"

// prototypes
int buildIncFile( std::ifstream &in, std::ofstream &out, int32_t pkt_size);

int main(int argc, char **argv)
{
	
	// file streams
	std::ifstream f_in;
	std::ofstream f_out;
	// array size
	int32_t packet_size = 18;
	// flag option verification
	enum
	{
		FLAG_NOT_PRESENT, FLAG_FAILED, FLAG_OK
	} n_flag = FLAG_NOT_PRESENT, i_flag = FLAG_NOT_PRESENT, o_flag = FLAG_NOT_PRESENT;

	while( argc-- != 1 )
	{
		switch (*(argv[argc]+1))
		{
			case 'N':
			case 'n':
				// convert the parameter to hex and test it
				n_flag = FLAG_OK;
				packet_size = atoh<uint32_t>(argv[argc]+2);
				if ( (packet_size%3) != 0 )
				{
					fprintf(stderr, "%s: %d, Invalid parameter for -N. Using default value of 18\n", __FILE__, __LINE__);
					packet_size = 18;
					n_flag = FLAG_FAILED;
				}
				else if (packet_size < 9)
				{
					packet_size = 18;
				}
				break;
			
			case 'I':
			case 'i':
				// try to open the input file
				i_flag = FLAG_OK;
				f_in.open((argv[argc]+2), std::ifstream::in);
				if (f_in.fail())
				{
					fprintf(stderr, "%s: %d, Could not open file %s\n", __FILE__, __LINE__, (argv[argc]+2));
					i_flag = FLAG_FAILED;
				}
				break;

			case 'O':
			case 'o':
				// try to open the output file
				o_flag = FLAG_OK;
				f_out.open((argv[argc]+2));
				if (f_out.fail())
				{
					fprintf(stderr, "%s: %d, Invalid parameter for %s\n", __FILE__, __LINE__, (argv[argc]+2));
					o_flag = FLAG_FAILED;
				}
				break;

			default:
				fprintf(stderr, "%s: %d, Unknown parameter for %s\n", __FILE__, __LINE__, argv[argc]+1);
				exit(1);
				break;
		}
	}
	// make sure we have the min flags to continue...
	if (i_flag != FLAG_OK)
	{
		// we need to exit. This is a fatal error
		if (1 == f_out.is_open())
		{
			f_out.close();
		}
		exit(1);
	}
	// now validate the output flag
	if (o_flag != FLAG_OK)
	{
		if (o_flag == FLAG_FAILED)
		{
			// close the input file and exit
			f_in.close();
			exit(1);
		}
		else
		{
			// open as the default output file
			f_out.open("default.inc");
			if (f_out.fail())
			{
				fprintf(stderr, "%s: %d, Failed to create default file\n", __FILE__, __LINE__);
				exit(1);
			}
		}
	}	

	// build the new file
	buildIncFile(f_in, f_out, packet_size);
	
	// and close the resources
	f_in.close();
	f_out.close();
	
	return 0;
}

int buildIncFile( std::ifstream &in, std::ofstream &out, int32_t pkt_size)
{
	// used to parse the srec file
	char *line = (char *)calloc(1024, sizeof(char));
	char* line_base = line;
	if (line == NULL)
	{
		// signal an error to the console and exit
		fprintf(stderr, "%s: %d, Memory allocation failed.\n", __FILE__, __LINE__);
		return -1;
	}
	// Write a header for the file
	out << "// $Id$" << std::endl << std::endl
		  << "/**"  << std::endl
		  << " * @file <filename>" << std::endl
		  << " * " << std::endl
		  << " * This include file is for Freescale DSP (DSP563xx).  The data is transfered" << std::endl
		  << " * via CHIRP commands when the device is booted into PPP operational mode." << std::endl
		  << " *" << std::endl
		  << " * @brief This file contains the data to be transfered into a DSP563xx's" << std::endl
		  << " *         RAM and is registered as a SLOT PPP " << std::endl 
		  << " *" << std::endl
		  << " * @author <author>  " << std::endl
		  << " * " << std::endl
		  << " * @version <version> " << std::endl
		  << " * " << std::endl
		  << " */ " << std::endl << std::endl
		  << "// $Log$ " << std::endl << std::endl
		  << "#include <stdint.h>" << std::endl << std::endl;

	// here is where the file is formatted and created
	do {
		static int mem_space;
		// get a line from the input file (srec format)
		in >> line;
		// find out what type of record it is and create the new file
		// S0 is a memory space switch record
		if (strncmp(line, "S0", strlen("S0")) == 0)
		{
			// store the memory space for writing use
			char* type = (char*)calloc(16, sizeof(char)); 
			char* type_base = type;
			// make sure memory was allocated
			if (type == NULL)
			{
				fprintf(stderr, "%s: %d, Memory allocation failed.\n", __FILE__, __LINE__);
				free(line_base);
				return -1;
			}
			// get the memory space from the record
			memcpy(type, line+6, 2);
			// conver to an int type
			sscanf_s(type, "%x", &mem_space);
			// release memory
			free(type_base);
		}
		// S2 is a 24-bit address
		else if (strncmp(line, "S2", strlen("S2")) == 0)
		{
			// used for all mem spaces
			int rec_len = 0, write_cnt = 0;
			char* len = (char*)calloc(32, sizeof(char));
			char* len_base = len;
			// make sure memory was allocated
			if (len == NULL)
			{
				fprintf(stderr, "%s: %d, Memory allocation failed.\n", __FILE__, __LINE__);
				free(line_base);
				return -1;
			}
			// find out how large this record is
			memcpy(len, line+2, 2);
			// convert to integer
			sscanf_s(len, "%x", &rec_len);

			int offset_cnt = 0;
			char* tmp_address = (char*)calloc(32, sizeof(char));
			// make sure memory was allocated
			char* tmp_base = tmp_address;
			if (tmp_address == NULL)
			{
				fprintf(stderr, "%s: %d, Memory allocation failed.\n", __FILE__, __LINE__);
				free(len_base);
				free(line_base);
				return -1;
			}
			// offset the record type and amount
			line += 4;
			// start writing to the file - remove the address and cksum from the length
			rec_len -= 4;
			// +6 accomodates the PPP header
			//if ((rec_len+6) <= pkt_size)
			//{
			//	out << "// The packet below consists of " << (rec_len+6) << " bytes." << std::endl;
			//}
			//else
			//{
			//	out << "// The packet below consists of " << (pkt_size) << " bytes." << std::endl;
			//}
			// get the address fromt the line
			memcpy(tmp_address, line, 6);
			// remove all unnecessary leading 0's
			for(int i=0; i<6; i++)
			{
				// make and entire byte can be removed otherwise nothing at all
				if (*(tmp_address+i) == '0')
				{
					offset_cnt++;
				}
				else
				{
					i=6;
				}
			}
			// write for Yspace
			if (mem_space == 2)
			{
				// variable used for length of packet
				out << "uint32_t const PPP_Y" << (tmp_address+offset_cnt);
				if ((rec_len+6) <= pkt_size)
				{
					out << "_LEN = " << (rec_len+6) << ";" << std::endl;
				}
				else
				{
					out << "_LEN = " << (pkt_size) << ";" << std::endl;
				}
				// Header for the variable
				out << "uint8_t  const PPP_Y" << (tmp_address+offset_cnt) << "[] = {";
			}
			// write for Xspace
			else if (mem_space == 1)
			{
				// variable used for length of packet
				out << "uint32_t const PPP_X" << (tmp_address+offset_cnt);
				if ((rec_len+6) <= pkt_size)
				{
					out << "_LEN = " << (rec_len+6) << ";" << std::endl;
				}
				else
				{
					out << "_LEN = " << (pkt_size) << ";" << std::endl;
				}
				out << "uint8_t  const PPP_X" << (tmp_address+offset_cnt) << "[] = {";
			}
			// write for Pspace
			else if (mem_space == 4)
			{
				// variable used for length of packet
				out << "uint32_t const PPP_P" << (tmp_address+offset_cnt);
				if ((rec_len+6) <= pkt_size)
				{
					out << "_LEN = " << (rec_len+6) << ";" << std::endl;
				}
				else{
					out << "_LEN = " << (pkt_size) << ";" << std::endl;
				}
				out << "uint8_t  const PPP_P" << (tmp_address+offset_cnt) << "[] = {";
			}
			else
			{
				fprintf(stderr, "%s: %d, Unknown srec memory space.\n", __FILE__, __LINE__);
				free(len_base);
				free(line_base);
				free(tmp_base);
				return -1;
			}
			offset_cnt = 0;
			// bypass the address
			line += 6;
			// format the packet embedded amount of data that will be sent
			if ((rec_len+6) <= pkt_size)
			{
				sprintf_s(len, 16, "%02X", (rec_len/3));
			}
			else
			{
				sprintf_s(len, 16, "%02X", ((pkt_size-6)/3));
			}	
			
			// PPP header for Yspace
			if (mem_space == 2)
			{
				// Header for the variable
				out << "0xC6,0x00,0x" << len;
			}
			// write for Xspace
			else if (mem_space == 1)
			{
				out << "0xC5,0x00,0x" << len;
			}
			// write for Pspace
			else if (mem_space == 4)
			{
				out << "0xC4,0x00,0x" << len;
			}
			else
			{
				fprintf(stderr, "%s: %d, Unknown srec memory space.\n", __FILE__, __LINE__);
				free(len_base);
				free(line_base);
				free(tmp_base);
				return -1;
			}
			// and the starting address of the packet
			out   << ",0x" << tmp_address[0] << tmp_address[1] << ",0x" 
				  << tmp_address[2] << tmp_address[3] << ",0x" << tmp_address[4] 
				  << tmp_address[5];
			// write to the file minding the amount requested by the caller
			do {
				// keep the packets of a reasonable size
				if ((write_cnt+6) < pkt_size)
				{
					write_cnt++;
					out << ",0x" << line[offset_cnt] << line[offset_cnt+1];
					offset_cnt += 2;
					rec_len--;
				}
				// end the last line
				else
				{
					// create the new address
					int new_address=0, off_cnt=0;
					// end the last packet
					out << "};" << std::endl << std::endl;
					// and start a new line
					// +6 accomodates the PPP header
					//if ((rec_len+6) <= pkt_size)
					//{
					//	out << "// The below packet consists of " << (rec_len+6) << " bytes." << std::endl;
					//}
					//else
					//{
					//	out << "// The below packet consists of " << (pkt_size) << " bytes." << std::endl;
					//}
					// convert the address to int
					sscanf_s(tmp_address, "%x", &new_address);
					// find the new starting address - remove the header and account for 24bit data
					new_address += (write_cnt/3);
					write_cnt = 0;
					sprintf_s(tmp_address, 16, "%06X", new_address);
					// clear the un-needed 0's
					for(int i=0; i<6; i++)
					{
						if (*(tmp_address+i) == '0')
						{
							off_cnt++;
						}
						else
						{
							i=6;
						}
					}
					// write for Yspace
					if (mem_space == 2)
					{
						// variable used for length of packet
						out << "uint32_t const PPP_Y" << (tmp_address+off_cnt);
						if ((rec_len+6) <= pkt_size)
						{
							out << "_LEN = " << (rec_len+6) << ";" << std::endl;
						}
						else
						{
							out << "_LEN = " << (pkt_size) << ";" << std::endl;
						}
						out << "uint8_t  const PPP_Y" << (tmp_address+off_cnt) << "[] = {";
					}
					// write for Xspace
					else if (mem_space == 1)
					{
						// variable used for length of packet
						out << "uint32_t const PPP_X" << (tmp_address+off_cnt);
						if ((rec_len+6) <= pkt_size)
						{
							out << "_LEN = " << (rec_len+6) << ";" << std::endl;
						}
						else
						{
							out << "_LEN = " << (pkt_size) << ";" << std::endl;
						}
						out << "uint8_t  const PPP_X" << (tmp_address+off_cnt) << "[] = {";
					}
					// write for Pspace
					else if (mem_space == 4)
					{
						// variable used for length of packet
						out << "uint32_t const PPP_P" << (tmp_address+off_cnt);
						if ((rec_len+6) <= pkt_size)
						{
							out << "_LEN = " << (rec_len+6) << ";" << std::endl;
						}
						else
						{
							out << "_LEN = " << (pkt_size) << ";" << std::endl;
						}
						out << "uint8_t  const PPP_P" << (tmp_address+off_cnt) << "[] = {";
					}
					else
					{
						fprintf(stderr, "%s: %d, Unknown srec memory space.\n", __FILE__, __LINE__);
						free(len_base);
						free(line_base);
						free(tmp_base);
						return -1;
					}
					// format the packet embedded amount of data that will be sent
					len = len_base;
					if ((rec_len+6) < pkt_size)
					{
						sprintf_s(len, 16, "%02X", (rec_len/3));
					}
					else
					{
						sprintf_s(len, 16, "%02X", ((pkt_size-6)/3));
					}	
			
					// PPP header for Yspace
					if (mem_space == 2)
					{
						// Header for the variable
						out << "0xC6,0x00,0x" << len;
					}
					// write for Xspace
					else if (mem_space == 1)
					{
						out << "0xC5,0x00,0x" << len;
					}
					// write for Pspace
					else if (mem_space == 4)
					{
						out << "0xC4,0x00,0x" << len;
					}
					else
					{
						fprintf(stderr, "%s: %d, Unknown srec memory space.\n", __FILE__, __LINE__);
						free(len_base);
						free(line_base);
						free(tmp_base);
						return -1;
					}
					// and the starting address of the packet
					out   << ",0x" << tmp_address[0] << tmp_address[1] << ",0x" 
						  << tmp_address[2] << tmp_address[3] << ",0x" << tmp_address[4] 
						  << tmp_address[5];
				}

			} while(rec_len != 0);
			// close off the line
			out << "};" << std::endl << std::endl;
			// release from the heap
			free(tmp_base);
			free(len_base);
		}
		// S8 is an end of file record
		else if (strncmp(line, "S8", strlen("S8")) == 0)
		{
			// reset the memory space indicator
			mem_space = 0;
		}
		// record that is not currently supported
		else
		{
			// reset the memory space indicator
			mem_space = 0;
		}

	} while(!in.eof());
	// release memory
	free(line_base);

	return 0;
}
