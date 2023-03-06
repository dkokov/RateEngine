#
# * Tests Makefile *
#
#   Created by Dimitar Kokov ,dkokov75 at gmail dot com  (2019-06-03)
#
#   History:
#	
#

include=../Makefile

all: json_ext_test json_ext_test_2 json_ext_test_2b json_ext_cdrm mem_test mem_test_arr mem_test_stack

json_ext_test: json_ext_test.c
	$(CC) -Wall -g -o json_ext_test json_ext_test.c -ljson-c -L../ -lre7core

json_ext_test_2: json_ext_test_2.c
	$(CC) -Wall -g -o json_ext_test_2 json_ext_test_2.c -ljson-c -L../ -lre7core

json_ext_test_2b: json_ext_test_2b.c
	$(CC) -Wall -g -o json_ext_test_2b json_ext_test_2b.c -ljson-c -L../ -lre7core

json_rpc_test: json_rpc_test.c
	$(CC) -Wall -g -o json_rpc_test json_rpc_test.c -ljson-c -L../ -lre7core

json_rpc_test_2: json_rpc_test_2.c
	$(CC) -Wall -g -o json_rpc_test_2 json_rpc_test_2.c -ljson-c -L../ -lre7core

json_ext_cdrm: json_ext_cdrm.c
	$(CC) -Wall -g -o json_ext_cdrm json_ext_cdrm.c -ljson-c -L../ -lre7core

mem_test: mem_test.c
	$(CC) -Wall -g -o mem_test mem_test.c -L../ -lre7core

mem_test_arr: mem_test_arr.c
	$(CC) -Wall -g -o mem_test_arr mem_test_arr.c -L../ -lre7core

mem_test_stack: mem_test_stack.c
	$(CC) -Wall -g -o mem_test_stack mem_test_stack.c -L../ -lre7core

mem_list_test: mem_list_test.c
	$(CC) -Wall -g -o mem_list_test mem_list_test.c -L../ -lre7core

clean:
	@rm -fv json_ext_test json_ext_test_2 json_ext_test_2b json_rpc_test json_rpc_test_2 json_ext_cdrm mem_list_test mem_test mem_test_arr mem_test_stack *.json
